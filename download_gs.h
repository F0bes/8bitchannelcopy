#pragma once
// Quick download from gs function

#include "defs.h"
#include "memmap.h"
#include <stdio.h>
#include <tamtypes.h>

int download_from_gs(u32 address, u32 format, const char *fname, int psm_size, int width, int height)
{

	u32* destBuffer = aligned_alloc(64, width * height * sizeof(u32));

	static union
	{
		u32 value_u32[4];
		u128 value;
	} enable_path3 ALIGNED(16) = {
		{VIF1_MSKPATH3(0), VIF1_NOP, VIF1_NOP, VIF1_NOP}};

	u32 prev_imr;
	u32 prev_chcr;

	u32 dmaChain[20 * 2] ALIGNED(16);
	u32 i = 0; // Used to index through the chain

	u32 *pdma32 = (u32 *)&dmaChain;
	u64 *pdma64 = (u64 *)(pdma32 + 4);

	// Set up VIF packet
	pdma32[i++] = VIF1_NOP;
	pdma32[i++] = VIF1_MSKPATH3(0x8000);
	pdma32[i++] = VIF1_FLUSHA;
	pdma32[i++] = VIF1_DIRECT(6);

	// Set up our GS packet
	i = 0;

	pdma64[i++] = GIFTAG(5, 1, 0, 0, 0, 1);
	pdma64[i++] = GIF_AD;
	// pdma64[i++] = GSBITBLTBUF_SET(address / 64, width / 64, format, 0, 0, 0);
	pdma64[i++] = GSBITBLTBUF_SET(address / 64, width < 64 ? 1 : width / 64, format, 0, 0, 0);
	pdma64[i++] = GSBITBLTBUF;
	pdma64[i++] = GSTRXPOS_SET(0, 0, 0, 0, 0); // SSAX, SSAY, DSAX, DSAY, DIR
	pdma64[i++] = GSTRXPOS;
	pdma64[i++] = GSTRXREG_SET(width, height); // RRW, RRh
	pdma64[i++] = GSTRXREG;
	pdma64[i++] = 0;
	pdma64[i++] = GSFINISH;
	pdma64[i++] = GSTRXDIR_SET(1); // XDIR
	pdma64[i++] = GSTRXDIR;

	prev_imr = GsPutIMR(GsGetIMR() | 0x0200);
	prev_chcr = *D1_CHCR;

	if ((*D1_CHCR & 0x0100) != 0)
	{
		free(destBuffer);
		return 0;
	}

	// Finish event
	*GS_CSR = CSR_FINISH;

	// DMA crap

	
	printf("DMA stuff\n");
	*D1_QWC = 0x7;
	*D1_MADR = (u32)pdma32;
	FlushCache(0);
	printf("%X\n",*D1_MADR);
	*D1_CHCR = 0x101;

	asm __volatile__("sync.l\n");

	// check if DMA is complete (STR=0)

	printf("Waiting for DMA channel completion\n");
	while (*D1_CHCR & 0x0100)
		;
	printf("Waiting for GS_CSR\n");
	while ((*GS_CSR & CSR_FINISH) == 0)
		;
	printf("Waiting for the vif fifo to empty\n");
	// Wait for viffifo to become empty
	while ((*VIF1_STAT & (0x1f000000)))
		;

	// Reverse busdir and transfer image to host
	*VIF1_STAT = VIF1_STAT_FDR;
	*GS_BUSDIR = (u64)0x00000001;

	FlushCache(0);

	u32 trans_size = ((width * height * psm_size) / 0x100);

	printf("Download the frame in 2 transfers of qwc 0x%x\n", trans_size);

	*D1_QWC = trans_size;
	*D1_MADR = (u32)destBuffer;
	*D1_CHCR = 0x100;

	asm __volatile__(" sync.l\n");

	// check if DMA is complete (STR=0)
	while (*D1_CHCR & 0x0100)
		;

	*D1_QWC = trans_size;
	*D1_CHCR = 0x100;

	asm __volatile__(" sync.l\n");

	// Wait for viffifo to become empty
	while ((*VIF1_STAT & (0x1f000000)))
		;

	// check if DMA is complete (STR=0)
	while (*D1_CHCR & 0x0100)
		;

	*D1_CHCR = prev_chcr;

	asm __volatile__(" sync.l\n");

	*VIF1_STAT = 0;
	*GS_BUSDIR = (u64)0;
	// Put back prew imr and set finish event

	GsPutIMR(prev_imr);
	*GS_CSR = CSR_FINISH;
	// Enable path3 again
	*VIF1_FIFO = enable_path3.value;

	/*
		Write out to a file
	*/

	FILE *f = fopen(fname, "w+");
	if (f == NULL)
	{
		printf("??? couldn't open file\n");
		free(destBuffer);
		return 0;
	}
	printf("File should be size %d\n", (psm_size / 8) * width * height);
	fwrite(destBuffer, psm_size / 8, width * height, f);
	fclose(f);
	free(destBuffer);
	return 0;
}

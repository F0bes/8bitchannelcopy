#include "newipu.h"

#include <kernel.h>
#include <graph.h>
#include <draw.h>
#include <gs_psm.h>
#include <stdlib.h>
#include <dma.h>
#include <gs_gp.h>

/*
	Simple example of using the newipu library to perform colour space
	conversion on a 256x256 pixel block of data.

	An optimization would be to use scratchpad instead of main memory
	and PATH2 instead of PATH3

	WARNING: Flashing Colours
*/

framebuffer_t fb;
zbuffer_t zb;
// Initialise the GS with some generic settings
// 640x448, 24 bit frame
// Disabled zbuffer
// 0,0 xyoffset
void init_gs(void)
{
	fb.width = 640;
	fb.height = 448;
	fb.psm = GS_PSM_24;
	fb.address = graph_vram_allocate(fb.width, fb.height, fb.psm, GRAPH_ALIGN_PAGE);
	fb.mask = 0;

	zb.zsm = GS_PSMZ_32;
	zb.address = graph_vram_allocate(fb.width, fb.height, zb.zsm, GRAPH_ALIGN_PAGE);
	zb.enable = 0;
	zb.method = ZTEST_METHOD_ALLPASS;
	zb.mask = 0;

	graph_initialize(fb.address, fb.width, fb.height, fb.psm, 0, 0);

	qword_t* init_data = (qword_t*)aligned_alloc(64, sizeof(qword_t) * 30);
	qword_t* q = init_data;

	q = draw_setup_environment(q, 0, &fb, &zb);
	q = draw_primitive_xyoffset(q, 0, 0, 0);
	q = draw_disable_tests(q, 0, &zb);

	// Draw a full screen sprite, a nice blue colour
	PACK_GIFTAG(q, GIF_SET_TAG(1, 1, GIF_PRE_ENABLE, GIF_PRIM_SPRITE, GIF_FLG_PACKED, 3),
		GIF_REG_RGBAQ | (GIF_REG_XYZ2 << 4) | (GIF_REG_XYZ2 << 8));
	q++;
	// RGBAQ
	q->dw[0] = (u64)((0x00) | ((u64)0x33 << 32));
	q->dw[1] = (u64)((0x33) | ((u64)0xFF << 32));
	q++;
	// XYZ2
	q->dw[0] = (u64)((((0 << 4)) | (((u64)(0 << 4)) << 32)));
	q->dw[1] = (u64)(1);
	q++;
	q->dw[0] = (u64)((((fb.width << 4)) | (((u64)(fb.height << 4)) << 32)));
	q->dw[1] = (u64)(1);
	q++;

	dma_channel_send_normal(DMA_CHANNEL_GIF, init_data, q - init_data, 0, 0);
	dma_channel_fast_waits(DMA_CHANNEL_GIF);
	dma_wait_fast();

	free(init_data);
}

// Holds 8x8 pixels of data
IPU::Types::Data::MACROBLOCK8* data;
IPU::Types::Data::RGBA32* data_out;

void generateRAW8Data()
{
	// Randomly generate a YCbCr Colour

	u32 Y = rand() * 255;
	u32 Cb = rand() * 255;
	u32 Cr = rand() * 255;

	for (int i = 0; i < 256; i++)
	{

		for (int j = 0; j < 16; j++)
		{

			for (int k = 0; k < 16; k++)
			{

				data[i].Y[j][k] = Y;
				data[i].Cb[j >> 1][k >> 1] = Cb;
				data[i].Cr[j >> 1][k >> 1] = Cr;
			}
		}
	}
	FlushCache(0);
}

u32 texture_address = 0;
// Draw and upload a sprite, textured with the IPU data
void draw_image_data()
{
	if (texture_address == 0)
		texture_address = graph_vram_allocate(256, 256, GS_PSM_32, GRAPH_ALIGN_PAGE);

	qword_t up_data[12] __aligned(64);
	qword_t* q = up_data;

	q = draw_texture_transfer(q, data_out, 256, 256, GS_PSM_32, texture_address, 256);
	q = draw_texture_flush(q);
	dma_channel_send_chain(DMA_CHANNEL_GIF, up_data, q - up_data, 0, 0);
	dma_wait_fast();

	q = up_data;
	PACK_GIFTAG(q, GIF_SET_TAG(1, 1, GIF_PRE_ENABLE, GIF_SET_PRIM(GIF_PRIM_SPRITE, 0, 1, 0, 1, 0, 1, 0, 0), GIF_FLG_PACKED, 6),
		GIF_REG_AD | (GIF_REG_UV << 4) | (GIF_REG_RGBAQ << 8) | (GIF_REG_XYZ2 << 12) | (GIF_REG_UV << 16) | (GIF_REG_XYZ2 << 20));
	q++;
	// TEX0
	q->dw[0] = GS_SET_TEX0(texture_address / 64, 4, GS_PSM_32, 8, 8, 1, TEXTURE_FUNCTION_DECAL, 0, 0, 0, 0, 0);
	q->dw[1] = GS_REG_TEX0;
	q++;
	// UV
	q->dw[0] = GIF_SET_ST(0, 0);
	q->dw[1] = 0x3f800000;
	q++;
	// RGBAQ
	q->dw[0] = (u64)((0x00) | ((u64)0xFF << 32));
	q->dw[1] = (u64)((0xFF) | ((u64)0xFF << 32));
	q++;
	// XYZ2
	q->dw[0] = (u64)(((320 - 128) << 4) | (((u64)((224 + 128) << 4)) << 32));
	q->dw[1] = (u64)(1);
	q++;
	// UV
	q->dw[0] = GIF_SET_ST(256 << 4, 256 << 4);
	q->dw[1] = 0x3f800000;
	q++;
	// XYZ2
	q->dw[0] = (u64)(((320 + 128) << 4) | (((u64)((224 - 128) << 4)) << 32));
	q->dw[1] = (u64)(1);
	q++;

	dma_channel_send_normal(DMA_CHANNEL_GIF, up_data, q - up_data, 0, 0);
	dma_channel_fast_waits(DMA_CHANNEL_GIF);
}

u32 ipu_from_qwc_rem = 0;
u32 ipu_to_qwc_rem = 0;

// On DMA interrupts for IPU_TO and IPU_FROM
// it'll check if the channel has been drained
// and if there is more to be sent
// If so, load up the QWC and start the transfer again
// In this example, our 256x256 image does not require this
s32 ipuDMAHandler(s32 cause)
{
	IPU_DEBUG("IPU handler called\n");
	if (ipu_from_qwc_rem > 0 && !*IPU::IO::IPU0QWC)
	{
		IPU_DEBUG("ADDING %d QW TO FROM\n", ipu_from_qwc_rem);
		*IPU::IO::IPU0QWC = ipu_from_qwc_rem > 0xFFFF ? 0xFFFF : ipu_from_qwc_rem;
		ipu_from_qwc_rem -= *IPU::IO::IPU0QWC;
		*IPU::IO::IPU0CHCR = 0x100;
	}

	if (ipu_to_qwc_rem > 0 && !*IPU::IO::IPU1QWC)
	{
		IPU_DEBUG("ADDING %d QW TO TO\n", ipu_to_qwc_rem);
		*IPU::IO::IPU1QWC = ipu_to_qwc_rem > 0xFFFF ? 0xFFFF : ipu_to_qwc_rem;
		ipu_to_qwc_rem -= *IPU::IO::IPU1QWC;
		*IPU::IO::IPU1CHCR = 0x100;
	}
	ExitHandler();
	return 0;
}

u32 thread_id = 0;
u8 thread_stack[4096] alignas(16);
void workThread(void* args)
{
	// Initialise the IPU
	ipuInit();

	DisableDmac(DMAC_FROM_IPU);
	DisableDmac(DMAC_TO_IPU);

	// TODO: Implement this in the library
	AddDmacHandler(DMAC_FROM_IPU, ipuDMAHandler, 0);
	AddDmacHandler(DMAC_TO_IPU, ipuDMAHandler, 0);

	// Create our main memory buffers for our data going to and from the IPU
	data = (IPU::Types::Data::MACROBLOCK8*)aligned_alloc(16, sizeof(IPU::Types::Data::MACROBLOCK8) * 1024);
	data_out = (IPU::Types::Data::RGBA32*)aligned_alloc(16, sizeof(IPU::Types::Data::RGBA32) * 256);

	// Set up our DMAC channel structures

	IPU::Types::DMA::IPUDMA dma_To;
	ipu_to_qwc_rem = (sizeof(IPU::Types::Data::MACROBLOCK8) * 256) / 16;
	dma_To.QWC = ipu_to_qwc_rem < 0xFFFF ? ipu_to_qwc_rem : 0xFFFF;
	ipu_to_qwc_rem -= dma_To.QWC;
	dma_To.SPR = 0;
	dma_To.MADR = (u32)data;

	IPU::Types::DMA::IPUDMA dma_From;
	ipu_from_qwc_rem = (sizeof(IPU::Types::Data::RGBA32) * 256) / 16;
	dma_From.QWC = ipu_from_qwc_rem < 0xFFFF ? ipu_from_qwc_rem : 0xFFFF;
	ipu_from_qwc_rem -= dma_From.QWC;
	dma_From.SPR = 0;
	dma_From.MADR = (u32)data_out;

	// Set our threshold
	ipuSETTH(0, 0x100);

	EnableDmac(DMAC_FROM_IPU);
	EnableDmac(DMAC_TO_IPU);
	while (1)
	{
		generateRAW8Data();

		// Reload our remaining data counts
		// This is so our DMAC handler knows how much data needs to be processed
		ipu_to_qwc_rem = (sizeof(IPU::Types::Data::MACROBLOCK8) * 256) / 16;
		ipu_to_qwc_rem -= dma_To.QWC;
		ipu_from_qwc_rem = ((sizeof(IPU::Types::Data::RGBA32) * 256) / 16);
		ipu_from_qwc_rem -= dma_From.QWC;

		// Load our configuration into the DMA channels
		// Resetting our MADR and QWC
		ipuSetDMA(&dma_From, &dma_To);

		// Execute the CSC instruction
		// No Dithering
		// RGB32 Output
		// 1024 Macroblocks

		ipuCSC(IPU::Options::Commands::DTE::NoDithering,
			IPU::Options::Commands::OFM::RGB32, 256);

		// Wait for the IPU (No timeout, wait indefinitely)
		ipuWait(0);

		// Currently, ipuWait does not wait for the IPU DMA channels
		// Only the busy bit in IPU_CTRL
		// It's best to make sure that your data has reached it's destination before
		// throwing it at the GS
		dma_channel_wait(DMA_CHANNEL_fromIPU, 0);

		// Upload and draw our data
		draw_image_data();

		// Wait for vsync
		graph_wait_vsync();
	}
}

int main(void)
{
	// Set up Drawing and Display environment
	init_gs();

	ee_thread_t ee_thread;
	ee_thread.func = (void*)workThread;
	ee_thread.initial_priority = 0x40;
	ee_thread.gp_reg = &_gp;
	ee_thread.stack = thread_stack;
	ee_thread.stack_size = sizeof(thread_stack);

	thread_id = CreateThread(&ee_thread);

	StartThread(thread_id, nullptr);

	IPU_DEBUG("Sleeping this main thread\n");

	SleepThread();
}

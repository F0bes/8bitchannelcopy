#include <stdlib.h>
#include <kernel.h>
#include <draw.h>
#include <graph.h>
#include <gs_gp.h>
#include <gs_psm.h>
#include <dma.h>

#include "download_gs.h"

framebuffer_t fb;
zbuffer_t zb;
void init_gs(void)
{
	*(u64*)0x10003000 = 1;
	fb.width = 640;
	fb.height = 448;
	fb.psm = GS_PSM_32;
	fb.address = graph_vram_allocate(fb.width, fb.height, fb.psm, GRAPH_ALIGN_PAGE);
	fb.mask = 0;

	zb.zsm = GS_PSMZ_32;
	zb.address = graph_vram_allocate(fb.width, fb.height, zb.zsm, GRAPH_ALIGN_PAGE);
	zb.enable = 0;
	zb.method = ZTEST_METHOD_ALLPASS;
	zb.mask = 0;

	graph_initialize(fb.address, fb.width, fb.height, fb.psm, 0, 0);

	graph_wait_vsync();

	qword_t* init_data = (qword_t*)aligned_alloc(64, sizeof(qword_t) * 30);
	qword_t* q = init_data;

	q = draw_setup_environment(q, 0, &fb, &zb);
	q = draw_primitive_xyoffset(q, 0, 0, 0);
	q = draw_disable_tests(q, 0, &zb);

	PACK_GIFTAG(q, GIF_SET_TAG(1, 1, GIF_PRE_ENABLE, GIF_PRIM_SPRITE, GIF_FLG_PACKED, 4),
		GIF_REG_RGBAQ | (GIF_REG_XYZ2 << 4) | (GIF_REG_XYZ2 << 8) | (GIF_REG_AD << 12));
	q++;
	// RGBAQ
	q->dw[0] = (u64)((0x00) | ((u64)0x00 << 32));
	q->dw[1] = (u64)((0x00) | ((u64)0xFF << 32));
	q++;
	// XYZ2
	q->dw[0] = (u64)((((0 << 4)) | (((u64)(0 << 4)) << 32)));
	q->dw[1] = (u64)(1);
	q++;
	q->dw[0] = (u64)((((fb.width << 4)) | (((u64)(fb.height << 4)) << 32)));
	q->dw[1] = (u64)(1);
	q++;
	q->dw[0] = GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 1);
	q->dw[1] = GS_REG_TEST;
	q++;

	dma_channel_send_normal(DMA_CHANNEL_GIF, init_data, q - init_data, 0, 0);
	dma_channel_fast_waits(DMA_CHANNEL_GIF);
	dma_wait_fast();

	free(init_data);
}

void uploadCLUT()
{
	u32* PAL = aligned_alloc(64, sizeof(u32) * 256);
	for (int i = 0; i < 256; i++)
	{
		PAL[i] = (i << 24) | (i << 16) | (i << 8) | (i << 0);
	}
	FlushCache(0);

	qword_t transfer_chain[30] ALIGNED(64);
	qword_t* q = transfer_chain;

	q = draw_texture_transfer(q, PAL, 16, 16, GS_PSM_32, 0, 256);
	q = draw_texture_flush(q);

	dma_channel_send_chain(DMA_CHANNEL_GIF, transfer_chain, q - transfer_chain, 0, 0);
	dma_wait_fast();
	free(PAL);
}

void drawFlatSprite()
{
	qword_t* draw_packet = aligned_alloc(64, sizeof(qword_t) * 50);
	qword_t* q = draw_packet;

	// Draw two gouraud shaded triangles

	PACK_GIFTAG(q, GIF_SET_TAG(6, 1, GIF_PRE_ENABLE, GIF_SET_PRIM(GIF_PRIM_TRIANGLE, 1, 0, 0, 0, 0, 0, 0, 0), GIF_FLG_PACKED, 2),
		GIF_REG_RGBAQ | (GIF_REG_XYZ2 << 4));
	q++;
	{
		// RGBAQ
		q->dw[0] = (u64)((0xFF) | ((u64)0x00 << 32));
		q->dw[1] = (u64)((0x00) | ((u64)0xFF << 32));
		q++;
		// XYZ2
		q->dw[0] = (u64)((((64 << 4)) | (((u64)(0 << 4)) << 32)));
		q->dw[1] = (u64)(1);
		q++;
		// RGBAQ
		q->dw[0] = (u64)((0x00) | ((u64)0xFF << 32));
		q->dw[1] = (u64)((0x00) | ((u64)0x00 << 32));
		q++;
		// XYZ2
		q->dw[0] = (u64)((((64 << 4)) | (((u64)(32 << 4)) << 32)));
		q->dw[1] = (u64)(1);
		q++;
		// RGBAQ
		q->dw[0] = (u64)((0x00) | ((u64)0x00 << 32));
		q->dw[1] = (u64)((0xFF) | ((u64)0x00 << 32));
		q++;
		// XYZ2
		q->dw[0] = (u64)((((128 << 4)) | (((u64)(32 << 4)) << 32)));
		q->dw[1] = (u64)(1);
		q++;
	}
	{
		// RGBAQ
		q->dw[0] = (u64)((0x00) | ((u64)0x00 << 32));
		q->dw[1] = (u64)((0xFF) | ((u64)0x00 << 32));
		q++;
		// XYZ2
		q->dw[0] = (u64)((((128 << 4)) | (((u64)(32 << 4)) << 32)));
		q->dw[1] = (u64)(1);
		q++;
		// RGBAQ
		q->dw[0] = (u64)((0x00) | ((u64)0xFF << 32));
		q->dw[1] = (u64)((0x00) | ((u64)0xFF << 32));
		q++;
		// XYZ2
		q->dw[0] = (u64)((((128 << 4)) | (((u64)(0 << 4)) << 32)));
		q->dw[1] = (u64)(1);
		q++;
		// RGBAQ
		q->dw[0] = (u64)((0xFF) | ((u64)0x00 << 32));
		q->dw[1] = (u64)((0x00) | ((u64)0xFF << 32));
		q++;
		// XYZ2
		q->dw[0] = (u64)((((64 << 4)) | (((u64)(0 << 4)) << 32)));
		q->dw[1] = (u64)(1);
		q++;
	}


	/*
PACK_GIFTAG(q, GIF_SET_TAG(1, 1, GIF_PRE_ENABLE, GIF_PRIM_SPRITE, GIF_FLG_PACKED, 3),
			GIF_REG_RGBAQ | (GIF_REG_XYZ2 << 4) | (GIF_REG_XYZ2 << 8));
q++;
{
	// RGBAQ
	q->dw[0] = (u64)((0x00) | ((u64)0x00 << 32));
	q->dw[1] = (u64)((0x44) | ((u64)0x00 << 32));
	q++;
	// XYZ2
	q->dw[0] = (u64)((((64 << 4)) | (((u64)(0 << 4)) << 32)));
	q->dw[1] = (u64)(1);
	q++;
	// XYZ2
	q->dw[0] = (u64)((((128 << 4)) | (((u64)(32 << 4)) << 32)));
	q->dw[1] = (u64)(1);
	q++;
}
*/
	dma_channel_send_normal(DMA_CHANNEL_GIF, draw_packet, q - draw_packet, 0, 0);
	dma_wait_fast();
	free(draw_packet);
}

typedef enum
{
	CHANNEL_RED,
	CHANNEL_GREEN,
	CHANNEL_BLUE,
	CHANNEL_ALPHA,
} ColourChannels;

// Channel copy a block
void performChannelCopy(ColourChannels channelIn, ColourChannels channelOut, u32 blockX, u32 blockY, u32 source)
{
	qword_t* copy_packet = aligned_alloc(64, sizeof(qword_t) * 500);
	qword_t* q = copy_packet;

	q = copy_packet;

	// For the BLUE and ALPHA channels, we need to offset our 'U's by 8 texels
	const u32 horz_block_offset = (channelIn == CHANNEL_BLUE || channelIn == CHANNEL_ALPHA);
	// For the GREEN and ALPHA channels, we need to offset our 'T's by 2 texels
	const u32 vert_block_offset = (channelIn == CHANNEL_GREEN || channelIn == CHANNEL_ALPHA);

	const u32 clamp_horz = horz_block_offset ? 8 : 0;
	const u32 clamp_vert = vert_block_offset ? 2 : 0;
	PACK_GIFTAG(q, GIF_SET_TAG(4, 1, GIF_PRE_DISABLE, 0, GIF_FLG_PACKED, 1),
		(GIF_REG_AD));
	q++;
	// TEX0
	q->dw[0] = GS_SET_TEX0((source >> 6), 2, GS_PSM_8, 10, 10, 1, 1, 0, GS_PSM_32, 0, 0, 1);
	q->dw[1] = GS_REG_TEX0;
	q++;
	// CLAMP
	q->dw[0] = GS_SET_CLAMP(3, 3, 0xF7, clamp_horz, 0xFD, clamp_vert);
	q->dw[1] = GS_REG_CLAMP;
	q++;
	// TEXFLUSH
	q->dw[0] = GS_SET_TEXFLUSH(1);
	q->dw[1] = GS_REG_TEXFLUSH;
	q++;
	u32 frame_mask;
	switch (channelOut)
	{
		case CHANNEL_RED:
			frame_mask = ~0x000000FF;
			break;
		case CHANNEL_GREEN:
			frame_mask = ~0x0000FF00;
			break;
		case CHANNEL_BLUE:
			frame_mask = ~0x00FF0000;
			break;
		case CHANNEL_ALPHA:
			frame_mask = ~0xFF000000;
			break;
	}
	// FRAME
	q->dw[0] = GS_SET_FRAME(0, 10, 0, frame_mask);
	q->dw[1] = GS_REG_FRAME;
	q++;

	PACK_GIFTAG(q, GIF_SET_TAG(96, 1, GIF_PRE_ENABLE, GIF_SET_PRIM(GS_PRIM_SPRITE, 0, 1, 0, 0, 0, 1, 0, 0), GIF_FLG_PACKED, 4),
		(GIF_REG_UV) | (GIF_REG_XYZ2 << 4) | (GIF_REG_UV << 8) | (GIF_REG_XYZ2 << 12));
	q++;
	for (int y = 0; y < 32; y += 2)
	{
		if (((y % 4) == 0) ^ (vert_block_offset == 1)) // Even (4 16x2 sprites)
		{
			for (int x = 0; x < 64; x += 16)
			{
				// UV
				q->dw[0] = GS_SET_ST(8 + ((8 + x * 2) << 4), 8 + ((y * 2) << 4));
				q->dw[1] = 0;
				q++;
				// XYZ2
				q->dw[0] = (u64)((x + blockX) << 4) | ((u64)((y + blockY) << 4) << 32);
				q->dw[1] = (u64)(1);
				q++;
				// UV
				q->dw[0] = GS_SET_ST(8 + ((24 + x * 2) << 4), 8 + ((2 + y * 2) << 4));
				q->dw[1] = 0;
				q++;
				// XYZ2
				q->dw[0] = (u64)((x + 16 + blockX) << 4) | ((u64)((y + 2 + blockY) << 4) << 32);
				q->dw[1] = (u64)(1);
				q++;
			}
		}
		else // Odd (Eight 8x2 sprites)
		{
			for (int x = 0; x < 64; x += 8)
			{
				// UV
				q->dw[0] = GS_SET_ST(8 + ((4 + x * 2) << 4), 8 + ((y * 2) << 4));
				q->dw[1] = 0;
				q++;
				// XYZ2
				q->dw[0] = (u64)((x + blockX) << 4) | ((u64)((y + blockY) << 4) << 32);
				q->dw[1] = (u64)(1);
				q++;
				// UV
				q->dw[0] = GS_SET_ST(8 + ((12 + x * 2) << 4), 8 + ((2 + y * 2) << 4));
				q->dw[1] = 0;
				q++;
				// XYZ2
				q->dw[0] = (u64)((x + 8 + blockX) << 4) | ((u64)((y + 2 + blockY) << 4) << 32);
				q->dw[1] = (u64)(1);
				q++;
			}
		}
	}
	FlushCache(0);
	dma_channel_send_normal(DMA_CHANNEL_GIF, copy_packet, q - copy_packet, 0, 0);
	dma_wait_fast();

	q = copy_packet;
	PACK_GIFTAG(q, GIF_SET_TAG(1, 1, GIF_PRE_DISABLE, 0, GIF_FLG_PACKED, 1),
		(GIF_REG_AD));
	q++;
	// FRAME
	q->dw[0] = GS_SET_FRAME(0, 10, 0, 0x00);
	q->dw[1] = GS_REG_FRAME;
	q++;

	dma_channel_send_normal(DMA_CHANNEL_GIF, copy_packet, q - copy_packet, 0, 0);
	dma_wait_fast();

	free(copy_packet);
}

int main(void)
{
	int downloaded_frame = 0;
	int frame_count = 0;
	init_gs();
	while (1)
	{
		frame_count++;
/*		if (downloaded_frame == 0 && frame_count == 200)
		{
			downloaded_frame = 1;
			dma_channel_wait(DMA_CHANNEL_GIF, 0);
			download_from_gs(fb.address, GS_PSM_24, "host:8bitcopy.data", 24, 640, 448);
		}
*/		uploadCLUT();
		drawFlatSprite();
		performChannelCopy(CHANNEL_RED, CHANNEL_RED, 128, 0, 2048);
		performChannelCopy(CHANNEL_GREEN, CHANNEL_GREEN, 128, 32, 2048);
		performChannelCopy(CHANNEL_BLUE, CHANNEL_BLUE, 128, 64, 2048);
		performChannelCopy(CHANNEL_ALPHA, CHANNEL_RED, 128, 96, 2048);

		graph_wait_vsync();
	}
	printf("Sleeping thread\n");
	SleepThread();
}

#include <fat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <malloc.h>
#include <math.h>
#include <gccore.h>
#include <wiiuse/wpad.h>

#include "console_ui.h"
#include "settings.h"

// #include "NeHe_tpl.h"
// #include "NeHe.h"

#define DEFAULT_FIFO_SIZE	(64*1024) // wiiSX uses min FIFO size which is 64x1024 bytes = 65536 bytes = 64kb, also not coincidentally the max vertex draws is 65536 https://libogc.devkitpro.org/gx_8h.html#a8bd2dc908dea327389c951dd87b9db58
// The Graphics FIFO is the mechanism used to communicate graphics commands from the CPU to the Graphics Processor (GP). The FIFO base pointer should be 32-byte aligned. memalign() can return 32-byte aligned pointers. The size should also be a multiple of 32B.
// The CPU's write-gather pipe is used to write data to the FIFO. Therefore, the FIFO memory area must be forced out of the CPU cache prior to being used. DCInvalidateRange() may be used for this purpose. Due to the mechanics of flushing the write-gather pipe, the FIFO memory area should be at least 32 bytes larger than the maximum expected amount of data stored. Up to 32 NOPs may be written at the end during flushing.
// Note GX_Init() also takes the arguments base and size and initializes a FIFO using these values and attaches the FIFO to both the CPU and GP. The application must allocate the memory for the graphics FIFO before calling GX_Init(). Therefore, it is not necessary to call this function unless you want to resize the default FIFO sometime after GX_Init() has been called or you are creating a new FIFO. The minimum size is 64kB defined by GX_FIFO_MINSIZE.
// This function will also set the read and write pointers for the FIFO to the base address, so ordinarily it is not necessary to call GX_InitFifoPtrs() when initializing the FIFO. In addition, This function sets the FIFO's high water mark to (size-16kB) and the low water mark to (size/2), so it is also not necessary to call GX_InitFifoLimits().

static void *frameBuffer[2] = { NULL, NULL};
GXRModeObj *rmode;

// info for PAD: https://github.com/devkitPro/libogc/blob/master/gc/ogc/pad.h 
// WPAD: https://github.com/devkitPro/libogc/blob/master/gc/wiiuse/wpad.h


uint32_t ConsoleUI::defaultKeys[]
{
    WPAD_BUTTON_2, WPAD_BUTTON_1, WPAD_BUTTON_B, WPAD_BUTTON_A, WPAD_BUTTON_MINUS, WPAD_BUTTON_HOME, WPAD_BUTTON_LEFT, WPAD_BUTTON_RIGHT, WPAD_BUTTON_DOWN, WPAD_BUTTON_UP, WPAD_BUTTON_PLUS					 
};

const char *ConsoleUI::keyNames[]
{
    "2", "1", "B", "A", "minus", "home", "left", "right", "down", "up", "plus" 
};

void ConsoleUI::startFrame(uint32_t color)
{
    // Clear the TV and gamepad screens
   GXColor background = {0, 0, 0, 0xff};
   GX_SetCopyClear(background, 0x00ffffff);
}

u32	fb = 0; 	// initial framebuffer index
void ConsoleUI::endFrame()
{
    // Finish and display a frame
GX_DrawDone();

		fb ^= 1;		// flip framebuffer
		GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
		GX_SetColorUpdate(GX_TRUE);
		GX_CopyDisp(frameBuffer[fb],GX_TRUE);

		VIDEO_SetNextFramebuffer(frameBuffer[fb]);

		VIDEO_Flush();

		VIDEO_WaitVSync();
}

/* from WiiU try nehe lesson 6 for create texture wii-example https://github.com/devkitPro/wii-examples/blob/master/graphics/gx/neheGX/lesson06/source/lesson6.c
also look at gx-sprites https://github.com/devkitPro/wii-examples/blob/master/graphics/gx/gxSprites/source/gxsprites.c

void *ConsoleUI::createTexture(uint32_t *data, int width, int height)
{
    // Create a new texture and copy data to it
    Texture *texture = new Texture(width, height);
    glGenTextures(1, &texture->tex);
    glBindTexture(GL_TEXTURE_2D, texture->tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    return texture;
}
*/

// Potential Texture Code: https://github.com/devkitPro/wii-examples/blob/38a1592e3cf3c2595d052b042058bf3179ff40de/graphics/gx/neheGX/lesson19/source/lesson19.c

GXTexObj texture; // changed GXTexObj texture; to *texture, not sure if that's ok, it seemed to work
TPLFile neheTPL; 
int NeHe_tpl, NeHe_tpl_size, nehe;
void *ConsoleUI::createTexture(uint32_t *data, int width, int height)
{
 // setup the vertex attribute table
	// describes the data args: vat location 0-7, type of data, data format, size, scale so for ex. in the first call we are sending position data with
	// 3 values X,Y,Z of size F32. scale sets the number of fractional bits for non float data.
	GX_InvVtxCache();
	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_POS,  GX_DIRECT);
	GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);

	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS,  GX_POS_XYZ,  GX_F32,   0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST,   GX_F32,   0);

	GX_SetNumChans(1); // You need this for colorized textures to work.
	GX_SetNumTexGens(1); //set number of textures to generate
 	
  	// setup texture coordinate generation
	// args: texcoord slot 0-7, matrix type, source to generate texture coordinates from, matrix to use
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);

 	// Set up TEV to paint the textures properly.
	GX_SetTevOp(GX_TEVSTAGE0,GX_MODULATE);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);

	GX_InvVtxCache();
	GX_InvalidateTexAll();

 	TPL_OpenTPLFromMemory(&neheTPL, (void *)NeHe_tpl,NeHe_tpl_size);
	TPL_GetTexture(&neheTPL,nehe,&texture);

  return &texture;
}

/*

void ConsoleUI::destroyTexture(void *texture)
{
    // Clean up a texture when it's safe to do so
    vita2d_wait_rendering_done();
    vita2d_free_texture((vita2d_texture*)texture);
}

*/

void ConsoleUI::destroyTexture(void *texture)
{
GX_DrawDone();
}

/*

void ConsoleUI::drawTexture(void *texture, float tx, float ty, float tw, float th,
    float x, float y, float w, float h, bool filter, int rotation, uint32_t color)
{
    // Convert texture coordinates to floats
    float s1 = tx / ((Texture*)texture)->width;
    float t1 = ty / ((Texture*)texture)->height;
    float s2 = (tx + tw) / ((Texture*)texture)->width;
    float t2 = (ty + th) / ((Texture*)texture)->height;

    // Convert the surface color to floats
    float r = (color >> 0) & 0xFF;
    float g = (color >> 8) & 0xFF;
    float b = (color >> 16) & 0xFF;

    // Arrange texture coordinates so they can be rotated
    float texCoords[] = { s2, t2, s1, t2, s1, t1, s2, t1 };
    int offsets[] = { 0, 6, 2 };
    int o = offsets[rotation];

    // Define vertex data to upload
    VertexData vertices[] =
    {
        VertexData(x + w, y + h, texCoords[(o + 0) & 0x7], texCoords[(o + 1) & 0x7], r, g, b),
        VertexData(x + 0, y + h, texCoords[(o + 2) & 0x7], texCoords[(o + 3) & 0x7], r, g, b),
        VertexData(x + 0, y + 0, texCoords[(o + 4) & 0x7], texCoords[(o + 5) & 0x7], r, g, b),
        VertexData(x + w, y + 0, texCoords[(o + 6) & 0x7], texCoords[(o + 7) & 0x7], r, g, b)
    };

    // Draw a texture
    glBindTexture(GL_TEXTURE_2D, ((Texture*)texture)->tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter ? GL_LINEAR : GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter ? GL_LINEAR : GL_NEAREST);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}


*/

Mtx view; // view and perspective matrices
Mtx model, modelview;
Mtx44 perspective;
void DrawFlag(Mtx view, GXTexObj texture);
// try https://github.com/devkitPro/wii-examples/blob/38a1592e3cf3c2595d052b042058bf3179ff40de/graphics/gx/gxSprites/source/gxsprites.c
// try lesson 11 instead with a single flag texture, just leave out the animation data. https://github.com/devkitPro/wii-examples/blob/38a1592e3cf3c2595d052b042058bf3179ff40de/graphics/gx/neheGX/lesson11/source/lesson11.c

void ConsoleUI::drawTexture(void *texture, float tx, float ty, float tw, float th,
    float x, float y, float w, float h, bool filter, int rotation, uint32_t color)
{
	DrawFlag(view,texture);

		GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
		GX_SetColorUpdate(GX_TRUE);
		GX_CopyDisp(frameBuffer[fb],GX_TRUE);

}

//---------------------------------------------------------------------------------
int main(int argc, char **argv) {
//---------------------------------------------------------------------------------

	f32 yscale;

	u32 xfbHeight;

	Mtx	view;
	Mtx44 perspective;

	u32	fb = 0; 	// initial framebuffer index
	GXColor background = {0, 0, 0, 0xff};


	// init the vi.
	VIDEO_Init();
	WPAD_Init();

	rmode = VIDEO_GetPreferredMode(NULL);

	// allocate 2 framebuffers for double buffering
	frameBuffer[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	frameBuffer[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(frameBuffer[fb]);
	VIDEO_SetBlack(false);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

	// setup the fifo and then init the flipper
	void *gp_fifo = NULL;
	gp_fifo = memalign(32,DEFAULT_FIFO_SIZE);
	memset(gp_fifo,0,DEFAULT_FIFO_SIZE);

	GX_Init(gp_fifo,DEFAULT_FIFO_SIZE);

	// clears the bg to color and clears the z buffer
	GX_SetCopyClear(background, 0x00ffffff);

	// other gx setup
	GX_SetViewport(0,0,rmode->fbWidth,rmode->efbHeight,0,1);
	yscale = GX_GetYScaleFactor(rmode->efbHeight,rmode->xfbHeight);
	xfbHeight = GX_SetDispCopyYScale(yscale);
	GX_SetScissor(0,0,rmode->fbWidth,rmode->efbHeight);
	GX_SetDispCopySrc(0,0,rmode->fbWidth,rmode->efbHeight);
	GX_SetDispCopyDst(rmode->fbWidth,xfbHeight);
	GX_SetCopyFilter(rmode->aa,rmode->sample_pattern,GX_TRUE,rmode->vfilter);
	GX_SetFieldMode(rmode->field_rendering,((rmode->viHeight==2*rmode->xfbHeight)?GX_ENABLE:GX_DISABLE));

	GX_SetCullMode(GX_CULL_NONE);
	GX_CopyDisp(frameBuffer[fb],GX_TRUE);
	GX_SetDispCopyGamma(GX_GM_1_0);

	// setup our camera at the origin
	// looking down the -z axis with y up
	guVector cam = {0.0F, 0.0F, 0.0F},
			up = {0.0F, 1.0F, 0.0F},
		  look = {0.0F, 0.0F, -1.0F};
	guLookAt(view, &cam, &up, &look);


	// setup our projection matrix
	// this creates a perspective matrix with a view angle of 90,
	// and aspect ratio based on the display resolution
	f32 w = rmode->viWidth;
	f32 h = rmode->viHeight;
	guPerspective(perspective, 45, (f32)w/h, 0.1F, 300.0F);
	GX_LoadProjectionMtx(perspective, GX_PERSPECTIVE);

	// The console understands VT terminal escape codes
	// This positions the cursor on row 2, column 0
	// we can use variables for this with format codes too
	// e.g. printf ("\x1b[%d;%dH", row, column );
	printf("\x1b[2;0H");


	if (!fatInitDefault()) {
		printf("fatInitDefault failure: terminating\n");
		goto error;
	}

	DIR *pdir;
	struct dirent *pent;
	struct stat statbuf;

	pdir=opendir(".");

	if (!pdir){
		printf ("opendir() failure; terminating\n");
		goto error;
	}

	while ((pent=readdir(pdir))!=NULL) {
		stat(pent->d_name,&statbuf);
		if(strcmp(".", pent->d_name) == 0 || strcmp("..", pent->d_name) == 0)
			continue;
		if(S_ISDIR(statbuf.st_mode))
			printf("%s <dir>\n", pent->d_name);
		if(!(S_ISDIR(statbuf.st_mode)))
			printf("%s %lld\n", pent->d_name, statbuf.st_size);
	}
	closedir(pdir);

error:
	while(1) {

		WPAD_ScanPads();

		if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME) exit(0);

		// do this before drawing
		GX_SetViewport(0,0,rmode->fbWidth,rmode->efbHeight,0,1);


		// do this stuff after drawing
		GX_DrawDone();

		fb ^= 1;		// flip framebuffer
		GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
		GX_SetColorUpdate(GX_TRUE);
		GX_CopyDisp(frameBuffer[fb],GX_TRUE);

		VIDEO_SetNextFramebuffer(frameBuffer[fb]);

		VIDEO_Flush();

		VIDEO_WaitVSync();

	}

	return 0;
}

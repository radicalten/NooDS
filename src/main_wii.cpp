#include <grrlib.h>
#include "console_ui.h"
#include "settings.h"

#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <wiiuse/wpad.h>

//TODO write create destory and draw texture functions for wii
/* Source: GRRLIB_texEdit.c

 * Create an empty texture in GX_TF_RGBA8 format.
 * @param width Width of the new texture to create.
 * @param height Height of the new texture to create.
 * @return A GRRLIB_texImg structure newly created.

GRRLIB_texImg*  GRRLIB_CreateEmptyTexture (const u32 width, const u32 height)
{
    return GRRLIB_CreateEmptyTextureFmt(width, height, GX_TF_RGBA8);
}

Source: GRRLIB_render.c

 * Draw a texture.
 * @param xpos Specifies the x-coordinate of the upper-left corner.
 * @param ypos Specifies the y-coordinate of the upper-left corner.
 * @param tex The texture to draw.
 * @param degrees Angle of rotation.
 * @param scaleX Specifies the x-coordinate scale. -1 could be used for flipping the texture horizontally.
 * @param scaleY Specifies the y-coordinate scale. -1 could be used for flipping the texture vertically.
 * @param color Color in RGBA format.

void  GRRLIB_DrawImg (const f32 xpos, const f32 ypos, const GRRLIB_texImg *tex, const f32 degrees, const f32 scaleX, const f32 scaleY, const u32 color) {
    GXTexObj  texObj;
    Mtx       m, m1, m2, mv;

    if (tex == NULL || tex->data == NULL)
        return;

    GX_InitTexObj(&texObj, tex->data, tex->w, tex->h,
                  tex->format, GX_CLAMP, GX_CLAMP, GX_FALSE);

    if (GRRLIB_Settings.antialias == false) {
        GX_InitTexObjLOD(&texObj, GX_NEAR, GX_NEAR,
                         0.0f, 0.0f, 0.0f, 0, 0, GX_ANISO_1);
        GX_SetCopyFilter(GX_FALSE, rmode->sample_pattern, GX_FALSE, rmode->vfilter);
    }
    else {
        GX_SetCopyFilter(rmode->aa, rmode->sample_pattern, GX_TRUE, rmode->vfilter);
    }

Source: GRRLIB_render.c
 * Call this function after drawing.

void  GRRLIB_Render (void) {
    GX_DrawDone();          // Tell the GX engine we are done drawing
    GX_InvalidateTexAll();

    fb ^= 1;  // Toggle framebuffer index

    GX_SetZMode      (GX_TRUE, GX_LEQUAL, GX_TRUE);
    GX_SetColorUpdate(GX_TRUE);
    GX_CopyDisp      (xfb[fb], GX_TRUE);

    VIDEO_SetNextFramebuffer(xfb[fb]);  // Select eXternal Frame Buffer
    VIDEO_Flush();                      // Flush video buffer to screen
    VIDEO_WaitVSync();                  // Wait for screen to update
    // Interlaced screens require two frames to update
    if (rmode->viTVMode & VI_NON_INTERLACE) {
        VIDEO_WaitVSync();
    }
}

*/

//---------------------------------------------------------------------------------
int main() {
//---------------------------------------------------------------------------------

	// Initialise the video system
	VIDEO_Init();

	// This function initialises the attached controllers
	WPAD_Init();

	// Obtain the preferred video mode from the system
	// This will correspond to the settings in the Wii menu
	rmode = VIDEO_GetPreferredMode(NULL);

	// Allocate memory for the display in the uncached region
	xfb[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

	// Initialise the console, required for printf
	console_init(xfb[0],20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
	//SYS_STDIO_Report(true);

	// Set up the video registers with the chosen mode
	VIDEO_Configure(rmode);

	// Tell the video hardware where our display memory is
	VIDEO_SetNextFramebuffer(xfb[0]);

	// Make the display visible
	VIDEO_SetBlack(false);

	// Flush the video register changes to the hardware
	VIDEO_Flush();

	// Wait for Video setup to complete
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

	// The console understands VT terminal escape codes
	// This positions the cursor on row 2, column 0
	// we can use variables for this with format codes too
	// e.g. printf ("\x1b[%d;%dH", row, column );
	printf("\x1b[2;0H");

	printf("Hello World!\n");

	
	while(1) {

	//console_ui.cpp main loop
	// Update the framebuffer and start rendering

       
	// Draw the DS top screen and bot screen
	GRRLIB_texImg *topTexture = GRRLIB_CreateEmptyTexture(256, 192);
// (const f32 xpos, const f32 ypos, const GRRLIB_texImg *tex, const f32 degrees, const f32 scaleX, const f32 scaleY, const u32 color) 
// u32 RGBA8 (255,255,255,0) -> hex #FFFFFF00 -> decimal 4294967040
// https://www.binaryhexconverter.com/hex-to-decimal-converter
                GRRLIB_DrawImg(0,0, topTexture, 0, 0, 0, 4294967040);
	GRRLIB_texImg *botTexture = GRRLIB_CreateEmptyTexture(256, 192);
                GRRLIB_DrawImg(0,192, botTexture, 0, 0, 0, 4294967040);

	// Call WPAD_ScanPads each loop, this reads the latest controller states
	WPAD_ScanPads();
	u32 pressed = WPAD_ButtonsDown(0);
	if ( pressed & WPAD_BUTTON_HOME ) exit(0);

	// Finish drawing and free textures
	GRRLIB_Render();
		
	}

	return 0;
}

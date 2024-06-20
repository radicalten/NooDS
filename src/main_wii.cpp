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


TODO: optimization of grrlib -> Delete the interlaced screen if check -> that runs every frame and is just wasting resources. 
we will run on progressive scan, and therefore can save lots of overhead by not including ANY if statements in a main loop

TODO: Fix so it can work on dolphin search VIDEO in grrlib git source: GRRLIB_core.c
GRRLIB_main.c minimum code to use GRRLIB https://github.com/GRRLIB/GRRLIB/blob/d93847e6a3e350bd1157d61cc1315d8bbff76968/examples/template/source/main.c#L13

*/

//---------------------------------------------------------------------------------
int main(int argc, char **argv) {
//---------------------------------------------------------------------------------

 	// Initialise the Graphics & Video subsystem
    	GRRLIB_Init();
	
	// Initialise the Wiimotes
    	WPAD_Init();

	while(1) {

	WPAD_ScanPads(); 
        if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME)  break;
		
	// Draw the DS top screen and bot screen
	GRRLIB_texImg *topTexture = GRRLIB_CreateEmptyTexture(256, 192);
// (const f32 xpos, const f32 ypos, const GRRLIB_texImg *tex, const f32 degrees, const f32 scaleX, const f32 scaleY, const u32 color) 
// u32 RGBA8 (255,255,255,0) -> hex #FFFFFF00 -> decimal 4294967040 -> alternative can write OxFFFFFFFF = white opaque
// made a mistake 00 in alpha is transparent, FF is opaque, see GRRLIB example https://github.com/GRRLIB/GRRLIB/blob/d93847e6a3e350bd1157d61cc1315d8bbff76968/examples/basic_drawing/source/main.c		
// https://www.binaryhexconverter.com/hex-to-decimal-converter
                GRRLIB_DrawImg(160,480, topTexture, 0, 1.25, 1.25, 0xFFFFFFFF);
	GRRLIB_texImg *botTexture = GRRLIB_CreateEmptyTexture(256, 192);
                GRRLIB_DrawImg(160,240, botTexture, 0, 1.25, 1.25, 0xFF0000FF);
		

	// Finish drawing and free textures Render the frame buffer to the TV
	GRRLIB_Render();
		
	}
	
	GRRLIB_Exit(); // Be a good boy, clear the memory allocated by GRRLIB
	exit(0);  // Use exit() to exit a program, do not use 'return' from main()
}

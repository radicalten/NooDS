#include <grrlib.h>
#include "console_ui.h"
#include "settings.h"

#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <wiiuse/wpad.h>

uint32_t framebuffer[256 * 192 * 8];
ScreenLayout layout;

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
        void *topTexture = nullptr, *botTexture = nullptr;
        bool shift = (Settings::highRes3D || Settings::screenFilter == 1);

	// Draw the DS top screen and bot screen
	topTexture = GRRLIB_CreateEmptyTexture(256, 192);
                drawTexture(topTexture, 0, 0, 256 << shift, 192 << shift, layout.topX, layout.topY,
                    layout.topWidth, layout.topHeight, Settings::screenFilter, ScreenLayout::screenRotation);
	botTexture = GRRLIB_CreateEmptyTexture(256, 192);
                drawTexture(botTexture, 0, 0, 256 << shift, 192 << shift, layout.botX, layout.botY,
                    layout.botWidth, layout.botHeight, Settings::screenFilter, ScreenLayout::screenRotation);

	// Call WPAD_ScanPads each loop, this reads the latest controller states
	WPAD_ScanPads();
	u32 pressed = WPAD_ButtonsDown(0);
	if ( pressed & WPAD_BUTTON_HOME ) exit(0);

	// Finish drawing and free textures
        if (topTexture) destroyTexture(topTexture);
        if (botTexture) destroyTexture(botTexture);

	// Wait for the next frame
		VIDEO_WaitVSync();
		
	}

	return 0;
}

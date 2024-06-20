#include <grrlib.h>
#include "console_ui.h"
#include "settings.h"

#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <wiiuse/wpad.h>

#define SCALEH(x, h) (((x) * (h)) / 720)
#define SCALE(x) SCALEH(x, uiHeight)
bool ConsoleUI::touchMode;

Core *ConsoleUI::core;
bool ConsoleUI::running;
std::string ConsoleUI::ndsPath, ConsoleUI::gbaPath;
std::string ConsoleUI::basePath, ConsoleUI::curPath;

uint32_t ConsoleUI::framebuffer[256 * 192 * 8];
ScreenLayout ConsoleUI::layout;
bool ConsoleUI::gbaMode;
bool ConsoleUI::changed;


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

void mainLoop(MenuTouch (*specialTouch)(), ScreenLayout *touchLayout){	
	
	while(1) {

	//console_ui.cpp main loop
	// Update the framebuffer and start rendering
        void *topTexture = nullptr, *botTexture = nullptr;
        bool shift = (Settings::highRes3D || Settings::screenFilter == 1);
       	core->gpu.getFrame(framebuffer, gbaMode);
        startFrame(0);

	// Draw the DS top screen and bot screen
	topTexture = createTexture(&framebuffer[0], 256 << shift, 192 << shift);
                drawTexture(topTexture, 0, 0, 256 << shift, 192 << shift, layout.topX, layout.topY,
                    layout.topWidth, layout.topHeight, Settings::screenFilter, ScreenLayout::screenRotation);
	botTexture = createTexture(&framebuffer[(256 * 192) << (shift * 2)], 256 << shift, 192 << shift);
                drawTexture(botTexture, 0, 0, 256 << shift, 192 << shift, layout.botX, layout.botY,
                    layout.botWidth, layout.botHeight, Settings::screenFilter, ScreenLayout::screenRotation);

	// Call WPAD_ScanPads each loop, this reads the latest controller states
	WPAD_ScanPads();
	u32 pressed = WPAD_ButtonsDown(0);
	if ( pressed & WPAD_BUTTON_HOME ) exit(0);

	MenuTouch touch = getInputTouch();
        if (!touch.pressed && specialTouch)
            touch = (*specialTouch)();

		if (touch.pressed)
        {
            // Determine the touch position relative to the emulated touch screen
            ScreenLayout *sl = touchLayout ? touchLayout : &layout;
            int touchX = sl->getTouchX(SCALEH(touch.x, sl->winHeight), SCALEH(touch.y, sl->winHeight));
            int touchY = sl->getTouchY(SCALEH(touch.x, sl->winHeight), SCALEH(touch.y, sl->winHeight));

            // Send the touch coordinates to the core
            core->input.pressScreen();
            core->spi.setTouch(touchX, touchY);
        }
        else // Released
        {
            // Release the touch screen press
            core->input.releaseScreen();
            core->spi.clearTouch();
        }

	// Finish drawing and free textures
        endFrame();
        if (topTexture) destroyTexture(topTexture);
        if (botTexture) destroyTexture(botTexture);

	// Wait for the next frame
		VIDEO_WaitVSync();
		
	}
}

	return 0;
}

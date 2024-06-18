#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <wiiuse/wpad.h>

#include "console_ui.h"
#include "settings.h"
#include "defines.h"
#include <wx/rawbmp.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

// potential bmp on wii https://github.com/bastiro03/WiiBrowser-Lite/blob/5484d0bfadfd280a1f105cbdc3ac9ce9e59a4c34/mplayer/ffmpeg/libavcodec/bmp.c

uint32_t framebuffer[256 * 192 * 8];
ScreenLayout layout;
int bmp = 0;
int width = U8TO32(bmp, 0x12);
int height = U8TO32(bmp, 0x16);
int x = 0;
int y = 0;
int w = 0;
int h = 0;
// wxImage img = bmp.ConvertToImage();
// bmp = wxBitmap(img);
uint32_t *data = new uint32_t[width * height];
void *texture = createTexture(data, width, height);
drawTexture(texture, 0, 0, 1, 1, x, y, w, h, false, 0, color);


void *ConsoleUI::bmpToTexture(uint8_t *bmp)
{
    // Allocate data based on bitmap measurements
    int width = U8TO32(bmp, 0x12);
    int height = U8TO32(bmp, 0x16);
    uint32_t *data = new uint32_t[width * height];

    // Convert the bitmap to RGBA8 texture data
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            uint8_t *color = &bmp[0x46 + (((height - y - 1) * width + x) << 2)];
            data[y * width + x] = (color[3] << 24) | (color[0] << 16) | (color[1] << 8) | color[2];
        }
    }

    // Create a texture from the data
    void *texture = createTexture(data, width, height);
    delete[] data;
    return texture;
}

void drawRectangle(float x, float y, float w, float h, uint32_t color)
{
    // Draw a rectangle using a blank texture
    static uint32_t data = 0xFFFFFFFF;
    static void *texture = createTexture(&data, 1, 1);
    drawTexture(texture, 0, 0, 1, 1, x, y, w, h, false, 0, color);
}


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
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

	// Initialise the console, required for printf
	console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
	//SYS_STDIO_Report(true);

	// Set up the video registers with the chosen mode
	VIDEO_Configure(rmode);

	// Tell the video hardware where our display memory is
	VIDEO_SetNextFramebuffer(xfb);

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

	// Update the framebuffer and start rendering
        void *topTexture = nullptr, *botTexture = nullptr;
	bool shift = (Settings::highRes3D || Settings::screenFilter == 1);
	
	// Draw the DS top screen
	topTexture = createTexture(&framebuffer[0], 256 << shift, 192 << shift);
                drawTexture(topTexture, 0, 0, 256 << shift, 192 << shift, layout.topX, layout.topY,
                    layout.topWidth, layout.topHeight, Settings::screenFilter, ScreenLayout::screenRotation);

	// Draw the DS bottom screen
	botTexture = createTexture(&framebuffer[(256 * 192) << (shift * 2)], 256 << shift, 192 << shift);
                drawTexture(botTexture, 0, 0, 256 << shift, 192 << shift, layout.botX, layout.botY,
                    layout.botWidth, layout.botHeight, Settings::screenFilter, ScreenLayout::screenRotation);
	
	while(1) {

		// Call WPAD_ScanPads each loop, this reads the latest controller states
		WPAD_ScanPads();

		// WPAD_ButtonsDown tells us which buttons were pressed in this loop
		// this is a "one shot" state which will not fire again until the button has been released
		u32 pressed = WPAD_ButtonsDown(0);

		// We return to the launcher application via exit
		if ( pressed & WPAD_BUTTON_HOME ) exit(0);

		// Wait for the next frame
		VIDEO_WaitVSync();
	}

	return 0;
}

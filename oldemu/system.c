//*******************************************************************************************************
//*******************************************************************************************************
//
//      Name:       System.C
//      Purpose:    Main Interface Layer to SDL
//      Author:     Paul Robson
//      Date:       21st February 2014
//
//*******************************************************************************************************
//*******************************************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <SDL2/SDL.h>
#include "general.h"
#include "system.h"
#include "debugger.h"
#include "core.h"

#define W_WIDTH         (1024)                                                          // Window Width and Height.
#define W_HEIGHT        (768)

static SDL_Window *window;                                                              // Window reference.
static SDL_Renderer* renderer;                                                          // Accelerated Window renderer.
static DisplaySurface fontSurface;                                                      // The imported 2513 font as a display surface.
static DisplaySurface drawSurface;                                                      // The text screen surface.

static int currentKey;                                                                  // Pending key press
static BYTE8 isKeyPressed[512];                                                         // State of scan codes.

#define ISSHIFTPRESSED()  (SYSIsKeyPressed(SDLK_LSHIFT) || SYSIsKeyPressed(SDLK_RSHIFT))// Shift/Control macros
#define ISCTRLPRESSED()   (SYSIsKeyPressed(SDLK_LCTRL) || SYSIsKeyPressed(SDLK_RCTRL))    

static void SYSInitialise(void);                                                        // Forward references.
static void SYSTerminate(void);
static void SYSCreateFontSurface(void);
static BYTE8 SYSReadFont2513(char ch,int row);
static void SYSProcessKey(int keyCode,BOOL isShiftPressed,BOOL isControlPressed);
static int  SYSMapSDLK(int sdlkCode);

//*******************************************************************************************************
//                                Main Rendering / Key checking loop
//*******************************************************************************************************

void SYSRun(int argc,char *argv[]) {
    int key;
    SDL_Event event;
    BOOL quit = FALSE;
    SYSInitialise();                                                                    // Set up windows etc.   
    CPUInitialise(argc,argv);                                                           // Set up CPU

    while (!quit) {                                                                     // Keep going until finished.
        while(SDL_PollEvent(&event)) {                                                  // Process the event queue.
        
        if (event.type == SDL_KEYUP || event.type == SDL_KEYDOWN) {                     // Is it a key event
            key = event.key.keysym.sym;                                                 // This is the SDL Key Code
            isKeyPressed[SYSMapSDLK(key)] = (event.type == SDL_KEYDOWN);                // Track key states.
            if (key == SDLK_ESCAPE) quit = TRUE;                                        // Exit on shift escape pressed.
            if (event.type == SDL_KEYDOWN)                                              // Pass all key presses onto the key handler.
                        SYSProcessKey(key,ISSHIFTPRESSED(),ISCTRLPRESSED());
            }
        }
        SDL_RenderClear(renderer);                                                      // Clear the renderer.

        DBGRenderDisplay(&drawSurface);                                                 // Ask the debugger to update the display
        SYSUpdateSurface(&drawSurface);                                                 // Update text screen and render it.
        SYSRenderSurface(&drawSurface);
        DBGProcessFrame();                                                              // Process the frame.
        SDL_RenderPresent(renderer);                                                    // Render the current screen.                                            
    }
    SYSTerminate();
}

//*******************************************************************************************************
//                              Set up the SDL Interface Layer
//*******************************************************************************************************

static void SYSInitialise(void) {
    char buffer[64];
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO)<0)                                      // Initialise SDL
        exit(fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError() ));
    atexit(SYSTerminate);                                                              // Call terminate on the way out.
    sprintf(buffer,"%s Emulator : Build %d",EMUNAME,BUILD);
    window = SDL_CreateWindow(buffer, SDL_WINDOWPOS_UNDEFINED,                          // Create main window and renderer.
                                            SDL_WINDOWPOS_UNDEFINED, W_WIDTH, W_HEIGHT,0);
    renderer = SDL_CreateRenderer(window, -1, 0);    
    SYSCreateFontSurface();                                                             // Create the font surface.
    SYSCreateSurface(&drawSurface,W_WIDTH,W_HEIGHT);                                    // Create the drawing surface
    CPUInitialiseResources();                                                           // Initialise any surfaces needed.
    CPUReset();                                                                         // Reset the processor.
}

//*******************************************************************************************************
//                                  Tidy up SDL and Close it Down
//*******************************************************************************************************

static void SYSTerminate(void) {
    SDL_DestroyRenderer(renderer);                                                      // Destroy objects and quit.
    SDL_DestroyWindow(window);
    SDL_Quit();
}

//*******************************************************************************************************
//                           Convert keystrokes to typical ASCII keyboard
//*******************************************************************************************************

static void SYSProcessKey(int keyCode,BOOL isShiftPressed,BOOL isControlPressed) {
    int asciiKey = -1;
    if (keyCode == 167) keyCode = '@';                                                  // BackTick key for @
    if (keyCode == '\'') keyCode = ':';                                                 // Single Quote for Colon.

    if (isalpha(keyCode)) {                                                             // Handle A-Z key strokes.
        asciiKey = toupper(keyCode);                                                    // Basic keystrokes (U/C)
        if (isShiftPressed) asciiKey = tolower(asciiKey);                               // Shift gives L/C e.g. Shift Lock on.
        if (isControlPressed) asciiKey &= 0x1F;                                         // Control gives lower range.
    } else if (isdigit(keyCode)) {                                                      // Handle 0-9 key strokes.
        asciiKey = keyCode;                                                             // Return the key code.
        if (isShiftPressed) asciiKey = asciiKey - 16;                                   // Adjust for the shifted (! " # $ % & ' ( ))
    } else if (keyCode == SDLK_RETURN || keyCode == ' ' || keyCode == SDLK_ESCAPE ||    // Handle those which are completely stand alone.
               keyCode == SDLK_TAB) {
        asciiKey = keyCode;
    } else if (keyCode == '@' || keyCode == ':' || keyCode == '.' || keyCode == '\\' || // Shiftable punctuation characters.
               keyCode == '-' || keyCode == '[' || keyCode == ']' || keyCode == '/' ||
               keyCode == ',' || keyCode == ';') {
        asciiKey = keyCode;
        if (isShiftPressed) {                                                           // If shift pressed, change ASCII code.
            if (asciiKey >= 64+27) asciiKey += 32; 
            else asciiKey += (asciiKey < 48) ? 16 : -16;
        }
    } else if (keyCode == SDLK_BACKSPACE) asciiKey = 'H' & 0x1F;                        // Backspace generates CTRL+H
    else asciiKey = CPUScanCodeToASCIICheck(keyCode);                                   // Finally, anything specific.
    if (asciiKey > 0) currentKey = asciiKey;                                            // Put in 'buffer' if a legal key.
}

//*******************************************************************************************************
//          Map SDLK onto a useable array index (see wiki.libsdl.org/SDLKeycodeLookup)
//*******************************************************************************************************

static int SYSMapSDLK(int sdlkCode) {
    if (sdlkCode < 0x100) return sdlkCode;                                              // 00-FF map to 00-FF
    sdlkCode = sdlkCode & 0xFFFF;                                                       // look at lower 16 bits
    return (sdlkCode > 0xFF) ? 0 : sdlkCode | 0x100;                                    // return 100-1FF if lower 16 bits 00-FF
}

//*******************************************************************************************************
//                                            Check key state
//*******************************************************************************************************

BYTE8 SYSIsKeyPressed(int sdlkCode) {
    sdlkCode = SYSMapSDLK(sdlkCode);                                                    // map to array index
    return (sdlkCode == 0) ? 0 : isKeyPressed[sdlkCode];                                // return state or zero if no mapping.
}

//*******************************************************************************************************
//                                 Check if keyboard buffer empty
//*******************************************************************************************************

BOOL SYSIsKeyAvailable(void) {
    return currentKey != 0;
}

//*******************************************************************************************************
//                                   Get key from keyboard buffer
//*******************************************************************************************************

BYTE8 SYSReadKeyboard(void) {
    BYTE8 r = currentKey;                                                               // save key
    currentKey = 0;                                                                     // buffer empty
    return r;
}

//*******************************************************************************************************
//                                  Break to monitor (Windows/CMD key)
//*******************************************************************************************************

BOOL SYSIsMonitorBreak(void) {
    return SYSIsKeyPressed(SDLK_LGUI);
}

//*******************************************************************************************************
//                                 Create a surface and renderer
//*******************************************************************************************************

void SYSCreateSurface(DisplaySurface *surface,int width,int height) {
    Uint32 rmask, gmask, bmask, amask;                                                  // Sort out the display masks.
    #if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;gmask = 0x00ff0000;bmask = 0x0000ff00;amask = 0x000000ff;
    #else
    rmask = 0x000000ff;gmask = 0x0000ff00;bmask = 0x00ff0000;amask = 0xff000000;
    #endif
    surface->surface = SDL_CreateRGBSurface(0,width,height,32,rmask,gmask,bmask,amask); // Create a surface of the required size.
    surface->texture = SDL_CreateTextureFromSurface(renderer,surface->surface);         // Create a texture for the surface.
    SDL_FillRect(surface->surface,NULL,SDL_MapRGB(surface->surface->format,0,0,0));     // Fill it with black.
    surface->width = width;surface->height = height;                                    // Save size.
}

//*******************************************************************************************************
//                          Update Surface's texture if changed/blitted etc.
//*******************************************************************************************************

void SYSUpdateSurface(DisplaySurface *surface) {
    SDL_UpdateTexture(surface->texture,NULL,surface->surface->pixels,surface->surface->pitch);
}

//*******************************************************************************************************
//                           Render the surface as the main display element
//*******************************************************************************************************

void SYSRenderSurface(DisplaySurface *surface) {
    SDL_RenderCopy(renderer,surface->texture,NULL,NULL);
}

//*******************************************************************************************************
//                                  Write character on text display
//*******************************************************************************************************

void SYSWriteCharacter(int x,int y,char ch,int colour) {
    if (x < 0 || y < 0 || x >= T_WIDTH || y >= T_HEIGHT) return;                        // Check coordinate range
    ch = toupper(ch) & 0x3F;colour = colour & 7;                                        // Mask character and colour.
    SDL_Rect rcSrc,rcTgt;
    rcSrc.w = 6;rcSrc.h = 8;rcSrc.x = (ch & 0x3F) * 6;rcSrc.y = colour*8;
    rcTgt.w = W_WIDTH / T_WIDTH; rcTgt.h = W_HEIGHT / T_HEIGHT;
    rcTgt.x = rcTgt.w * x;rcTgt.y = rcTgt.h * y;
    SDL_BlitScaled(fontSurface.surface,&rcSrc,drawSurface.surface,&rcTgt);
}

//*******************************************************************************************************
//                                          Clear Text Screen
//*******************************************************************************************************

void SYSClearScreen(void) {
    SDL_FillRect(drawSurface.surface,NULL,SDL_MapRGB(drawSurface.surface->format,0,0,0));
}

//*******************************************************************************************************
//                                          Write a string
//*******************************************************************************************************

void SYSWriteString(int x,int y,char *s,int colour) {
    while (x < T_WIDTH && *s != '\0') SYSWriteCharacter(x++,y,*s++,colour);
}

//*******************************************************************************************************
//                                    Write a hexadecimal value
//*******************************************************************************************************

void SYSWriteHex(int x,int y,int n,int colour,int width) {
    char buffer[32];
    sprintf(buffer,"%0*x",width,n);
    SYSWriteString(x,y,buffer,colour);
}

//*******************************************************************************************************
//                           Create the 5x7 64 character font surface
//*******************************************************************************************************

static void SYSCreateFontSurface(void) {
    SDL_Rect rc;rc.w = rc.h = 1;
    SYSCreateSurface(&fontSurface,64*6,8*8);
    for (int y = 0;y < 8 * 8;y++) {
        rc.y = y;                                                                   
        int colour = y / 8;                                                             // Get colour.
        Uint32 uColour = SDL_MapRGB(fontSurface.surface->format,                        // Convert to RGB colour
                    (colour & 4) ? 255 : 0,(colour & 2) ? 255 : 0,(colour & 1) ? 255 : 0);
        if (colour == 0) uColour = SDL_MapRGB(fontSurface.surface->format,0,165,255);   // Colour 0 (Black) is orange - we don't want a black on black font :)
        for (int x = 0;x < 64*6;x++) {
            if (y % 8 != 7 && SYSReadFont2513(x / 6,y % 8) & (0x80 >> (x % 6))) {
                rc.x = x;SDL_FillRect(fontSurface.surface,&rc,uColour);
            }
        }
    }
    SYSUpdateSurface(&fontSurface);                                                    // Update it as changed - can be used from now on, no further updating.
}

//*******************************************************************************************************
//                          Access the internal 64 character font.
//*******************************************************************************************************

DisplaySurface *SYSGetFontSurface(void) {
    return &fontSurface;
}

//*******************************************************************************************************
//                  Get Tick Timer - needs about a 20Hz minimum granularity.
//*******************************************************************************************************

int SYSGetTime(void)
{
    return SDL_GetTicks();
}

//*******************************************************************************************************
//                                  Synchronise to a master clock
//*******************************************************************************************************

static int lastSync = 0;

void SYSSynchronise(int frequency) {
    int next = 1000/frequency + lastSync;                                                   // Time of next sync
    int now = SYSGetTime();
    while (SYSGetTime() < next) {  }                                                        // Wait for it
    lastSync = SYSGetTime();                                                                // Update last sync.
}

//*******************************************************************************************************
//         5 x 7 Font Data. Note it is 64 first row, 64 second row etc. rather than usual order  
//*******************************************************************************************************

static BYTE8 SYSFont2513[64*7] = {                                                         // From 2513 Datasheet
        112,32,240,112,240,248,248,120,136,112,8,136,128,136,136,112,
        240,112,240,112,248,136,136,136,136,136,248,248,0,248,0,0,
        0,32,80,80,32,192,64,32,32,32,32,0,0,0,0,0,
        112,32,112,248,16,248,56,248,112,112,0,0,16,0,64,112,
        136,80,136,136,136,128,128,128,136,32,8,144,128,216,136,136,
        136,136,136,136,32,136,136,136,136,136,8,192,128,24,0,0,
        0,32,80,80,120,200,160,32,64,16,168,32,0,0,0,8,
        136,96,136,8,48,128,64,8,136,136,0,0,32,0,32,136,
        168,136,136,128,136,128,128,128,136,32,8,160,128,168,200,136,
        136,136,136,128,32,136,136,136,80,80,16,192,64,24,32,0,
        0,32,80,248,160,16,160,32,128,8,112,32,0,0,0,16,
        152,32,8,16,80,240,128,16,136,136,32,32,64,248,16,16,
        184,136,240,128,136,240,240,128,248,32,8,192,128,168,168,136,
        240,136,240,112,32,136,136,168,32,32,32,192,32,24,80,0,
        0,32,0,80,112,32,64,0,128,8,32,248,0,248,0,32,
        168,32,48,48,144,8,240,32,112,120,0,0,128,0,8,32,
        176,248,136,128,136,128,128,152,136,32,8,160,128,136,152,136,
        128,168,160,8,32,136,136,168,80,32,64,192,16,24,136,0,
        0,32,0,248,40,64,168,0,128,8,112,32,32,0,0,64,
        200,32,64,8,248,8,136,64,136,8,32,32,64,248,16,32,
        128,136,136,136,136,128,128,136,136,32,136,144,128,136,136,136,
        128,144,144,136,32,136,80,216,136,32,128,192,8,24,0,0,
        0,0,0,80,240,152,144,0,64,16,168,32,32,0,0,128,
        136,32,128,136,16,136,136,64,136,16,0,32,32,0,32,0,
        120,136,240,112,240,248,128,120,136,112,112,136,248,136,136,112,
        128,104,136,112,32,112,32,136,136,32,248,248,0,248,0,248,
        0,32,0,80,32,24,104,0,32,32,32,0,64,0,32,0,
        112,112,248,112,16,112,112,64,112,224,0,64,16,0,64,32 };

static BYTE8 SYSReadFont2513(char ch,int row) {
    return SYSFont2513[ch + row * 64];
}


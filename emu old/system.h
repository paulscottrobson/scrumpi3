//*******************************************************************************************************
//*******************************************************************************************************
//
//      Name:       System.H
//      Purpose:    Include file, SDL Interface
//      Author:     Paul Robson
//      Date:      	21st February 2014
//
//*******************************************************************************************************
//*******************************************************************************************************

#ifndef _SYSTEM_H

#include <SDL2/SDL.h>

typedef struct _DisplaySurface {														// Display Surface structure.
    SDL_Surface *surface;                                                               // The surface
    SDL_Texture *texture;                                                               // The associated texture.
    int width,height;																	// Size
} DisplaySurface;

void SYSCreateSurface(DisplaySurface *surface,int width,int height);
void SYSUpdateSurface(DisplaySurface *surface);
void SYSRenderSurface(DisplaySurface *surface);
DisplaySurface *SYSGetFontSurface(void);

void SYSWriteCharacter(int x,int y,char ch,int colour);
void SYSRun(int argc,char *argv[]);
void SYSClearScreen(void);
void SYSWriteString(int x,int y,char *s,int colour);
void SYSWriteHex(int x,int y,int n,int colour,int width);
int  SYSGetTime(void);
void SYSSynchronise(int frequency);
BOOL SYSIsMonitorBreak(void);
BOOL SYSIsKeyAvailable(void);
BYTE8 SYSReadKeyboard(void);
BYTE8 SYSIsKeyPressed(int sdlkCode);

#endif


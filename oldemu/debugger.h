//*******************************************************************************************************
//*******************************************************************************************************
//
//      Name:       Debugger.H
//      Purpose:    Debugger Header
//      Author:     Paul Robson
//      Date:       8th February 2014
//
//*******************************************************************************************************
//*******************************************************************************************************

#ifndef _DEBUGGER_H
#define _DEBUGGER_H

WORD16 	DBGGetCurrentBreakpoint(void);
void 	DBGSetCodeAddress(WORD16 codeAddress);
void	DBGRenderDisplay(DisplaySurface *drawSurface);
void DBGProcessFrame(void);

void DBGUtilsLabels(int x,int y,char *items[],int colour);
void DBGUtilsDisassemble(int x,int y1,int y2,int address,int programCounter);
void DBGUtilsDisplay(int x,int y1,int y2,int address,int bytesPerLine,int formatWidth,int showASCII,int addressHighlight);

#endif

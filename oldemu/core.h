//*******************************************************************************************************
//*******************************************************************************************************
//
//      Name:       Core.H (6800)
//      Purpose:    CPU Core Header
//      Author:     Paul Robson
//      Date:       8th February 2014
//
//*******************************************************************************************************
//*******************************************************************************************************

#ifndef _CORE_H
#define _CORE_H

#ifdef DEBUGGABLE

#define EMUNAME 			"WallPaper One"
#define PREINCREMENT
#define T_WIDTH 	(40)
#define T_HEIGHT 	(25)

typedef struct _CPUStatus {
	BYTE8 a,e,s;
	WORD16 pc,p0,p1,p2,p3;
	int (*readCodeMemory)(int address);
	int (*readDataMemory)(int address);
} CPUSTATUS;

CPUSTATUS *CPUGetStatus(CPUSTATUS *s);
int CPUGetInstructionInfo(int address,char *buffer);
int CPUGetCodeAddressMask(void);
int CPUGetDataAddressMask(void);
void CPURenderDebug(SDL_Rect *rc,int codeAddress,int dataAddress);
DisplaySurface *CPUGetDisplaySurface(void);
void CPUInitialiseResources(void);
int CPUScanCodeToASCIICheck(int scanCode);
void CPUInitialise(int argc,char *argv[]);

#endif

#define RUN_FOREVER 	(0xAA) 				// Modes for running processor.
#define SINGLE_STEP 	(0x01)
#define RUN_FRAME 		(0x00)

void CPUReset(void);
BYTE8 CPUExecute(BYTE8 singleStep);

void CPUXDelay(WORD16 ms);
void CPUXWriteScreen(BYTE8 x,BYTE8 y,BYTE8 ch);
void CPUXSynchronise(WORD16 frequency);
BYTE8 CPUXReadKey(BYTE8 key);

#define CPUX_INT			(0x20)			// INT Key (connected to SA)
#define CPUX_ALPHA1			(0x31)			// Shift Mode keys
#define CPUX_ALPHA2			(0x32)
#define CPUX_PUNC 			(0x33)
#define CPUX_USER			(0x40)			// Unlabelled key 21.

void CPUWriteUART(BYTE8 offset,BYTE8 data);
BYTE8 CPUReadUART(BYTE8 offset);

void CPUWrite8154(BYTE8 offset,BYTE8 data);
BYTE8 CPURead8154(BYTE8 offset);

#endif

//*******************************************************************************************************
//*******************************************************************************************************
//
//      Name:       Core.C (SC/MP Scrumpi 3)
//      Purpose:    CPU Core
//      Author:     Paul Robson
//      Date:       14th February 2014
//
//*******************************************************************************************************
//*******************************************************************************************************

#include <stdlib.h>
#include <stdio.h>
#include "general.h"
#include "system.h"
#include "debugger.h"
#include "core.h"

#define SHADOW_P0

#ifdef ARDUINO 																		// Arduino mode.
#define FAST_ADD 		 															// Use fast addressing.
#define NO_CYCLES 																	// Don't count cycles.
#endif

static void writeMemory(WORD16 addr,BYTE8 data);									// Forward definitions for R/W
static BYTE8 readMemory(WORD16 addr);
static void longDelay(BYTE8 operand,BYTE8 accumulator);
static void setInversion(BYTE8 newInversion);
static void updateShadow(WORD16 p0);

#define RAMSIZE 	(1024)															// Expansion RAM size.

static WORD16 cycleCount; 															// Cycle Count (in this frame)

static BYTE8 expansionRAMMemory[RAMSIZE+2];											// 1k Expansion RAM Memory (800-BFF)
static BYTE8 videoMemory[256]; 														// 256 bytes Video Memory (E00-EFF)
static BYTE8 ins8154RAMMemory[128];													// 128 bytes standard memory (F80-FFF)
static BYTE8 screenInverted = 0;													// Is the screen inverted ?
static BYTE8 interruptCounter = 0; 													// 8 bit counter, tests interrupt every 256 instructions.

static BYTE8 *shadowP0; 															// Shadow value of P0, points to next instruction (one ahead)

#define CYCLESPERSECOND 	(3510000L/2)											// Clock Speed (3.51Mhz, 1 Cycles is 2 Clocks)
#define FRAMERATE 			(50L) 													// Frame rate (RTC tick rate)
#define CYCLESPERFRAME 		((WORD16)(CYCLESPERSECOND/FRAMERATE))					// CPU Cycles per frame.
#define INSTRUCTIONSPERSEC 	(CYCLESPERSECOND/10L) 									// Instructions per second, roughly.
#define INSTRPEREMUFRAME 	(INSTRUCTIONSPERSEC/30) 								// About 30 refreshes a second.

#define branch(n) { p0 = (n); updateShadow(p0); }									// Anything involving PC change (except increments)

#define readByte(n) 	readMemory(n) 												// Mem Read/Write Macros
#define writeByte(n,d)  writeMemory(n,d)

#define writeIOFlags(v)   { if ((v & 2) != screenInverted) setInversion(v & 2); }	// Set F0/F1/F2
#define readSenseLines() (CPUXReadKey(CPUX_INT) ? 1 : 0)							// Read SA/SB

#ifdef FAST_ADD 																	// Use fast or slow EAC addition
#define addAddress(a,b) 	((a)+(b)) 												// Fast   : just add the address and offset
#else
#define addAddress(a,b) 	(((a) & 0xF000) | (((a)+(b)) & 0xFFF)) 					// Correct: add the two but keep the upper 
																					// 4 bits of the address unchanged
#endif

#include "scmp_autocode.h"															// This is the system generated code 
#include "scrumpi3_bios.h"															// This is the Scrumpi 3 BIOS 

//*******************************************************************************************************
//									Program Loading etc.
//*******************************************************************************************************

void CPUInitialise(int argc,char *argv[]) {
/*
	if (argc == 2) {
		FILE *f = fopen(argv[1],"rb");
		if (f == NULL) exit(fprintf(stderr,"Can't open binary"));
		fread(romMemory,1,2048,f);
		fclose(f);
	}
*/
}

//*******************************************************************************************************
//						Changed P0. Update the shadow pointer accordingly.
//*******************************************************************************************************

static void updateShadow(WORD16 p0) {
	#ifdef SHADOW_P0 																// Empty function if shadowing not enabled !

	p0 = p0 & 0xFFF; 																// 12 bit address.
	p0++;																			// The shadow pointer works ahead e.g. it points to next instruction.

	if (p0 < 0x800) { 																// Running from ROM.
		shadowP0 = _biosROM+p0; 												
	} else if (p0 >= 0x800 && p0 < 0x800+RAMSIZE) { 								// Running from main RAM.
		shadowP0 = expansionRAMMemory+(p0 & 0x3FF);
	} else if (p0 >= 0xF80) { 														// Running from NS8154 RAM.
		shadowP0 = ins8154RAMMemory+(p0 & 0x7F); 
	} else { 																		// Could run code in Video RAM in theory :)
		#ifdef ARDUINO
		TODO: DO something !!!!
		#else
		fprintf(stderr,"Tried to run code at %04x. No code.\n",p0);
		exit(-1); 		
		#endif
	}
	#endif
}

//*******************************************************************************************************
//										   Memory Read
//*******************************************************************************************************

static BYTE8 readMemory(WORD16 addr) {
	//if ((addr & 0xFFF) > 0x40 && p0 > 0x20 && (addr & 0xF000) != 0x7000) printf("Read at %x %x\n",p0,addr);
	addr &= 0xFFF;																	// 12 bit address.
	if (addr < 0x800) {																// ROM memory (000-7FF)
		return _biosROM[addr];										
	}
	if (addr < 0x800+RAMSIZE) return expansionRAMMemory[addr & 0x3FF];				// 1k Extended RAM (800-BFF)
	if (addr >= 0xF80) return ins8154RAMMemory[addr & 0x7F];						// 128 bytes standard RAM (F80-FFF)
	if ((addr >> 8) == 0xE) return videoMemory[addr & 0xFF];						// 256 Bytes Video RAM (E00-EFF)

	if ((addr >> 8) == 0xC) {														// Keyboard scan.
		BYTE8 retVal = 0;
		if (CPUXReadKey(CPUX_USER)) retVal |= 0x80;									// Bit 7 User
		if (CPUXReadKey(CPUX_PUNC)) retVal |= 0x40;									// Bit 6 Punc
		if (CPUXReadKey(CPUX_ALPHA2)) retVal |= 0x20;								// Bit 5 Alpha 2
		if (CPUXReadKey(CPUX_ALPHA1)) retVal |= 0x10;								// Bit 4 Alpha 1
		BYTE8 row,column;
		for (row = 0;row < 4;row++) {												// Scan rows
			if (addr & (0x08 >> row)) {												// Row bit set ?
				for (column = 0;column < 4;column++)								// Scan each column
					if (CPUXReadKey(column + row * 4)) 								// If key pressed.
						retVal |= (0x08 >> column);									// seet corresponding column bit.
			}
		}
		return retVal;
	}
	if ((addr >> 8) == 0xD) return CPUReadUART(addr & 0xFF);						// UART (D00-DFF)
	if ((addr >> 7) == 0x1E) return CPURead8154(addr & 0x7F);						// NS8154 (F00-F7F)
	return 0x00;																	// Return float low.
}

//*******************************************************************************************************
//									Write to Memory / IO Devices
//*******************************************************************************************************

static void writeMemory(WORD16 addr,BYTE8 data) {
	addr &= 0xFFF;																	// 12 bit address.
	if (addr >= 0xF80) { ins8154RAMMemory[addr & 0x7F] = data; return; }			// F80-FFF Standard RAM (128 bytes)

	if ((addr >> 10) == 2) { 													 	// 800-BFF Expansion RAM (1k bytes)
		addr &= 0x3FF;
		if (addr < RAMSIZE) expansionRAMMemory[addr] = data; 
		return; 
	}

	if ((addr >> 8) == 0xE) {														// Write to VRAM (E00-EFF)
		addr = addr & 0xFF;
		if (videoMemory[addr] != data) {											// Has it changed ?
			videoMemory[addr] = data;
			if (screenInverted) data ^= 0x80;										// Is the screen inverted ?
			CPUXWriteScreen(addr & 0x1F,addr >> 5,data);							// Write to video screen.
		}
		return;
	}
	if ((addr >> 8) == 0xD) { CPUWriteUART(addr & 0xFF,data); return; }				// D00-DFF UART
	if ((addr >> 7) == 0x1E) { CPUWrite8154(addr & 0x7F,data); return; }			// F00-F7F NS8154
}

//*******************************************************************************************************
//							Deal with the screen inversion flag
//*******************************************************************************************************

static void setInversion(BYTE8 newInversion) {
	screenInverted = newInversion; 													// Set the inversion flag.
	WORD16 a;
	BYTE8 v;
	for (a = 0x7E00;a < 0x7F00;a++) { 												// Work through the whole screen
		v = videoMemory[a & 0xFF];videoMemory[a & 0xFF] = v ^ 1; 					// Read current byte and make it so write will change it
		writeMemory(a,v); 															// Write it back.
	}
}

//*******************************************************************************************************
//										Long Delay. Approximate !
//*******************************************************************************************************

static void longDelay(BYTE8 operand,BYTE8 accumulator) {
	LONG32 delayCycles = 13 + 2 * accumulator + 2 * operand + 512 * operand; 		// Total number of cycles delaying.
	WORD16 remaining = CYCLESPERFRAME - cycleCount;									// No of cycles remaining before synchronise
	WORD16 toRemove = (delayCycles < remaining) ? delayCycles : remaining;			// We lose the smaller of no of delay cycles and what's left this frame.
	cycleCount += toRemove;
	delayCycles -= toRemove;
	WORD16 timeInMs = 1000L * delayCycles / CYCLESPERSECOND; 						// Time in seconds = delay Cycles / Cycles per Second
																					// So 1000 times this is a time in milliseconds.
	if (timeInMs) CPUXDelay(timeInMs);												// Delay this many milliseconds (at most, 1/10 second)																					// This cannot be used for cycle counting for things like cassette I/O
}

//*******************************************************************************************************
//											Reset the Processor
//*******************************************************************************************************


void CPUReset(void) {
	WORD16 i;
	resetProcessor(); 																// Resets the CPU
	expansionRAMMemory[RAMSIZE] = 0x90;												// If PC is pointer shadowed, this stops it running
	expansionRAMMemory[RAMSIZE+1] = 0xFE;											// off the end of physical memory.
}

//*******************************************************************************************************
//								Execute one or more instructions on the TMS1100
//*******************************************************************************************************

#ifdef NO_CYCLES 																	// Are we counting cycles ?
#define addCycles(n) {} 												
#else 																			
#define addCycles(n) cycleCount += (n)
#endif

BYTE8 CPUExecute(BYTE8 singleStep) {
	WORD16 count = (singleStep != 0) ? 1 : INSTRPEREMUFRAME;						// How many to do ?
	WORD16 breakPoint = DBGGetCurrentBreakpoint();									// Get the current breakpoint.
	BYTE8 runForever = (singleStep == RUN_FOREVER);									// Run forever flag.
	do {
		BYTE8 opcode,operand; 														// Opcode and operand are fetched here, not in code execution.

		#ifdef SHADOW_P0															// Using the shadow pointer
		opcode = *shadowP0++;p0++; 													// Fetch opcode									
		if (opcode & 0x80) { operand = *shadowP0++;p0++; }							// Fetch operand if appropriate.
		#else 																		// Use normal memory read/write.
		opcode = readMemory(++p0);													// Fetch opcode.
		if (opcode & 0x80) operand = readMemory(++p0);								// Fetch operand if there is one. (opcode $80-$FF)
		#endif

		switch(opcode) {															// Execute operation code
			#include "scmp_opcodes.h"
		}

		#ifndef NO_CYCLES 															// If cycle counting, sync the CPU with a real clock.
		if (cycleCount > CYCLESPERFRAME) { 											// Cycle time out
			cycleCount -= CYCLESPERFRAME;											// Reset it.
			CPUXSynchronise(FRAMERATE);												// Synchronise it to real speed.
		}
		#endif

		if (interruptCounter-- == 0) { 												// Every 256 instructions check for interrupts.
			if ((s & 0x08) != 0 && CPUXReadKey(CPUX_INT) != 0) {					// If interrupts enabled and INT key pressed.
				s &= 0xF7;															// Disable interrupts by clearing the IE flag in S
				temp16 = p3;p3 = p0;branch(temp16);									// Swap P0 and P3 (basically does DINT ; XPPC 3)
			}
			count = 1; 																// Force end of group.
		}
	}
	while (runForever || (--count > 0 && (p0+1) != breakPoint));					// Execute until breakpoint or tState counter high or once in S/S mode
																					// maybe run for ever in which case short circuit will not do check.

	return ((p0+1) == breakPoint);													// Return true if at breakpoint.
}

#ifdef DEBUGGABLE

//*******************************************************************************************************
//										Return the Status of the Processor
//*******************************************************************************************************

static CPUSTATUS stat;																// Used for status when NULL passed.

int _readMemory(int addr) { return readMemory(addr); }								// required because the functions are int/int.

CPUSTATUS *CPUGetStatus(CPUSTATUS *st) {
	if (st == NULL) st = &stat;
	st->a = a;st->e = e;st->s = constructStatus();
	st->p0 = p0;st->p1 = p1;st->p2 = p2;st->p3 = p3;
	st->pc = p0+1;																	// Note : pre-incrementing PC for SC/MP.
	st->readCodeMemory = _readMemory;
	st->readDataMemory = _readMemory;
	return st;
}
#endif

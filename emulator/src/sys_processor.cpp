// *******************************************************************************************************************************
// *******************************************************************************************************************************
//
//		Name:		sys_processor.c
//		Purpose:	Processor Emulation.
//		Created:	12th July 2019
//		Author:		Paul Robson (paul@robsons.org.uk)
//
// *******************************************************************************************************************************
// *******************************************************************************************************************************

#include <stdio.h>
#include "sys_processor.h"
#include "sys_debug_system.h"
#include "hardware.h"

// *******************************************************************************************************************************
//														   Timing
// *******************************************************************************************************************************

#define CYCLE_RATE 		(3510000)													// Cycles per second (0.96Mhz)
#define FRAME_RATE		(60)														// Frames per second (50 arbitrary)
#define CYCLES_PER_FRAME (CYCLE_RATE / FRAME_RATE)									// Cycles per frame (20,000)

// *******************************************************************************************************************************
//													SC/MP I/O Lines
// *******************************************************************************************************************************

static void writeIOFlags(BYTE8 flags) {

}

static BYTE8 readSenseLines(void) {
	return 0;
}


// *******************************************************************************************************************************
//														CPU / Memory
// *******************************************************************************************************************************

#define addAddress(a,b) 	(((a) & 0xF000) | (((a)+(b)) & 0xFFF)) 					// 4/12 bit Add macro.
#define branch(a)  	p0 = (a)														// Execute a branch

#include "scmp/scmp_autocode.h"
static BYTE8 ramMemory[RAMSIZE];													// Memory at $0000 upwards
static LONG32 cycles;																// Cycle Count.
static BYTE8 operand;

// *******************************************************************************************************************************
//											 Memory and I/O read and write macros.
// *******************************************************************************************************************************

#define readByte(a) 	_Read(a)													// Basic Read
#define writeByte(a,d)	_Write(a,d)													// Basic Write

#define addCycles(n)  	cycles += (n)												// Bump cycle counter

#define Fetch() 	_Read(++p0)														// Fetch byte


static inline BYTE8 _Read(WORD16 address);											// Need to be forward defined as 
static inline void _Write(WORD16 address,BYTE8 data);								// used in support functions.

// *******************************************************************************************************************************
//											   Read and Write Inline Functions
// *******************************************************************************************************************************

static inline BYTE8 _Read(WORD16 address) {
	address &= 0xFFF; 																// only 12 bits relevant.
	if (address < 0xC00 || address >= 0xF80) return ramMemory[address];				// RAM, ROM, 8154 RAM.
	if ((address >> 8) == 0xE) return ramMemory[address];							// Video RAM.
	if ((address >> 8) == 0xC) return 0;											// Cxx Keyboard
	if ((address >> 8) == 0xD) return HWReadUART(address & 0xFF);					// UART (D00-DFF)
	if ((address >> 7) == 0x1E) return HWRead8154(address & 0x7F);					// NS8154 (F00-F7F)
	return 0x00;																	// Return float low lines.

}

static inline void _Write(WORD16 address,BYTE8 data) {
	address &= 0xFFF; 																// only 12 bits relevant.
	if (address < 0xC00 || address >= 0xF80) ramMemory[address] = data;				// RAM, ROM, 8154 RAM.
	if ((address >> 8) == 0xE) ramMemory[address] = data;							// Video RAM.
	if ((address >> 8) == 0xD) return HWWriteUART(address & 0xFF,data);				// UART (D00-DFF)
	if ((address >> 7) == 0x1E) return HWWrite8154(address & 0x7F,data);			// NS8154 (F00-F7F)
}


// *******************************************************************************************************************************
//														BIOS
// *******************************************************************************************************************************

#include "scrumpi3_bios.h"

#ifdef INCLUDE_DEBUGGING_SUPPORT
static void CPULoadChunk(FILE *f,BYTE8* memory,int count);
#endif

// *******************************************************************************************************************************
//														Reset the CPU
// *******************************************************************************************************************************


void CPUReset(void) {

	int romSize = sizeof(_biosROM);
	for (int i = 0;i < romSize;i++) ramMemory[i] = _biosROM[i];						// Copy ROM images in


	#ifdef INCLUDE_DEBUGGING_SUPPORT 												// In Debug versions can
	FILE *f = fopen("monitor.rom","rb"); 											// read in new ROMs if in
	if (f != NULL) {																// current directory.
		CPULoadChunk(f,ramMemory,0x800);
		fclose(f);
	}
	#endif

	resetProcessor();																// Reset CPU
}

// *******************************************************************************************************************************
//					Called on exit, does nothing on ESP32 but required for compilation
// *******************************************************************************************************************************

#ifdef INCLUDE_DEBUGGING_SUPPORT
#include "gfx.h"
void CPUExit(void) {	
	GFXExit();
}
#else
void CPUExit(void) {}
#endif

// *******************************************************************************************************************************
//												Handle long delay 
// *******************************************************************************************************************************

static void longDelay(BYTE8 operand,BYTE8 a) {

}

// *******************************************************************************************************************************
//												Execute a single instruction
// *******************************************************************************************************************************

BYTE8 CPUExecuteInstruction(void) {
	BYTE8 opcode = Fetch();															// Fetch opcode.
	if (opcode & 0x80) operand = Fetch();
	switch(opcode) {																// Execute it.
		#include "scmp/scmp_opcodes.h"
	}
	if (cycles < CYCLES_PER_FRAME) return 0;										// Not completed a frame.
	cycles = cycles - CYCLES_PER_FRAME;												// Adjust this frame rate.
	return FRAME_RATE;																// Return frame rate.
}

// *******************************************************************************************************************************
//												Read/Write Memory
// *******************************************************************************************************************************

BYTE8 CPUReadMemory(WORD16 address) {
	return readByte(address);
}

void CPUWriteMemory(WORD16 address,BYTE8 data) {
	writeByte(address,data);
}

#ifdef INCLUDE_DEBUGGING_SUPPORT

// *******************************************************************************************************************************
//		Execute chunk of code, to either of two break points or frame-out, return non-zero frame rate on frame, breakpoint 0
// *******************************************************************************************************************************

BYTE8 CPUExecute(WORD16 breakPoint1,WORD16 breakPoint2) { 
	BYTE8 next;
	do {
		BYTE8 r = CPUExecuteInstruction();											// Execute an instruction
		if (r != 0) return r; 														// Frame out.
		next = CPUReadMemory(p0+1);
	} while (p0+1 != breakPoint1 && p0+1 != breakPoint2 && next != 0x00);			// Stop on breakpoint or HALT
	return 0; 
}

// *******************************************************************************************************************************
//									Return address of breakpoint for step-over, or 0 if N/A
// *******************************************************************************************************************************

WORD16 CPUGetStepOverBreakpoint(void) {
	BYTE8 opcode = CPUReadMemory(p0+1);												// Current opcode.
	if (opcode == 0x3F) return (p0+2) & 0xFFFF;										// Step over JSR.
	return 0;																		// Do a normal single step
}

void CPUEndRun(void) {
	FILE *f = fopen("memory.dump","wb");
	fwrite(ramMemory,1,RAMSIZE,f);
	fclose(f);
}

static void CPULoadChunk(FILE *f,BYTE8* memory,int count) {
	while (count != 0) {
		int qty = (count > 4096) ? 4096 : count;
		fread(memory,1,qty,f);
		count = count - qty;
		memory = memory + qty;
	}
}
void CPULoadBinary(char *fileName) {
	FILE *f = fopen(fileName,"rb");
	if (f != NULL) {
		CPULoadChunk(f,ramMemory,RAMSIZE);
		fclose(f);
		resetProcessor();
	}
}

// *******************************************************************************************************************************
//											Retrieve a snapshot of the processor
// *******************************************************************************************************************************

static CPUSTATUS st;																	// Status area

CPUSTATUS *CPUGetStatus(void) {
	st.a = a;st.e = e;
	st.p0 = p0;st.p1 = p1;st.p2 = p2;st.p3 = p3;
	st.s = constructStatus();
	st.cycles = cycles;
	return &st;
}

#endif
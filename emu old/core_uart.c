//*******************************************************************************************************
//*******************************************************************************************************
//
//      Name:       Core_Uart.C (SC/MP Scrumpi 3)
//      Purpose:    CPU Core (UART Emulation)
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

// TODO: Implement UART if bothered

void CPUWriteUART(BYTE8 offset,BYTE8 data) {
}

BYTE8 CPUReadUART(BYTE8 offset) {
	return 0x00;
}
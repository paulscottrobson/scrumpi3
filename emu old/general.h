//*******************************************************************************************************
//*******************************************************************************************************
//
//      Name:       General.H
//      Purpose:    General Constants, Includes
//      Author:     Paul Robson
//      Date:       1st January 2014
//
//*******************************************************************************************************
//*******************************************************************************************************

#ifndef _GENERAL_H
#define _GENERAL_H

typedef unsigned char BOOL;
typedef unsigned char BYTE8;                                                        // Type definitions used in CPU Emulation
typedef unsigned short WORD16;
typedef unsigned int LONG32;

#define BYTE8CONST	BYTE8 															// Constant byte type (ROM on Microcontroller)

#define BUILD   	(0) 															// Build Number.

#define FALSE       (0)                                                             // Boolean type
#define TRUE        (!(FALSE))

void SYSRun(int argc,char *argv[]);

#endif



; *******************************************************************************************************************************
; *******************************************************************************************************************************
;
;                                             SCRUMPI 3 ROM (White Room Rewrite)
;                                             ==================================
;
;                      Written by Paul Robson 26-27 February 2014. Based on designs by John H. Miller-Kirkpatrick
;
;       TODO: Map it so it actually works in Page 7, not page 0. I think everything is designed to go in Page 7 !
;             Hardware, BIOS Routines, Other Jumps, Tricks and Initialisation.
;
; *******************************************************************************************************************************
; *******************************************************************************************************************************
;
;       0FC0-0FDF       Stack (32 bytes)
;       0FE0-0FE7       Labels (8 bytes)
;       0FE8            Current Cursor position.
;       0FE9            Count Byte for Displays 
;       0FEA,B          Implant current address (Monitor only)
;       0FEC            Char printed at start of implant line print (Monitor only)
;       0FF7-0FFF       Storage for Registers (Monitor only)
;
;       SC/MP Pointer Registers [P3.H,P3.L,P2.H,P2.L,P1.H,P1.L,E,S,A]
;
;
;       P1 general usage (many routines preserve this)
;       P2 stack
;       P3 call/return address
;
keyboardPort =  12 * 256                                                ; Keyboard port is at $0C00
uartPort =      13 * 256                                                ; UART is at $0D00
videoRAM =      14 * 256                                                ; Video RAM is at $0E00 
systemRAM =     15 * 256 + 128                                          ; System RAM is at $0F80

registerBase =  15 * 256 + 247                                          ; where the registers come/go

;
;       110 Baud TTY Speed Calculation
;
;       110 Baud = 110Hz.
;       CPU Cycles = 3.51Mhz / 2
;       110 Baud delay is 3,510,000/110 = 31,909 Cycles.
;       Delay time is (31909-13)/514 which is exactly 62.
;
baud110Delay = 62                                                       ; required value for 110 baud delay.

;
;       UART code is completely speculative. The only thing we actually know is that it is an AY-5-1013. This
;       chip has no fixed registers but it does have three access pins. 
;
;       Pin 4 : Received Data Enable is a logic 0 activated port read. I have connected this to A0.
;       Pin 23 : Data Strobe is a logic 0 activated port write. I have connected this to A1.
;       Pin 16 : Status word Emable is a logic 0 activated status read, I have connected this to A2.
;
uartPortBase = uartPort + 240                                           ; base address for R/W

uartReceivedData = 14                                                   ; (e.g. bit 0 only low)
uartDataStrobe = 13                                                     ; (e.g. bit 1 only low)
uartStatusWordEnabled = 11                                              ; (e.g. bit 2 only low)

uartSWEParityError = 1                                                  ; these are the connected status word bits
uartSWEFramingError = 2                                                 ; these are just errors.
uartSWEOverRunError = 4

uartSWEDataAvailable = 8                                                ; logic '1' when data is available to receive.
uartSWETransmitBufferEmpty = 16                                         ; logic '1' when data can be transmitted.

;
;       System variables I have used
;
cursor =        6 * 16 + 8                                              ; cursor position in VDU RAM (1 byte) $0FE8 offset from RAM
labelArray =    systemRAM + 6 * 16                                      ; label array ($0FF8)

tempCounter =   9                                                       ; offset from TOS ($FE9)
implantAddress = 10                                                     ; offset from TOS ($FEA,B)
charLine = 12                                                           ; character to print in implant (fixes double CR) ($FEC)
                        
; *******************************************************************************************************************************
;
;       Disassembly of the code shown in the brochure (first and last 48 bytes of monitor) doesn't tell us much, but we do
;       know that the addresses in the second ROM (mapped at x600-x7FF) are all in page 7 (e.g. the ROM is actually at $7600)
;       and so is the NS8154 RAM I/O Chip.
;
;       This doesn't actually matter when the ROM/RAM space is not fully decoded. It also means it's highly likely that
;       *everything* is in page 7 - RAM, ROM , 8154, UART, Keyboard Port.
;
;       So the code is designed to work with that.
;
; *******************************************************************************************************************************
;
romPage = 7 * 16                                                        ; the ROM page it is designed to run in.

; *******************************************************************************************************************************
;
;                                                     ROM Monitor Code
;
; *******************************************************************************************************************************
; *******************************************************************************************************************************

        cpu     sc/mp

        nop                                                             ; first instruction executed is at location 1.
        ld      @-32(p2)                                                ; points stack to $0FE0 - (stack pre decrements)
                                                                        ; this can be moved back to ROM 0 if necessary.
                                                                        ; note ; on my emulator this does $FFE0 for speed.

        ldi     (BootPrompt-1) & 0xFF                                   ; make BIOSPrintInLine think we have called it
        xpal    p3                                                      ; by putting a fake 'return' address as if we had
                                                                        ; done XPPC P3.

; *******************************************************************************************************************************
;
;                                     Print String following the caller, terminated by $04.
;
;                                            Note: this routine is *not* re-entrant.
;
; *******************************************************************************************************************************

BIOSPrintInline:
        ldi     (BIOSPrintCharacter - 1) / 256                          ; copy P3 (old) to P1, and set up P3 to call PrintCharacter
        xpah    p3
        xpah    p1
        ldi     (BIOSPrintCharacter - 1) & 255        
        xpal    p3
        xpal    p1                                                      ; after this instruction, P1 points to XPPC 3.
_PILNextCharacter:
        ld      @1(p1)                                                  ; go to the next character
        ld      0(p1)                                                   ; read it.
        xppc    p3                                                      ; print the character.
        ld      0(p1)                                                   ; re-read the character
        xri     0x04                                                    ; is it $04 (end of print string)
        jnz     _PILNextCharacter                                       ; go back if not.
        xppc    p1                                                      ; P1 points to the $04 terminator, so return will add one (pre-increment)
BIOSPILEnd:

; *******************************************************************************************************************************
;
;                                       Boot Prompt, go from here to the main options menu
;
; *******************************************************************************************************************************

BootPrompt:
        db      01                                                      ; this is the boot up text - the initial call of BIOSPrintInline to do this
        db      "SCRUMPI 3"                                             ; is faked. $01 is Clear Screen
        db      13                                                      ; carriage return.
        db      4                                                       ; end of prompt

        ldi     (Main-1) & 0xFF                                         ; jmp to Main.
        xpal    p0                                                      ; long jump same page.

        ; IF IT DOESN'T BOOT, CHECK MAIN IS STILL IN PAGE 0 ! IT IS CURRENTLY AT $100, the last possible byte !

; *******************************************************************************************************************************
;
;                                   Load Registers back in and call P3, then Save Registers
;
; *******************************************************************************************************************************

LoadRegisters:
        ld      registerBase+0                                          ; load P3,P2,P1,E,S,A
        xpah    p3
        ld      registerBase+1
        xpal    p3
        ld      registerBase+2
        xpah    p2
        ld      registerBase+3
        xpal    p2
        ld      registerBase+4
        xpah    p1
        ld      registerBase+5
        xpal    p1
        ld      registerBase+6
        xae
        ld      registerBase+7
        cas
        ld      registerBase+8

        xppc    p3                                                      ; call whatever code is being run.
 
SaveRegisters:
        st      registerBase+8                                          ; save A,S,E
        csa
        st      registerBase+7
        lde
        st      registerBase+6

        xpal    p1                                                      ; save P1,P2
        st      registerBase+5
        xpah    p1
        st      registerBase+4

        ldi     0xE0                                                    ; when saving P2, make P2 point to the stack at $FE0
        xpal    p2                                                      
        st      registerBase+3
        ldi     systemRAM / 256                                      
        xpah    p2
        st      registerBase+2

        ldi     (BIOSPrintInline-1) & 255                               ; when saving P3, make it point to the BIOSPrintInline routine
        xpal    p3
        st      registerBase+1                                          
        ldi     (BIOSPrintInline-1) / 256
        xpah    p3 
        st      registerBase+0

        xppc    p3                                                      ; print status dump text.
        db      1,"SCRUMPI 3 STATUS DUMP",13
        db      " P3   P2   P1  EX ST AC",13
        db      4

        ld      @-((BIOSPILEnd - registerBase-1) & 0x7F)(p1)            ; fix up P1 so it points to the registers. We can do this 
                                                                        ; because BIOSPrintInline exits via XPPC P1.

        ldi     2                                                       ; counter = 2
        st      tempCounter(p2)
_REDumpBytes:
        ldi     (BIOSPrintHexSpace-1) & 0xFF                            ; P3 to point to hex printer, with space.
        xpal    p3
        ldi     (BIOSPrintHexSpace-1) / 256
        xpah    p3
        ld      tempCounter(p2)                                         ; read counter.
        ani     1+8                                                     ; if odd, or 8, print with space
        jnz     _RESpaceNeeded                                          ; print no space after 2,4 and 6 - the Px.H bytes.

        ld      @BIOSPrintHexNoSpace - BIOSPrintHexSpace(p3)            ; fix call address so points to the no-space printer.
_RESpaceNeeded:
        ld      @1(p1)                                                  ; read the next byte.
        xppc    p3                                                      ; dump it.
        ild     tempCounter(p2)                                         ; increment counter
        xri     11
        jnz     _REDumpBytes                                            ; until dumped everything out.
        ld      @-0x38(p1)                                              ; make P1 point to 0FC8, the bit where the stack is dumped.
        ldi     3*8                                                     ; we are going to do twenty four bytes (e.g. 3 lines)

        jmp     DumpMemory                                              ; Dump the memory out.

; *******************************************************************************************************************************
;
;                                         Run Program from P1 and Continue (G and C)
;
; *******************************************************************************************************************************

Go:     ld      @-1(p1)                                                 ; going to XPPC 3 so subtract 1
        xpah    p1                                                      ; copy address from P1 into Register memory.
        st      0x17(p2)
        xpal    p1
        st      0x18(p2)
Continue:
        ldi     (LoadRegisters - 1) & 0xFF                              ; long jump same page.
        xpal    p0

; *******************************************************************************************************************************
;       
;                                                               Show Labels
;
; *******************************************************************************************************************************

ShowLabels:
        ldi     labelArray & 255                                        ; point P1 to label array
        xpal    p1
        ldi     labelArray / 256 
        xpah    p1
        ldi     -5*8                                                    ; list 1 line (the number is for following add)
                                                                        ; falls through to list memory.

; *******************************************************************************************************************************
;
;                                                               List Memory
;
; *******************************************************************************************************************************

ListMemory:
        ccl                                                             ; on entry from main loop, A = 0
        adi     6*8 
                                                                        ; falls through to DumpMemory

_Go:    jz      Go                                                      ; allows us to reach Go.

; *******************************************************************************************************************************
;
;                                               Dump A bytes of memory from P1 onwards.
;
; *******************************************************************************************************************************

DumpMemory:
        st      tempCounter(p2)                                         ; save in counter byte.
_DMMLoop:
        ld      tempCounter(p2)                                         ; time for a new line ?
        ani     7
        jnz     _DMMNotNewLine

        ldi     (BIOSPrintCharacter-1) & 255                            ; print a new line.
        xpal    p3
        ldi     (BIOSPrintCharacter-1) / 256
        xpah    p3
        ldi     0x0D
        xppc    p3
        ldi     (BIOSPrintHexNoSpace-1) & 255                           ; set up to print the address.
        xpal    p3                                                     
        ldi     (BIOSPrintHexNoSpace-1) / 256                          ; Note : on exit of BIOSPrintCharacter P3 already set up.
        xpah    p3

        xpah    p1                                                      ; print high byte.
        xae
        lde
        xpah    p1
        lde
        xppc    p3
        xpal    p1                                                      ; print low byte
        xae
        lde
        xpal    p1
        lde
        xppc    p3

_DMMNotNewLine:
        ld      @1(p1)                                                  ; print a byte
        xppc    p3
        dld     tempCounter(p2)                                         ; decrement counter
        jnz     _DMMLoop                                                ; done the lot, no go back.

; *******************************************************************************************************************************
;               
;                                                     Main Command Entry Point
;
;       Commands :
; 
;       I xxxx  implant.
;       L       show labels
;       H xxxx  hex dump.
;       G xxxx  run.
;       C       continue.
;
;
; *******************************************************************************************************************************

Main:   ldi     (BIOSPrintInline-1) / 256                               ; Print the 'COMMAND ?' prompt 
        xpah    p3
        ldi     (BIOSPrintInline-1) & 255
        xpal    p3
        xppc    p3
        db      13,"COMMAND ? ",4                                       ; can shorten this if we get really desperate :)

        ldi     (BIOSReadKey-1) / 256                                   ; Read a key.
        xpah    p3
        ldi     (BIOSReadKey-1) & 255                                   
        xpal    p3
        xppc    p3
        st      tempCounter(p2)                                         ; save in counter temp.
        ldi     ' '                                                     ; BIOS Readkey re-enters into print character so print space
        xppc    p3

        ldi     0x72                                                    ; look at extension ROM at $7200-$75FF
        xpah    p3
        ldi     0x00
        xpal    p3
        ld      0(p3)                                                   ; look at first byte to see if there's anything there at all.
        jz      _CheckCommands                                          ; if it is zero, ignore it.
        xppc    p3                                                      ; jump to $7201
        nop
_CheckCommands:

        ld      tempCounter(p2)                                         ; load control character.
        xri     'C'                                                     ; if not Continue then skip the reload and run bit.
        jz      Continue                                                ; if it is 'C' then continue by reloading registers and running.     
        xri     'C' ! 'L'                                               ; check for (L)abel display
        jz      ShowLabels

        ldi     (ReadHexadecimalSet-1) & 0xFF                           ; load a 4 digit hex number into P1.
        xpal    p3
        ldi     (ReadHexadecimalSet-1) / 256                                    
        xpah    p3
        ldi     4
        xppc    p3                                                      ; call it
        jnz     Main                                                    ; bad value !

        ld      tempCounter(p2)                                         ; reload control character.
        xri     'H'                                                     ; is it (H)ex Dump
        jz      ListMemory
        xri     'G' ! 'H'                                               ; is it (G)o
        jz      _Go

_Main:  jz      Main                                                    ; not superfluous - long jump !

        xri     'I'!'G'                                                 ; is it (I)mplant
        jnz     Main                                                    ; Nope, out of ideas.

; *******************************************************************************************************************************
;
;                                                       Implant Code
;
; *******************************************************************************************************************************

Implant:
        ldi     0x0D                                                    ; character to print after this

_ImpStoreCharacter:
        st      charLine(p2)

_ImpUpdate:
        ld      0(p1)                                                   ; read byte at implant address
        st      tempCounter(p2)                                         ; save it.

        xpal    p1                                                      ; save implant address
        st      implantAddress(p2)
        xpah    p1
        st      implantAddress+1(p2)

        ldi     (BIOSPrintCharacter-1) & 255                            ; print a new line, perhaps.
        xpal    p3
        ldi     (BIOSPrintCharacter-1) / 256
        xpah    p3
        ld      charLine(p2)                                            ; get character to print.
        xppc    p3
        ldi     (BIOSPrintHexNoSpace-1) & 255                           ; set up to print the address.
        xpal    p3                                                     
        ldi     (BIOSPrintHexNoSpace-1) / 256
        xpah    p3
        ld      implantAddress+1(p2)                                    ; print the address
        xppc    p3
        ld      implantAddress(p2)
        xppc    p3
        ld      tempCounter(p2)                                         ; and the data that's there.
        xppc    p3
        ldi     0x0D
        st      charLine(p2)                                            ; set to print CR next time.
_ImpGet:
        ldi     (ReadHexadecimalSet-1) & 255                            ; read a 2 byte number
        xpal    p3
        ldi     (ReadHexadecimalSet-1) / 256
        xpah    p3
        ldi     2
        xppc    p3
        xae                                                             ; store result in E

        ld      implantAddress+1(p2)                                    ; reload implant address in P1
        xpah    p1
        ld      implantAddress(p2)
        xpal    p1                                                      ; this loads the byte/key code into A
        xae                                                             ; now the byte/key code is in E and the error flag in A
                                                                        ; and P1 points to the byte data.
        jnz     _ImpControlKey                                          ; it wasn't a hex number, it was a control key.

        lde                                                             ; get byte back
        st      0(p1)                                                   ; write it at the implant address
        jmp     _ImpGet                                                 ; and get again - you can override or use INT to go to the next line.

_ImpNext:                                                               ; CR pressed
        ld      @1(p1)                                                 
        ldi     0x04                                                    ; no CR printed otherwise we get double CR
        jmp     _ImpStoreCharacter

_ImpControlKey:                                                         ; P1 = Implant, E = Key.
        lde                                                             ; get key code
        xri    '>'                                                      ; is it go to monitor
        jz      _Main                                                   ; yes, exit.
        xri     0x0D ! '>'                                              ; is it CR
        jz      _ImpNext                                                ; next byte.

        lde                                                             ; save original control in temp Counter
        st      tempCounter(p2)
        xri     '='                                                     ; check equals or ?
        jz      _ImpIsLabel
        xri     '='! '?'                                                
        jz      _ImpIsLabel
_ImpUpdate2:
        jmp     _ImpUpdate                                              

_ImpIsLabel:
        ldi     (BIOSReadKey - 1) & 0xFF                                ; get a key.
        xpal    p3
        ldi     (BIOSReadKey - 1) / 256
        xpah    p3
        xppc    p3                                                      ; get a key value.

        xae                                                             ; copy into E

        ld      implantAddress+1(p2)                                    ; reload implant address in P1
        xpah    p1
        ld      implantAddress(p2)
        xpal    p1                                                      

        lde                                                             ; get the key value.
        ani     0xF8                                                    ; check if it is $30-$37 e.g. numbers 0-7
        xri     0x30
        jnz     _ImpUpdate2                                              ; if not, then fail command.

        lde                                                             ; we put the label address in P3 as we won't call anything.
        xri     0x30 ! 0xE0                                             ; change from 3x to Ex
        xpal    p3
        ldi     systemRAM / 256
        xpah    p3 

        ld      tempCounter(p2)                                         ; get the temporary counter back.
        xri     '='                                                     ; is it equals
        jnz     _ImpCalculateOffset                                     ; calculate offset from location to here.

        ld      implantAddress(p2)                                      ; get the lower byte of the address
        st      0(p3)                                                   ; save in the label area
        jmp     _ImpUpdate2                                             ; and end command.

_ImpCalculateOffset:
        ld      0(p3)                                                   ; get label
        ccl
        cad     implantAddress(p2)                                      ; read the LSB.
        st      0(p1)                                                   ; store it
        jmp     _ImpUpdate2     

; *******************************************************************************************************************************
;
;                               Read a byte from the UART to A. On exit CY/L => error, , preserves P1
;
; *******************************************************************************************************************************

BIOSGetART:        
        ldi     uartPortBase / 256                                      ; we set P2 to point to the UART for writing
        xpah    p2
        ldi     uartPortBase & 255                              
        xpal    p2                                                      ; we can use E to save P2.L 
        xae                                                             ; which saves stacking/destacking P1.
_GARWait:
        ld      uartStatusWordEnabled(p2)                               ; read the status word
        ani     uartSWEDataAvailable                                    ; wait for data available
        jz      _GARWait
        ld      uartStatusWordEnabled(p2)                               ; re-read it and mask out the error bits.
        ani     uartSWEFramingError+uartSWEParityError+uartSWEOverRunError
        ccl
        adi     0xFF                                                    ; will set carry unless every bit is zero i.e. no errors
        ld      uartReceivedData(p2)                                    ; read the byte in.
        xae                                                             ; put in E ; get P2.L back
        xpal    p2                                                      ; save in P2
        ldi     0x0F                                                    ; set P2.H to point to the stack
        xpah    p2
        lde                                                             ; restore the read byte
        xppc    p3                                                      ; and exit.
        jmp     BIOSGetART


; *******************************************************************************************************************************
;
;                                                         1k ROM Space in the middle.
;
; *******************************************************************************************************************************

        org     0x200                                                   ; 0200 - 00 x 1024.
        db      1024 dup (0)
        org     0x600                                                   ; 2nd half of the ROM.
                                                                        ; no, I don't know why it's not at $200 either !

; *******************************************************************************************************************************
;
;                             Read a Debounced ASCII key from the Keyboard into A and Echo it.
;
;              Note: this is not re-entrant because it falls through into the BIOSPrintCharacter routine.
;
; *******************************************************************************************************************************

BIOSReadKey:
        ld      @-2(p2)                                                 ; stack space. +0 is current key, +1 is debounce count

_RKYWaitRelease:                                                        ; wait for all keys to be released.
        csa 
        ani     0x10                                                    ; firstly check SA.
        jnz     _RKYWaitRelease 
        xpal    p1                                                      ; now point P1 to the keyboard at $C00
        ldi     KeyboardPort / 256                  
        xpah    p1
        ld      0xF(p1)                                                 ; scan every row and column (reads $C0F)
        ani     15                                                      ; the control keys can stay pressed, it doesn't matter.
        jnz     _RKYWaitRelease

_RKYDebounceReset:                                                      ; come back here if debounce check fails e.g. not held steady.
        ldi     0x0                                                     ; reset the current key value
        st      0(p2)

_RKYRecheckKey:
        ldi     3                                                       ; we start the key count at 3 (because the LHS is the MSB)
        xae

        csa                                                             ; read the state of Sense A.
        ani     0x10                                                    ; check sense A pressed now $00 No, $10 yes
        scl
        cai     3                                                       ; now $FD now, $0D yes.
        jp      _RKYGotASCIIKey                                         ; got the ASCII key ?
        ldi     0x08                                                    ; start reading $0C08 e.g. the top row.
_RKYDiscoverRow:
        xpal    p1                                                      ; set P1 to point to the row.
        ld      0(p1)                                                   ; read that row
        ani     15                                                      ; look at the 16 key part data only (no shifts)
        jnz     _RKYFoundKey                                            ; if a key is non-zero, then we have found a key press
        xae                                                             ; add 4 to E (e.g. the next row down)
        ccl
        adi     4
        xae
        xpal    p1                                                      ; this is the row address e.g. it goes 8,4,2,1
        sr                                                              ; shift it right.
        jnz     _RKYDiscoverRow                                         ; if non-zero, there's another row to scan.
        jmp     _RKYDebounceReset                                       ; if zero, no key found, so start key scanning again.

_RKYFindBit:
        xae                                                             ; decrement E (the Left most key is bit 3)
        scl
        cai     1
        xae 
_RKYFoundKey:                                                           ; found key. key bit is in A, base is in E (0,4,8,12)
        sr                                                              ; shift that bit pattern right.
        jnz     _RKYFindBit                                             ; if non-zero, change E and try again.
        xpal    p1                                                      ; clear P1.L so it points to $C00 again.

        ld      0(p1)                                                   ; read the keypad again - we're going to shift it now.008
        ani     0x70                                                    ; mask out the punctuation, and alpha shift keys. (Bits 4,5,6)
                                                                        ; NoShift : $00 Alpha 1 : $10 Alpha 2 : $20 Punc : $40

        xri     0x40                                                    ; NoShift : $40 Alpha 1 : $50 Alpha 2 : $60 Punc : $00
        jnz     _RKYNotPunctuation
        ldi     0x30                                                    ; set to $30 if $00 (punctuation.)
_RKYNotPunctuation:                                                     ; NoShift : $40 Alpha 1 : $50 Alpha 2 : $60 Punc : $30
        ccl
        adi     0xF0                                                    ; NoShift : $30 ('0') Alpha 1 : $40 ('@') Alpha 2 : $50 ('P') Punc : $20(' ')
        ore                                                             ; Or the lower 4 bits into it, A now contains the complete character.
_RKYGotASCIIKey:
        xae                                                             ; put back in E.
        ld      0(p2)                                                   ; read current character , if zero, it's a new test
        jnz     _RKYCheckDebounce
        lde                                                             ; save current character, we have just pressed that.
        st      0(p2)
        ldi     4                                                       ; need it to be the same for 4 consecutive reads.
        st      1(p2)
        jmp     _RKYRecheckKey

_RKYCheckDebounce:
        xre                                                             ; is it the same as the current key pressed 
        jnz     _RKYDebounceReset                                       ; if it has changed, debounce has failed, so go back and start again
        dld     1(p2)                                                   ; decrement the debounce counter
        jnz     _RKYRecheckKey                                          ; and go back if not been checked enough.

        ld      @2(p2)                                                  ; fix the stack up
        lde                                                             ; get the character into A.
        jmp     BIOSPrintCharacter                                      ; echo it, and return.

; *******************************************************************************************************************************
;
;                                Print A in Hex, with or without a trailing space. Preserves P1.
;
;                            Partially re-entrant, if called again always is in trailing space mode.
;
; *******************************************************************************************************************************

BIOSPrintHexNoSpace:
        xae                                                             ; put byte to output in E
        ldi     0                                                       ; no trailing space.
        jmp     _PHXMain
BIOSPrintHexSpace:                                                      
        xae                                                             ; put byte to output in E
        ldi     ' '                                                     ; with trailing space flag.
_PHXMain:                                                
        st      -1(p2)                                                  ; save trailing flag on stack
        ldi     (_PHXPrintHex - 1) / 256                                ; save return address in -3(p2),-4(p2)
        xpah    p3                                                      ; at the same time point to Print Nibble routine.                                                     
        st      -3(p2)                                                 
        ldi     (_PHXPrintHex - 1) & 255
        xpal    p3
        st      @-4(p2)                                                 ; and adjust stack downwards.
        lde                                                             ; restore byte to print
        st      2(p2)                                                   ; save it in stack temp space.
        rr                                                              ; rotate right to get upper nibl.
        rr
        rr
        rr
        xppc    p3                                                      ; print the upper nibble
        ldi     (_PHXPrintHex-1) & 255                                  ; this is not re-entrant because it tages onto PrintCharacter
        xpal    p3
        ldi     (_PHXPrintHex - 1) / 256                                ; however, these 3 bytes could be lost if PrintHex and BIOSPrintChar in same page.
        xpah    p3                                                                                                                   
        ld      2(p2)                                                   ; read byte to print
        xppc    p3                                                      ; print the lower nibble.
        ld      3(p2)                                                   ; read trailing flag        
        jz      _PHXNoTrailer                                           ; skip if zero
        xppc    p3                                                      ; and print that - BIOSPrintCharacter will be used.
_PHXNoTrailer:
        ld      1(p2)                                                   ; restore return address
        xpah    p3
        ld      @4(p2)                                                  ; fix the stack back here
        xpal    p3
        xppc    p3                                                      ; and exit
        jmp     BIOSPrintHexSpace                                       ; re-entrant with trailing space ONLY.

_PHXPrintHex:                                                           ; print A as a Nibble
        ani     15                                                      ; mask out nibble
        ccl

        dai     0x90                                                    ; hopefully this will work - SC/MP version of Z80 trick.
                                                                        ; 0-9 => 9x, CY/L = 0, A-F, 0x, CY/L = 1
        dai     0x40                                                    ; 0-9 => 3x,  A-F = 4x+1
                                                                        ; depends on how it actually adds but it works on most CPUs.
                                                                        ; *** FALLS THROUGH ***

; *******************************************************************************************************************************
;
;                                        Print the Character in A. Preserves P1 and A.
;
; *******************************************************************************************************************************

BIOSPrintCharacter:
        xae                                                             ; save character to print in E.
        ldi     systemRAM / 256                                         ; save P1 on stack, make it point to $0F80
        xpah    p1
        st      @-1(p2)  
        ldi     systemRAM & 255                                        
        xpal    p1
        st      @-1(p2) 

        lde                                                             ; get character to be printed back.
        ccl                                                             ; add $E0, causes carry if 20-FF
        adi     0xE0
        csa                                                             ; check the carry bit
        jp      _PRCControl                                             ; if clear, it is a control character.

        ld      cursor(p1)                                              ; read the current cursor position.
        xpal    p1                                                      ; put in P1.L
        ldi     videoRAM/256                                            ; make P1.H point to video RAM.
        xpah    p1
        xae                                                             ; retrieve character from E, save systemRAM in E.
        st      @1(p1)                                                  ; save character in P1
        xae                                                             ; put it back in E.

_PRCShowCursorAndSaveCheckScroll:                                       ; show cursor and save position BUT scroll up if at TOS (gone off bottom)
        xpal    p1                                                      ; scrolling up ?
        jz      _PRCScroll
        xpal    p1
_PRCShowCursorAndSavePosition:
        ldi     0xA0                                                    ; put a cursor (solid block) in the next square
        st      0(p1)

        ldi     SystemRAM / 256                                         ; make P1 point to System RAM.
        xpah    p1
        ldi     systemRAM & 255       
        xpal    p1                                                      ; at the same time retrieve the cursor position into A.
        st      cursor(p1)                                              ; write the cursor position out, updated.
_PRCExit:
        ld      @1(p2)                                                  ; restore P1 off the stack.
        xpal    p1
        ld      @1(p2)
        xpah    p1
        lde                                                             ; restore printed character from E.
        xppc    p3                                                      ; and exit the routine.
        jmp     BIOSPrintCharacter                                      ; make it re-entrant.

_PRCControl:                                                            
        ld      cursor(p1)                                              ; make P1 point to the current video RAM location.
        xpal    p1
        ldi     VideoRAM/256                                            
        xpah    p1
        ldi     ' '                                                     ; erase current cursor.
        st      0(p1)           
        lde                                                             ; get character code back.  

        xri     0x01                                                    ; test for code $01 (Clear Screen)
        jnz     _PRCControl2

_PRCClearScreen:
        xpal    p1                                                      ; current position in A, to P1.L
        ldi     ' '                                                     ; write space to screen and bump to next.
        st      @1(p1) 
        xpal    p1                                                      ; has it reached $00 again, e.g. whole screen.
        jnz     _PRCClearScreen                                         ; not finished clearing yet.
        xpal    p1                                                      ; p1 points to cursor now (e.g. it is 0) and p1.h is VRAM.
        ldi     VideoRAM/256                                            ; make it point to video RAM again.
        xpah    p1
        jmp     _PRCShowCursorAndSavePosition                           ; reshow the cursor and save the cursor position.

_PRCControl2:
        xri     0x01 ! 0x0D                                             ; carriage return ?
        jnz     _PRCControl3
        xpal    p1                                                      ; get cursor position in P1.
        ani     0xE0                                                    ; start of line.
        ccl
        adi     0x20                                                    ; next line down.
        xpal    p1                                                      ; save cursor position
        jmp     _PRCShowCursorAndSaveCheckScroll

_PRCControl3:                                                           
        xri     0x0D ! 0x08                                             ; is it backspace ($08)
        jnz     _PRCShowCursorAndSavePosition                           ; unknown control character.
        xpal    p1                                                      ; get the cursor position into A.
        jz      _PRCStayHome                                            ; can't backspace any further, already at 0
        ccl                                                             ; subtract 1 from cursor position.
        adi     0xFF
_PRCStayHome:
        xpal    p1                                                      ; position back in P1.
        jmp     _PRCShowCursorAndSavePosition                           ; do not check for scrolling, but reshow cursor and save.

_PRCScroll:                                                             ; scroll screen up. 
        ldi     VideoRAM/256                                            ; point P1.H to video RAM.
        xpah    p1
        ldi     0x20                                                    ; point P1.L to second line.
_PRCScrollLoop:
        xpal    p1                                                      
        ld      @1(p1)                                                  ; read line above and increment p1 afterwards.
        st      -0x21(p1)                                               ; save immediately above, allow for increment
        xpal    p1
        jnz     _PRCScrollLoop                                          ; on exit P1 = 0xF00.
        xpal    p1
        ld      @-32(p1)                                                ; fix up P1 to point to bottom line
        xpal    p1
_PRCClearBottom:
        xpal    p1                                                      ; clear the bottom line.
        ldi     ' '
        st      @1(p1)
        xpal    p1
        jnz     _PRCClearBottom
        xpal    p1
        ld      @-32(p1)                                                ; fix it up again so cursor is start of bottom line.
        jmp     _PRCShowCursorAndSavePosition

; *******************************************************************************************************************************
;
;       Attempt to read A hexadecimal digits. If okay : P1.H = <High>, E = <Low>, A = 0
;                                             Bad Key : E = Key Number, A != 0
;
; *******************************************************************************************************************************

ReadHexadecimalSet:
        st      @-6(p2)                                                 ; reserve stack space. offset 0 is the number of nibbles to read.
        xpal    p3                                                      ; save return address on stack
        st      5(p2)
        xpah    p3 
        st      4(p2)
        ldi     0                                                       ; clear the high byte (2) and low byte (3)
        st      2(p2)
        st      3(p2)
_RHSMain:                                                               ; main read loop.
        ldi     4
        st      1(p2)                                                   ; shift the current byte left 4 times.
_RHSShiftLoop:
        ccl
        ld      3(p2)                                                   ; read low byte
        add     3(p2)                                                   ; shift into carry.
        st      3(p2)
        ld      2(p2)                                                   ; high byte likewise, but inherit carry from low byte.
        add     2(p2)
        st      2(p2)
        dld     1(p2)                                                   ; do it four times
        jnz     _RHSShiftLoop
_RHSGetHexKey:
        ldi     (BIOSReadKey-1) / 256                                   ; read and echo a key
        xpah    p3
        ldi     (BIOSReadKey-1) & 255
        xpal    p3
        xppc    p3
        ccl
        xae                                                             ; put key in E as temporary store.
        lde     
        adi     255-'F'                                                 ; is key >= F
        jp      _RHSError                                               ; then error.
        adi     6                                                       ; key will now be 0,1,2,3,4,5 for A-F.
        jp      _RHSIsAlphabeticHex                                     ; go there if it is.
        adi     7                                                       ; if +ve will be wrong.
        jp      _RHSError                                               ; so go back.
        adi     1                                                       ; shift 0-9 into the correct range when the 9 is added after this.
        ccl
_RHSIsAlphabeticHex:
        adi     9                                                       ; we know CY/L will be set after the adi 6 that came here.
        jp      _RHSGotDigit                                            ; if +ve we have a legal digit 0-15.

_RHSError:
        lde                                                             ; get the key that was pressed, put in low byte.
        st      3(p2)   
        scl                                                             ; and exit with CY/L set
        jmp     _RHSExit

_RHSGotDigit:
        or      3(p2)                                                   ; or into the low byte
        st      3(p2)
        dld     0(p2)                                                   ; have we done this four times ?
        jnz     _RHSMain                                                ; no, go round again !
        ccl                                                             ; we want CY/L clear as it is okay.
_RHSExit:                                                               ; exit.
        ld      @6(p2)                                                  ; fix the stack back
        ld      -1(p2)
        xpal    p3
        ld      -2(p2)
        xpah    p3   
        ld      -4(p2)                                                  ; read high byte, put in P1.H
        xpah    p1
        ld      -3(p2)                                                  ; read low byte, put in E
        xpal    p1 
        csa                                                             ; get status
        ani     0x80                                                    ; isolate carry flag, returns $80 on error.
        xppc    p3                                                      ; and exit.
        jmp     ReadHexadecimalSet              

; *******************************************************************************************************************************
;
;                         Put A to Flag 1 as a 110 Baud TTY (1 Start, 8 Data, nothing else), preserves P1
;
; *******************************************************************************************************************************

BIOSPutTTY:
        xae                                                             ; put in E
        ldi     10                                                      ; set bit count to 10. Start + 8 Data + Clearing value.
        st      -1(p2)                                                  ; counter is on stack.
_PTTYSetLoop:
        csa                                                             ; output start bit.
        ori     0x02 
_PTTYLoop:
        cas                                                             ; write A to S.
        ldi     0x00                                                    ; delay 110 baud.
        dly     baud110delay                                            
        dld     -1(p2)                                                  ; decrement the counter
        jz      _PTTYExit                                               ; and exit if it is zero.
        lde                                                             ; shift E left into the carry/link bit.
        ccl
        ade
        xae      
        csa                                                             ; get the status register, CY/L is bit 7.
        ani     0xFD                                                    ; clear the F1 bit, just in case.
        jp      _PTTYLoop                                               ; if it is '1' then output to S and delay
        jmp     _PTTYSetLoop                                            ; otherwise set it to '1' and delay.
_PTTYExit:
        xppc    p3                                                      ; exit, this is re-entrant.
        jmp     BIOSPutTTY


; *******************************************************************************************************************************
;
;                              Read SB as a 110 Baud TTY into A (1 start bit, 8 data bits), preserves P1
;
; *******************************************************************************************************************************

BIOSGetTTY:
        ldi     0                                                       ; clear final result in E
        xae
_GTTYWait:
        csa                                                             ; wait until SB is logic '1', the start bit.
        ani     0x20                    
        jz      _GTTYWait                                               ; done !
        dly     baud110delay/2                                          ; go to middle of start pulse.
        ldi     8                                                       ; read this many bits.
        st      -1(p2)
_GTTYLoop:
        ldi     0                                                       ; go to the middle of the next pulse.
        dly     baud110delay
        csa                                                             ; read it in.
        ani     0x20                                                    ; mask out SB.
        jz      _GTTYSkipSet
        ldi     0x1 
_GTTYSkipSet:                                                           ; it is now 0 or 1.
        ccl
        ade                                                             ; E = E * 2 + A (e.g. shift the bit in.)
        ade
        xae                                                             ; put it back in E
        dld     -1(p2)                                                  ; do it 8 times
        jnz     _GTTYLoop                                       
        dly     baud110delay*5/2                                        ; ignore any stop bits and allow short delay
        xae                                                             ; get the result
        xppc    p3                                                      ; return
        jmp     BIOSGetTTY                                              ; get the TTY

; *******************************************************************************************************************************
;
;                                            Write byte A to the UART, preserves P1
;
; *******************************************************************************************************************************

BIOSPutART:
        xae                                                             ; save byte in E
        ldi     uartPortBase / 256                                      ; save P1, set it to point to the UART
        xpah    p1                                                      
        st      @-1(p2)
        ldi     uartPortBase & 255
        xpal    p1 
        st      @-1(p2)
_PARWait:
        ld      uartStatusWordEnabled(p1)                               ; wait for the status word to indicate we can send data.
        ani     uartSWETransmitBufferEmpty 
        jz      _PARWait
        lde                                                             ; restore data from E
        st      uartDataStrobe(p1)                                      ; send it by writing to the UART
        ld      @1(p2)                                                  ; restore P1
        xpal    p1
        ld      @1(p2)
        xpal    p1
        xppc    p3
        jmp     BIOSPutArt

        xppc    p3

;       
;       Currently there are 6 bytes free in the First ROM (Monitor Code mostly) and 15 in the second ROM (Routines). Code space
;       can be moved around by shifting UART and TTY between the ROMs.
;
;       Possible savings:
;       
;       Shorten the status message
;       Lose the intro message.
;       Check some sequential calls where P3.H may not change.
;       Make some less important routines not re-entrant (UART/TTY) and ReadHexadecimalSet (never reentered monitor routine)
;       Roll rather than scroll the display (e.g. return to top and blank next line on CR)
;       Remove check for =/? before asking for 0-7
;       Disable check on 0-7 test, make it just and 7 (= and ?)
;       Remove keyboard debouncing code.
;       Remove UART error checking code.
;
;       Disable either the TTY or UART. The review says it documents the monitor routines, but it says the 20ma loop can't be figured out
;       did those routines get removed ??
;
;       Fitting this in to 1k ROM is an achievement. Though tightness would wonder why have "SCRUMPI 3 STATUS DUMP" text "CPU" or even 
;       nothing at all besides the CLS (additionally, if you did this you could dump the whole stack !)
;
;       There are some odd shortcuts ; if you type a command that is not L or C in Implant, then it asks for the address even if it is an
;       invalid command. Checking for G,H,I would take us over the 1k.
;
;       You cannot jump to 0 to restart it because of assumptions it makes about registers P2/3 being zero.
;
;       There are two code bits reliant on the 12 bit adding 4 bit static code. The Stack set up at the start, and in the dump routine
;       where it shifts P1 from pointing to the BIOSPrintInline routine to pointing to the registers.
;
;       Both have the same error in the simplified offseting, that they will work but convert it from $a0xx to $bFxx rather than
;       $aFxx which would happen with the 12 bit adding. (b = a - 1)
;
;       Another similar problem is in the read/write of the RAM/ROM. On paper this code (at $70xx) should access $7Fxx when using negative
;       offsets, but it doesn't, it accesses $6Fxx.
;
;       This only matters when the memory map is fully decoded. It will have to be a compiler option. On a 1k machine it doesn't matter, the
;       4 most significant bits are basically ignored, unless you test explicitly for them (I don't). If you expand ROM or RAM and don't fix
;       this, e.g. put the units ONLY in page 7 it will not work properly.
;
;       One other workround is to decode the lower 3.75k only (e.g. the ROM and the RAM) and have the upper 1/4k (the I/O port) in all pages.
;       

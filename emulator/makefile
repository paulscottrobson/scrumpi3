ifeq ($(OS),Windows_NT)
CCOPY = copy
CMAKE = mingw32-make
CDEL = del /Q
CDELQ = >NUL
APPSTEM = .exe
S = \\
CXXFLAGS = -IC:$(S)sdl2$(S)include$(S)SDL2 -Iframework -I . -I .$(S)include -I .$(S)bin -I .$(S)6502 -DINCLUDE_DEBUGGING_SUPPORT -DWINDOWS
LDFLAGS = -LC:$(S)sdl2$(S)lib -lmingw32 -lSDL2main -lSDL2
else
CCOPY = cp
CDEL = rm -f
CDELQ = 
CMAKE = make
APPSTEM =
S = /
SDL_CFLAGS := $(shell sdl2-config --cflags)
SDL_LDFLAGS := $(shell sdl2-config --libs)
CXXFLAGS := $(SDL_CFLAGS) -O2 -DLINUX -DINCLUDE_DEBUGGING_SUPPORT \
			-I. -I .$(S)include -I.$(S)framework -I$(S)usr$(S)include$(S)SDL2 -I.$(S)6502 -I.$(S)bin
LDFLAGS := $(SDL_LDFLAGS)
endif

# ***********************************************************************************
#
#											Common Build
#
# ***********************************************************************************

APPNAME = scrumpi3$(APPSTEM)

SOURCES = 	framework$(S)main.o framework$(S)gfx.o framework$(S)debugger.o \
			src$(S)sys_processor.o \
			src$(S)hardware_kbd.o src$(S)hardware_uart.o src$(S)hardware_8154.o \
			src$(S)sys_debug_scrumpi.o
  
CC = g++

.PHONY: all clean emulator prebuild final

all: emulator

emulator: $(APPNAME)

%.o:%.cpp
	$(CC) $(CXXFLAGS) -c -o $@ $<
	
clean:
	$(CDEL) $(APPNAME) 
	$(CDEL) src$(S)*.o 
	$(CDEL) framework$(S)*.o


$(APPNAME): $(SOURCES)
	$(CC) $(SOURCES) $(LDFLAGS) -o $@
	




CC      = gcc
EXE	= 
CFLAGS  = -g -O2 -DCOLOUR -DDEBUGGABLE -D_THREAD_SAFE -I/usr/local/include/SDL2 -I/usr/X11R6/include  -I/usr/X11/include -DHAVE_OPENGL -g
LIBS	=  -L/usr/local/lib -lSDL2 

TARGETS = scrumpi3$(EXE)

all: $(TARGETS)

scrumpi3$(EXE): main.c system.c core.c debugger.c core_sdl.c core_uart.c core_8154.c
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)


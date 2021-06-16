cd ../Generation
lua process.lua
cp *.h ../Emulator

cd ../Fonts
lua genfont.lua
cp *.h ../Emulator

cd ../BIOS
asl -q -L bios.asm
p2bin -r 0-2047 bios.p
lua bintoh.lua
cp *.h ../Emulator

cd ../Emulator
touch *.c
make

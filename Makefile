EE_BIN = 8bitcopy.elf
EE_OBJS = 8bitcopy.o
EE_LIBS = -lkernel -ldma -lgraph -ldraw

all: $(EE_BIN)

clean:
	rm -f $(EE_OBJS) $(EE_BIN)

run: $(EE_BIN)
	ps2client execee host:$(EE_BIN)

wsl: $(EE_BIN)
	$(PCSX2) -elf "$(shell wslpath -w $(shell pwd))/$(EE_BIN)"

emu: $(EE_BIN)
	$(PCSX2) -elf "$(shell pwd)/$(EE_BIN)"

reset:
	ps2client reset
	ps2client netdump

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal

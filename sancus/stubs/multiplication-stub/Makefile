include ../Makefile.include

SOURCES         = $(shell ls *.c)
OBJECTS         = $(SOURCES:.c=.o)

TARGET          = main.elf
TARGET_NO_MAC   = no_mac_$(TARGET)

all: $(TARGET)

$(TARGET_NO_MAC): $(OBJECTS)
	$(LD) $(LDFLAGS) -lsancus-step  -o $@ $^

$(TARGET): $(TARGET_NO_MAC)
	$(SANCUS_CRYPTO) --fill-macs $(CRYPTOFLAGS) -o $@ $<

load: $(TARGET)
	$(SANCUS_LOAD) $(LOADFLAGS) $<

# redirect output to file and stdout (apt-get install expect)
# https://stackoverflow.com/a/7219252
sim: $(TARGET)
	unbuffer $(SANCUS_SIM) $(SIMFLAGS) $< | tee sim.out
	./leak-mul.py sim.out

clean:
	$(RM) $(TARGET) $(TARGET_NO_MAC) $(OBJECTS)
	rm -f sim-input.bin sim-output.bin
	rm -f *.fst *.vcd

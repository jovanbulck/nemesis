SANCUS_SUPPORT_DIR = /usr/local/share/sancus-support
DEVICE             = /dev/ttyUSB0
VENDOR_KEY         = 4078d505d82099ba

CC                 = sancus-cc
LD                 = sancus-ld
CRYPTO             = sancus-crypto
LOAD               = sancus-loader
RM                 = rm -f

CFLAGS             = -I$(SANCUS_SUPPORT_DIR)/include/ --verbose -O1 #-Os

MEM_SIZE_FLAGS      = --ram-size 24K --rom-size 32K
SIM_FLAGS           = $(MEM_SIZE_FLAGS)

LDFLAGS            = --standalone --verbose $(MEM_SIZE_FLAGS) #--debug
LIBS               = -L$(SANCUS_SUPPORT_DIR)/lib -ldev-uart -ldev-timer -ldev-pmodkypd -lsm-io
CRYPTOFLAGS        = --key $(VENDOR_KEY) --debug --fill-macs
LOADFLAGS          = -device $(DEVICE) -baudrate 115200

NAME               = keypad
OBJECTS            = main.o sm-pin.o keypad_mmio.o 
TARGET             = $(NAME).elf
TARGET_NO_MACS     = $(TARGET)-no-macs

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(TARGET): $(TARGET_NO_MACS)
	$(CRYPTO) $(CRYPTOFLAGS) -o $@ $<

$(TARGET_NO_MACS): $(OBJECTS)
	@echo "========== NOW LINKING =========="
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

.PHONY: load
load: $(TARGET)
	$(MAKE) --always-make
	$(LOAD) $(LOADFLAGS) $<

.PHONY: sim
sim: $(TARGET)
	sancus-sim $(SIM_FLAGS) $(TARGET)

.PHONY: clean
clean:
	$(RM) $(TARGET) $(TARGET_NO_MACS) $(OBJECTS)
	$(RM) *.fst *.bin

PRJ    = optiboot

OBJ        = $(PRJ).o
OPTIMIZE = -Os -fno-inline-small-functions -fno-split-wide-types# -mshort-calls

CC         = avr-gcc

# Override is only needed by avr-lib build system.

CFLAGS        = -g -Wall $(OPTIMIZE) -mmcu=$(MCU) -DF_CPU=$(AVR_FREQ) $(DEFS)
LDFLAGS       = $(LDSECTIONS) -Wl,--relax -Wl,--gc-sections -nostartfiles -nostdlib

OBJCOPY        = $(GCCROOT)avr-objcopy
OBJDUMP        = $(call fixpath,$(GCCROOT)avr-objdump)

SIZE           = $(GCCROOT)avr-size

TARGET = $(PRJ).hex
MCU = atmega328p
CFLAGS += '-DLED_START_FLASHES=3' '-DBAUD_RATE=115200'
AVR_FREQ = 16000000L
LDSECTIONS  = -Wl,--section-start=.text=0x7be4

AVRDUDE = sudo avrdude -C avrdude_gpio.conf -c pi_1 -p $(MCU)

LFU = 0xFF
HFU = 0xD6
EFU = 0x05
ULCK = 0x3F
LCK = 0x0F

all : $(PRJ).hex

$(PRJ).hex: $(PRJ).elf
	$(OBJCOPY) -j .text -j .data -j .version --set-section-flags .version=alloc,load -O ihex $< $@

$(PRJ).elf: $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)
	$(SIZE) $@

clean:
	rm -rf *.o *.elf *.lst *.map *.sym *.lss *.eep *.srec *.bin *.hex

flash: all
	$(AVRDUDE) -U lock:w:$(ULCK):m 
	$(AVRDUDE) -U flash:w:$(PRJ).hex:i
	$(AVRDUDE) -U lock:w:$(LCK):m 

# write fuses to mcu
fuse:
	$(AVRDUDE) -U lfuse:w:$(LFU):m -U hfuse:w:$(HFU):m -U efuse:w:$(EFU):m
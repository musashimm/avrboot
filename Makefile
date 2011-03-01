PRG			= avrboot
SRCS_DIR 	= software

vpath %.c $(SRCS_DIR)
vpath %.h $(SRCS_DIR)
vpath %.o $(SRCS_DIR)
vpath %.elf $(SRCS_DIR)
vpath %.lst $(SRCS_DIR)

#_OBJS         = hd44780_lite.o usart_lite.o lcd_bootloader.o
_OBJS         = usart_lite.o hd44780_lite.o avrboot.o
OBJS = $(patsubst %,$(SRCS_DIR)/%,$(_OBJS))

#MCU_TARGET     = at90s2313
#MCU_TARGET     = at90s2333
#MCU_TARGET     = at90s4414
#MCU_TARGET     = at90s4433
#MCU_TARGET     = at90s4434
#MCU_TARGET     = at90s8515
#MCU_TARGET     = at90s8535
#MCU_TARGET     = atmega128
#MCU_TARGET     = atmega1280
#MCU_TARGET     = atmega1281
#MCU_TARGET     = atmega16
#MCU_TARGET     = atmega163
#MCU_TARGET     = atmega164p
#MCU_TARGET     = atmega165
#MCU_TARGET     = atmega165p
#MCU_TARGET     = atmega168
#MCU_TARGET     = atmega169
#MCU_TARGET     = atmega169p
MCU_TARGET     = atmega32
#MCU_TARGET     = atmega324p
#MCU_TARGET     = atmega325
#MCU_TARGET     = atmega3250
#MCU_TARGET     = atmega329
#MCU_TARGET     = atmega3290
#MCU_TARGET     = atmega48
#MCU_TARGET     = atmega64
#MCU_TARGET     = atmega640
#MCU_TARGET     = atmega644
#MCU_TARGET     = atmega644p
#MCU_TARGET     = atmega645
#MCU_TARGET     = atmega6450
#MCU_TARGET     = atmega649
#MCU_TARGET     = atmega6490
#MCU_TARGET     = atmega8
#MCU_TARGET     = atmega8515
#MCU_TARGET     = atmega8535
#MCU_TARGET     = atmega88
#MCU_TARGET     = attiny2313
#MCU_TARGET     = attiny24
#MCU_TARGET     = attiny25
#MCU_TARGET     = attiny26
#MCU_TARGET     = attiny261
#MCU_TARGET     = attiny44
#MCU_TARGET     = attiny45
#MCU_TARGET     = attiny461
#MCU_TARGET     = attiny84
#MCU_TARGET     = attiny85
#MCU_TARGET     = attiny861
OPTIMIZE       = -O2
BOOTSTART 	   = 0x7800

DEFS           =
LIBS           =

# avrdude settings
AVRDUDE_MCU_TARGET = m32
AVRDUDE_PROGRAMMER_TYPE = stk200
AVRDUDE_PORT = /dev/parport0
AVRDUDE_HEX_FILE = $(PRG).hex

# You should not have to change anything below here.

CC             = avr-gcc

# Override is only needed by avr-lib build system.

override CFLAGS        = -g -Wall $(OPTIMIZE) -mmcu=$(MCU_TARGET) $(DEFS)
override LDFLAGS       = -Wl,--section-start=.text=$(BOOTSTART),-Map,$(SRCS_DIR)/$(PRG).map

OBJCOPY        = avr-objcopy
OBJDUMP        = avr-objdump

EEPROM_DATE = $(shell date +"%Y%m%d_%H%M%S")
EEPROM_DUMP = eeprom_$(EEPROM_DATE).dump
EEPROM_LAST_DUMP = eeprom_last.dump

all: $(SRCS_DIR)/$(PRG).elf lst text info load

%.c: %.h
	touch $@

$(OBJS):  %.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(SRCS_DIR)/$(PRG).elf: $(OBJS)
	@$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -rfv $(SRCS_DIR)/*.o $(SRCS_DIR)/$(PRG).elf $(SRCS_DIR)/*.eps $(VSRCS_DIR)/*.png $(SRCS_DIR)/*.pdf $(SRCS_DIR)/*.bak
	rm -rfv $(SRCS_DIR)/*.lst $(SRCS_DIR)/*.map $(EXTRA_CLEAN_FILES)

lst: $(PRG).lst

%.lst: %.elf
	$(OBJDUMP) -h -S $< > $(SRCS_DIR)/$@

# Rules for building the .text rom images

text: hex bin srec

hex:  $(PRG).hex
bin:  $(PRG).bin
srec: $(PRG).srec

%.hex: %.elf
	$(OBJCOPY) -j .text -j .data -O ihex $< $(SRCS_DIR)/$@

%.srec: %.elf
	$(OBJCOPY) -j .text -j .data -O srec $< $(SRCS_DIR)/$@

%.bin: %.elf
	$(OBJCOPY) -j .text -j .data -O binary $< $(SRCS_DIR)/$@

# Rules for building the .eeprom rom images

eeprom: ehex ebin esrec

ehex:  $(PRG)_eeprom.hex
ebin:  $(PRG)_eeprom.bin
esrec: $(PRG)_eeprom.srec

%_eeprom.hex: %.elf
	$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=100 -O ihex $< $@ \
	|| { echo empty $@ not generated; exit 0; }

%_eeprom.srec: %.elf
	$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O srec $< $@ \
	|| { echo empty $@ not generated; exit 0; }

%_eeprom.bin: %.elf
	$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O binary $< $@ \
	|| { echo empty $@ not generated; exit 0; }

# Every thing below here is used by avr-libc's build system and can be ignored
# by the casual user.

FIG2DEV                 = fig2dev
EXTRA_CLEAN_FILES       = $(SRCS_DIR)/*.hex $(SRCS_DIR)/*.bin $(SRCS_DIR)/*.srec

dox: eps png pdf

eps: $(PRG).eps
png: $(PRG).png
pdf: $(PRG).pdf

%.eps: %.fig
	$(FIG2DEV) -L eps $< $@

%.pdf: %.fig
	$(FIG2DEV) -L pdf $< $@

%.png: %.fig
	$(FIG2DEV) -L png $< $@

docs: FORCE
	rm -fr docs
	doxygen Doxyfile

FORCE:


info:
	@echo " "
	ls -la $(SRCS_DIR)/$(PRG).bin
	@echo " "

load:
	avrdude -p $(AVRDUDE_MCU_TARGET) -c $(AVRDUDE_PROGRAMMER_TYPE) -P $(AVRDUDE_PORT) -U flash:w:$(SRCS_DIR)/$(AVRDUDE_HEX_FILE) -E noreset

eeprom_save:
	@echo $(EEPROM_DUMP)
	avrdude -E noreset -p m32 -c stk200 -P /dev/parport0 -y -u -U eeprom:r:eeprom/$(EEPROM_DUMP):r
	cp -f eeprom/$(EEPROM_DUMP) eeprom/$(EEPROM_LAST_DUMP)

eeprom_load:
	avrdude -E noreset -p m32 -c stk200 -P /dev/parport0 -y -u -U eeprom:w:eeprom/$(EEPROM_LAST_DUMP):r

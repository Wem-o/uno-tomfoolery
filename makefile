SRC_FILES = src/*.c

# For genuine arduinos
# PORT = /dev/ttyACM0

# For non genuine arduinos
PORT = /dev/ttyUSB0
BAUDRATE = 250000

MCU = atmega328p
CPU_FREQ = 16000000UL

target/output.hex: target/output.elf
	avr-objcopy -j .text -j .data -O ihex $< $@

target/output.elf: $(SRC_FILES) makefile
	mkdir -p target
	avr-gcc -Wall -Os -DF_CPU=$(CPU_FREQ) -mmcu=$(MCU) -iquote includes \
	  -o $@ $(SRC_FILES)


upload: target/output.hex
	avrdude -p $(MCU) -b 115200 -c arduino -P $(PORT) -U flash:w:target/output.hex:i

minicom: target/output.hex upload
	minicom -b $(BAUDRATE) -D $(PORT)

clear:
	rm -rf target

.PHONY: upload clear minicom
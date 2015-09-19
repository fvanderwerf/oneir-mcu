
CC=avr-gcc

all: oneir.bin


oneir.hex: oneir.elf
	avr-objcopy -j .text -j .data -O ihex $< $@
	avr-size --format=avr --mcu=attiny $<

oneir.bin: oneir.hex
	avr-objcopy -I ihex -O binary $< $@



oneir.elf: oneir.c smbus.c ir.c
	avr-gcc -std=c99 -Wall -Os -mmcu=attiny45 -o $@ $^

.PHONY: all

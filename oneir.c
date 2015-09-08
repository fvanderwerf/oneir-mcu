
#define F_CPU 8000000

#include <util/delay.h>
#include <avr/interrupt.h>

#include "smbus.h"

#define SMBUS_ADDR 0x10

struct smbus_message message;


int main(void)
{
    DDRB |= _BV(IRLED);
    smbus_init(SMBUS_ADDR);
    sei();

    while(1)
    {
        smbus_receive(&message);
        if (message.address == 0xa5 && message.code == 0xc3)
            PORTB ^= _BV(IRLED);
    }

    return 0;
}

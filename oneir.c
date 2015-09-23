
#define F_CPU 8000000

#include <util/delay.h>
#include <avr/interrupt.h>

#include "smbus.h"
#include "ir.h"

#define SMBUS_ADDR 0x10

struct smbus_message message;


int main(void)
{
    ir_init();
    smbus_init(SMBUS_ADDR);
    sei();

    while(1)
    {
        smbus_receive(&message);
        ir_send(IR_RC5, message.address, message.code);
    }

    return 0;
}

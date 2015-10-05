
#define F_CPU 8000000

#include <util/delay.h>
#include <avr/interrupt.h>

#include "i2c.h"
#include "ir.h"

#define I2C_ADDR 0x10

struct i2c_message message;


int main(void)
{
    ir_init();
    i2c_init(I2C_ADDR);
    sei();

    while(1)
    {
        i2c_receive(&message);
        ir_send_cmd(IR_RC5, message.address, message.code);
    }

    return 0;
}

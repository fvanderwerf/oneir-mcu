
#define F_CPU 8000000

#include <util/delay.h>
#include <avr/interrupt.h>

#include "i2c.h"
#include "rc5.h"
#include "ir.h"
#include "command.h"

#define I2C_ADDR 0x10

struct i2c_message message;

struct ir_command command;

int main(void)
{
    ir_init();
    i2c_init(I2C_ADDR);
    sei();

    while(1)
    {
        i2c_receive(&message);

        command.type = RC5;
        command.rc5.address = message.address;
        command.rc5.code = message.code;

        rc5_to_raw(&command);

        ir_send(command.raw.fcarrier, command.raw.duty_cycle, command.raw.pattern);
    }

    return 0;
}

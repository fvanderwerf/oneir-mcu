
#define F_CPU 8000000

#include <util/delay.h>
#include <avr/interrupt.h>

#include "i2c.h"
#include "rc5.h"
#include "ir.h"
#include "command.h"

#define I2C_ADDR 0x10

struct ir_command command;

int main(void)
{
    ir_init();
    i2c_init(I2C_ADDR);
    sei();

    while(1)
    {
        i2c_receive(&command);

        if (command.type == IR_RC5)
            rc5_to_raw(&command);

        if (command.type == IR_RAW)
            ir_send(command.raw.fcarrier, command.raw.duty_cycle, command.raw.pattern);
    }

    return 0;
}

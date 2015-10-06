
#ifndef I2C_H
#define I2C_H

#include "command.h"

#include <stdint.h>

#define IRLED PB1


void i2c_init(uint8_t i2c_address);

void i2c_receive(struct ir_command *command);


#endif /* I2C_H */



#ifndef I2C_H
#define I2C_H

#include <stdint.h>


#define IRLED PB1


struct i2c_message {
    uint8_t address;
    uint8_t code;
};

void i2c_init(uint8_t i2c_address);

void i2c_receive(struct i2c_message *message);


#endif /* I2C_H */


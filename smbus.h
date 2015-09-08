
#ifndef SMBUS_H
#define SMBUS_H

#include <stdint.h>


#define IRLED PB1


struct smbus_message {
    uint8_t address;
    uint8_t code;
};

void smbus_init(uint8_t smbus_address);

void smbus_receive(struct smbus_message *message);


#endif /* SMBUS_H */



#ifndef IR_H
#define IR_H

#include <stdint.h>

enum ir_protocol {
    IR_RC5
};

void ir_init(void);

void ir_send(uint16_t carrier, uint8_t duty, uint8_t *pattern);

void ir_send_cmd(enum ir_protocol protocol, uint16_t address, uint16_t code);


#endif /* IR_H */


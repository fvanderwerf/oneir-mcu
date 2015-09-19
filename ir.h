
#ifndef IR_H
#define IR_H

#include <stdint.h>

void ir_init(void);

void ir_send(uint8_t address, uint8_t code);

#endif /* IR_H */


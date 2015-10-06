
#ifndef IR_H
#define IR_H

#include <stdint.h>

void ir_init(void);

void ir_send(uint16_t carrier, uint8_t duty, uint8_t *pattern);

#endif /* IR_H */


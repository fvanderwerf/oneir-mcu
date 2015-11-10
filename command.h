
#ifndef COMMAND_H
#define COMMAND_H

#include <stdint.h>

#define IR_RAW 0
#define IR_RC5 1

struct ir_command {
    uint8_t type;
    union {
        struct {
            uint16_t fcarrier;
            uint8_t duty_cycle;
            uint8_t pattern[100];
        } raw;

        struct {
            uint8_t address;
            uint8_t code;
        } rc5;
    };
};

#endif /* COMMAND_H */

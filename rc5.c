
#include "rc5.h"

static const uint8_t rc5_pulses_per_phase = 32;

static void ir_pattern_append(int duty, int reps, uint8_t **pattern)
{
    while (reps > 0x7F) {
        **pattern = (duty ? 0x80 : 0x00 ) | 0x7F;

        reps -= 0x7F;

        (*pattern)++;
    }

    **pattern = (duty ? 0x80 : 0x00 ) | reps;

    (*pattern)++;
} 

static void ir_write_rc5_bit(int bit, uint8_t **pattern)
{
    ir_pattern_append(!bit, rc5_pulses_per_phase, pattern);
    ir_pattern_append(bit,  rc5_pulses_per_phase, pattern);
}

void rc5_to_raw(struct ir_command *command)
{
    static int toggle = 0;

    int j;

    uint8_t address = command->rc5.address;
    uint8_t code = command->rc5.code;
    uint8_t *_pattern, **pattern;

    command->type = RAW;
    command->raw.fcarrier = 36000;
    command->raw.duty_cycle = 27;

    _pattern = command->raw.pattern;
    pattern = &_pattern;

    /* start bit */
    ir_write_rc5_bit(1, pattern);

    /* field bit */
    ir_write_rc5_bit(code < 64, pattern);

    /* toggle bit */
    toggle = 1 - toggle;
    ir_write_rc5_bit(toggle, pattern);

    /* address */
    for (j = 0; j < 5; j++)
        ir_write_rc5_bit((address >> (4 - j)) & 0x01, pattern);

    /* code */
    for (j = 0; j < 6; j++)
        ir_write_rc5_bit((code >> (5 - j)) & 0x01, pattern);

    /* final code */
    **pattern = 0;
}

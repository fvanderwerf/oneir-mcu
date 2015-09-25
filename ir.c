
#include "ir.h"

#include <avr/io.h>
#include <avr/interrupt.h>

#define IRLED PB1

#define DUTY_CYCLE 60

#define PULSES_PER_SYMBOL 32

#define IR_CLOCK_DIV (_BV(CS12))

#define IR_ENABLE_CLOCK()       \
    do {                        \
        TCCR1 |= IR_CLOCK_DIV;  \
    } while(0)

#define IR_DISABLE_CLOCK()      \
    do {                        \
        TCCR1 &= 0xF0;          \
    } while(0)


struct pattern {
    int duty;
    int reps;
};

static struct pattern ir_pattern[100];
static struct pattern *ir_pattern_current;
static struct pattern *ir_pattern_end;


inline void init_pll(void)
{
    /* enable pll */
    PLLCSR |= 1 << PLLE;

    /* wait for pll to lock */
    while (!(PLLCSR & _BV(PLOCK)))
        ;
}


inline void init_timer1(void)
{
    PLLCSR |= _BV(PCKE); // set PLL as source for timer 1
    TCCR1 = _BV(CTC1) | _BV(PWM1A) | _BV(COM1A1);  //clear timer1 on ocr1c match, enable pwm1a, pwm1a mode: set on 0, clear on match, clock is disabled
    OCR1A = 0;           // 0% dutycycle
    OCR1C = 255;         // non zero to avoid overflow interrupt

    TIFR |= _BV(TOV1);   // clear overflow interrupt if there is one pending
    TIMSK |= _BV(TOIE1); // enable timer1 overflow interrupt

}

void ir_init(void)
{
    DDRB |= _BV(IRLED);
    init_pll();
    init_timer1();
}


ISR(TIM1_OVF_vect)
{
    if (ir_pattern_current < ir_pattern_end) {
        OCR1A = ir_pattern_current->duty;

        if (--ir_pattern_current->reps == 0)
            ir_pattern_current++;
    } else {
        IR_DISABLE_CLOCK();
    }
}

static void ir_reset(void)
{
    IR_DISABLE_CLOCK();

    ir_pattern_end = ir_pattern_current = ir_pattern;

    TCNT1 = 0;
    OCR1A = 0;
    OCR1C = 221;          // 64MHz divided by 8 = 8Mhz, compare match clear at 221, --> 8Mhz / (221 + 1) = 36.0kHz

    GTCCR |= _BV(PSR1); //reset prescaler
    TIFR |= _BV(TOV1); //reset any pending interrupt
}


static void ir_pattern_append(int duty, int reps)
{
    ir_pattern_end->duty = duty;
    ir_pattern_end->reps = reps;

    ir_pattern_end++;
} 

static void ir_write_rc5_bit(int bit)
{
    if (bit == 0) {
        ir_pattern_append(DUTY_CYCLE, PULSES_PER_SYMBOL);
        ir_pattern_append(0, PULSES_PER_SYMBOL);
    } else {
        ir_pattern_append(0, PULSES_PER_SYMBOL);
        ir_pattern_append(DUTY_CYCLE, PULSES_PER_SYMBOL);
    }
}

static void ir_write_rc5_pattern(uint16_t address, uint16_t code)
{
    static int toggle = 0;
    int j;

    /* start bit */
    ir_write_rc5_bit(1);

    /* field bit */
    ir_write_rc5_bit(code < 64);

    /* toggle bit */
    toggle = 1 - toggle;
    ir_write_rc5_bit(toggle);

    /* address */
    for (j = 0; j < 5; j++)
        ir_write_rc5_bit((address >> (4 - j)) & 0x01);

    /* code */
    for (j = 0; j < 6; j++)
        ir_write_rc5_bit((code >> (5 - j)) & 0x01);

}

void ir_send_cmd(enum ir_protocol protocol, uint16_t address, uint16_t code)
{
    ir_reset();

    switch(protocol) {
        case IR_RC5:
            ir_write_rc5_pattern(address, code);
            break;
        default:
            return;
    }

    IR_ENABLE_CLOCK();
}


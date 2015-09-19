
#include "ir.h"

#include <avr/io.h>
#include <avr/interrupt.h>

#define IRLED PB1

#define PULSES_PER_SYMBOL 32

static enum ir_state {
    IR_NONE,
    IR_INIT,
    IR_SEND,
    IR_DONE
} ir_state = IR_NONE;

// RC-5 TV '0'
static int ir_pattern_ocra[]   = {  0, 60,  0, 60, 60,  0, 60,  0, 60,  0, 60,  0, 60,  0, 60,  0, 60,  0, 60,  0, 60,  0, 60,  0, 60,  0, 60,  0 };
static int ir_pattern_length = sizeof(ir_pattern_ocra)/sizeof(int);
static int ir_pattern_index;
static int ir_pulsecountdown;

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
    OCR1C = 221;         // 64MHz divided by 8 = 8Mhz, compare match clear at 221, --> 8Mhz / (221 + 1) = 36.0kHz

    TIFR |= _BV(TOV1);   // clear overflow interrupt if there is one pending
    TIMSK |= _BV(TOIE1); // enable timer1 overflow interrupt

}

void ir_init(void)
{
    ir_state = IR_NONE;
    DDRB |= _BV(IRLED);
    init_pll();
    init_timer1();
}


ISR(TIM1_OVF_vect)
{
    switch(ir_state) {
        case IR_INIT:
            OCR1A = ir_pattern_ocra[ir_pattern_index];
            ir_pulsecountdown = PULSES_PER_SYMBOL;
            ir_state = IR_SEND;
            break;
        case IR_SEND:
            ir_pulsecountdown--;
            if (ir_pulsecountdown == 0) {
                ir_pattern_index++;
                if (ir_pattern_index >= ir_pattern_length) {
                    ir_state = IR_DONE;
                    OCR1A = 0;
                } else {
                    OCR1A = ir_pattern_ocra[ir_pattern_index];
                    ir_pulsecountdown = PULSES_PER_SYMBOL;
                }
            }
            break;
        case IR_DONE:
        case IR_NONE:
        default:
            ir_state = IR_NONE;
            TCCR1 &= 0xF0; //disable clock
            break;
    }
}

void send_code(void)
{
    ir_state = IR_INIT;
    ir_pattern_index = 0;

    TCNT1 = 0;
    OCR1A = 0;
    GTCCR |= _BV(PSR1); //reset prescaler
    TIFR |= _BV(TOV1); //reset any pending interrupt
    TCCR1 |= _BV(CS12); //enable clock
    //TCCR1 |= _BV(CS13) | _BV(CS12) | _BV(CS11) | _BV(CS10); //enable clock
}

void ir_write_pattern(int value, int *offset)
{
    if (value == 0) {
        ir_pattern_ocra[(*offset)++] = 60;
        ir_pattern_ocra[(*offset)++] = 0;
    } else {
        ir_pattern_ocra[(*offset)++] = 0;
        ir_pattern_ocra[(*offset)++] = 60;
    }
}

void ir_send(uint8_t address, uint8_t code)
{
    int i = 0;
    int j;
    static int toggle = 0;

    /* start bit */
    ir_write_pattern(1, &i);

    /* field bit */
    if (code < 64)
        ir_write_pattern(1, &i);
    else
        ir_write_pattern(0, &i);

    /* toggle bit */
    toggle = 1 - toggle;
    ir_write_pattern(toggle, &i);

    /* address */
    for (j = 0; j < 5; j++)
        ir_write_pattern((address >> (4 - j)) & 0x01, &i);

    /* code */
    for (j = 0; j < 6; j++)
        ir_write_pattern((code >> (5 - j)) & 0x01, &i);

    send_code();
}

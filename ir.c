
#include "ir.h"

#include <avr/io.h>
#include <avr/interrupt.h>

#define IRLED PB1


#define PULSES_PER_SYMBOL 32

static int ir_clkdiv = _BV(CS12);
static int ir_duty = 60;

static uint8_t *ir_pattern;


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
}

void ir_init(void)
{
    DDRB |= _BV(IRLED);
    init_pll();
    init_timer1();
}

void ir_start_clock()
{
    OCR1A = 0;
    TCNT1 = 0;
    GTCCR |= _BV(PSR1); //reset prescaler
    TIFR |= _BV(TOV1); //reset any pending interrupt
    TIMSK |= _BV(TOIE1); // enable timer1 overflow interrupt
    TCCR1 |= ir_clkdiv;
}

void ir_stop_clock()
{
    TIMSK &= ~_BV(TOIE1); // disable timer1 overflow interrupt
    OCR1A = 0;
    TCCR1 &= 0xF0;
}


ISR(TIM1_OVF_vect)
{
    if (*ir_pattern) {
        OCR1A = (*ir_pattern & 0x80) ? ir_duty : 0;

        *ir_pattern -= 1;
        
        if ((*ir_pattern & 0x7F) == 0)
            ir_pattern++;
    } else {
        ir_stop_clock();
    }
}


static void ir_set_clock(uint16_t f)
{
    int i;
    //ir_clkdiv = 4;

    for (i = 0; i < 15; i++) {
        // clkdivider is 1 << i

        // check if this divider is large enough
        // check if Fpck / clkdiv / 256 < f  -- 256 -> range of counter
        //          64MHz/ (1<<i) / 256 < f
        if (f > (250000L >> i)) {
            //ir_clkdiv = 4;
            ir_clkdiv = i + 1; //value for TCCR1 CSxx, divider is offset by 1, because 0 is used to turn off clk

            //ocr0c_2 this value is twice the value of the final OCR1C,
            int ocr1c_2 = (512 * 250000L >>  i) / f;
            // now we can use the "bit after the comma" from ocr1c_2 for proper rounding
            OCR1C = (ocr1c_2 >> 1) + (ocr1c_2 & 0x01) - 1; // -1, because the timer run up to and including this register
            break;
        }
    }
}

static void ir_config(uint16_t carrier, uint8_t duty)
{
    //OCR1C = 221;          // 64MHz divided by 8 = 8Mhz, compare match clear at 221, --> 8Mhz / (221 + 1) = 36.0kHz
    ir_set_clock(carrier);

    uint16_t _duty = OCR1C;
    _duty *= duty;
    _duty /= 100;
    ir_duty = _duty;
}

void ir_send(uint16_t carrier, uint8_t duty, uint8_t *pattern)
{
    ir_stop_clock();

    ir_config(carrier, duty);

    ir_pattern = pattern;

    ir_start_clock();
}


#define F_CPU 8000000

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include "smbus.h"

#define SCL PB2
#define SDA  PB0

#define IRLED PB1


#define I2C_ADDR 0x10

enum i2c_state {
    CHECK_ADDRESS,
    PREPARE_READ_REGISTER,
    READ_REGISTER,
    PREPARE_READ_DATA,
    READ_DATA,
    DONE

} i2c_state;

uint8_t i2c_register;
enum {
    I2C_WRITE,
    I2C_READ
} i2c_direction;
uint16_t i2c_data;
uint8_t i2c_data_num;
int output;

void init_i2c(void)
{
    DDRB |= ( 1 << SCL ) | ( 1 << SDA );

    // set SCL high
    PORTB |= ( 1 << SCL );

    // set SDA high
    PORTB |= ( 1 << SDA );

    // Set SDA as input
    DDRB &= ~( 1 << SDA );

    USICR = _BV(USISIE) | _BV(USIWM1) | _BV(USICS1);

    // clear all interrupt flags and reset overflow counter
    USISR = _BV(USISIF) | _BV(USIOIF) | _BV(USIPF) | _BV(USIDC);
}

//i2c start condition interrupt service routine
ISR(USI_START_vect)
{
  DDRB &= ~( 1 << SDA );

  /* enable interrupts for other work to continue,
   * disable Start interrupt to avoid infinite recursing here
   */

  USICR &= ~_BV(USISIE);
  sei();

   
  while (
       // SCL his high
       ( PINB & ( 1 << PINB2 ) ) &&
       // and SDA is low
       !( ( PINB & ( 1 << PINB0 ) ) )
  );

  /* disable interrupts again and re-enable disable Start */
  cli();
  USICR |= _BV(USISIE);

  if ( !( PINB & ( 1 << PINB0 ) ) )
  {
    i2c_state = CHECK_ADDRESS;
    i2c_data_num = 0;
    USISR = 0; //reset overflow counter;
    USISR |= _BV(USIOIF);
    USICR |= _BV(USIOIE) | _BV(USIWM0);
  }

    USISR |= _BV(USISIF); // clear interrupt
}

#define SETUP_SEND_NACK()                           \
    do {                                            \
        DDRB &= ~_BV(SDA); /*set SDA as input */    \
        USISR = 14; /* set counter to send 1 bit */ \
    } while(0)

#define SETUP_SEND_ACK()                            \
    do {                                            \
        USIDR = 0; /* pull SDA low */               \
        DDRB |= _BV(SDA); /*set SDA as output */    \
        USISR = 14; /* set counter to send 1 bit */ \
    } while(0)

#define SETUP_RECEIVE_BYTE()                        \
    do {                                            \
        DDRB &= ~_BV(SDA); /* set SDA as input */   \
        USISR = 0; /* set counter to recv 8 bytes */\
    } while(0)

ISR(USI_OVF_vect)
{
    switch (i2c_state)
    {
        case CHECK_ADDRESS:
            if (USIDR >> 1 == I2C_ADDR) { 
                i2c_direction = USIDR & 0x01;

                USIDR = 0; /* pull SDA low */              
                DDRB |= _BV(SDA); /*set SDA as output */    
                USISR = 14; /* set counter to send 1 bit */ 
    
                i2c_state = PREPARE_READ_REGISTER;


            } else {
                USICR &= ~(_BV(USIOIE) | _BV(USIWM0));
                i2c_state = DONE;
            }
            break;

        case PREPARE_READ_REGISTER:
            SETUP_RECEIVE_BYTE();
            i2c_state = READ_REGISTER;
            break;

        case READ_REGISTER:
            i2c_register = USIDR;

            if (i2c_register == 0x10) {
                SETUP_SEND_ACK();
                i2c_state = PREPARE_READ_DATA;
            } else {
                SETUP_SEND_NACK();
                i2c_state = DONE;
            }
        
            break;

        case PREPARE_READ_DATA:
            SETUP_RECEIVE_BYTE();
            i2c_state = READ_DATA;
            break;

        case READ_DATA:
            i2c_data >>= 8;
            i2c_data |= ((uint16_t) USIDR) << 8;
            i2c_data_num++;

            
            SETUP_SEND_ACK();

            if (i2c_data_num == 2) {
                if (i2c_data == 0xa5c3)
                    PORTB ^= _BV(IRLED);
                i2c_state = DONE;
            } else {
                i2c_state = PREPARE_READ_DATA;
            }

            break;
        case DONE:
        default:
            DDRB &= ~(_BV(SDA));
            USICR &= ~(_BV(USIOIE) | _BV(USIWM0));
            break;
    }

    USISR |= _BV(USIOIF); //clear interrupt
}


int main(void)
{
    DDRB |= _BV(IRLED);
    init_i2c();
    sei();

#if 0
    for (i = 0; i < 4; i++) {
        PORTB |= _BV(IRLED);
    }

    while (1) {
        //_delay_ms(500);
        if (USISR & _BV(USISIF)) {
            USISR |= _BV(USISIF);
            PORTB ^= _BV(IRLED);
        }
    }
#endif
    while(1)
    {

    }

    return 0;
}

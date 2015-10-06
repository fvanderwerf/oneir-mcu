
#include "i2c.h"

#include <avr/io.h>
#include <avr/interrupt.h>

#include <stdlib.h>

#define SCL PB2
#define SDA PB0

#define SCLPIN PINB2
#define SDAPIN PINB0

enum i2c_state {
    CHECK_ADDRESS,
    PREPARE_READ_LENGTH,
    READ_LENGTH,
    PREPARE_READ_DATA,
    READ_DATA,
    DONE
};

enum {
    I2C_WRITE,
    I2C_READ
} i2c_direction;

static uint8_t address;
static uint8_t i2c_length;
volatile static uint8_t i2c_pending;
static uint8_t *i2c_data_ptr;
volatile static int i2c_data_ready;

static enum i2c_state state;

void i2c_init(uint8_t i2c_address)
{
    address = i2c_address;
    i2c_data_ptr = NULL;
    i2c_data_ready = 0;
    i2c_pending = 0;

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

//start condition interrupt service routine
ISR(USI_START_vect)
{
  DDRB &= ~( 1 << SDA );

  /* Enable interrupts for other work to continue.
   * Disable Start interrupt to avoid infinite recursion
   */
  USICR &= ~_BV(USISIE);
  sei();
   
  while (
       // SCL his high
       ( PINB & ( 1 << SCLPIN ) ) &&
       // and SDA is low
       !( ( PINB & ( 1 << SDAPIN ) ) )
  );

  /* disable interrupts again and re-enable disable Start */
  cli();
  USICR |= _BV(USISIE);

  if ( !( PINB & ( 1 << SDAPIN ) ) )
  {
    state = CHECK_ADDRESS;
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
    switch (state)
    {
        case CHECK_ADDRESS:
            if (USIDR >> 1 == address) { 
                i2c_direction = USIDR & 0x01;

                SETUP_SEND_ACK();
    
                state = PREPARE_READ_LENGTH;

            } else {
                USICR &= ~(_BV(USIOIE) | _BV(USIWM0));
                state = DONE;
            }
            break;

        case PREPARE_READ_LENGTH:
            SETUP_RECEIVE_BYTE();
            state = READ_LENGTH;
            break;

        case READ_LENGTH:
            i2c_length = USIDR;
            state = PREPARE_READ_DATA;
            SETUP_SEND_ACK();

            if (i2c_data_ptr == NULL) {
                i2c_pending = 1;
                return;
            }
            break;

        case PREPARE_READ_DATA:
            SETUP_RECEIVE_BYTE();
            state = READ_DATA;
            break;

        case READ_DATA:
            *i2c_data_ptr = USIDR;
            i2c_length--;
            i2c_data_ptr++;

            SETUP_SEND_ACK();

            if (i2c_length == 0) {
                i2c_data_ready = 1;
                i2c_data_ptr = NULL;
                state = DONE;
            } else {
                state = PREPARE_READ_DATA;
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

void i2c_receive(struct ir_command *command)
{
    i2c_data_ptr = (uint8_t *) command;

    cli();

    if (i2c_pending) {
        USISR |= _BV(USIOIF); //clear interrupt
        i2c_pending = 0;
    }

    sei();

    while (!i2c_data_ready)
        ;

    i2c_data_ready = 0;
}


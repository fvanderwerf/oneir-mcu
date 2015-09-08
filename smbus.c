
#include "smbus.h"

#include <avr/io.h>
#include <avr/interrupt.h>

#define SCL PB2
#define SDA PB0

#define SCLPIN PINB2
#define SDAPIN PINB0

enum smbus_state {
    CHECK_ADDRESS,
    PREPARE_READ_REGISTER,
    READ_REGISTER,
    PREPARE_READ_DATA,
    READ_DATA,
    DONE
};

enum {
    SMBUS_WRITE,
    SMBUS_READ
} smbus_direction;

static uint8_t address;


static enum smbus_state state;
static uint8_t smbus_register;

volatile static int smbus_data_ready;
static uint16_t smbus_data;
static uint8_t smbus_data_num;

void smbus_init(uint8_t smbus_address)
{
    address = smbus_address;
    smbus_data_ready = 0;

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
    smbus_data_num = 0;
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
                smbus_direction = USIDR & 0x01;

                USIDR = 0; /* pull SDA low */              
                DDRB |= _BV(SDA); /*set SDA as output */    
                USISR = 14; /* set counter to send 1 bit */ 
    
                state = PREPARE_READ_REGISTER;


            } else {
                USICR &= ~(_BV(USIOIE) | _BV(USIWM0));
                state = DONE;
            }
            break;

        case PREPARE_READ_REGISTER:
            SETUP_RECEIVE_BYTE();
            state = READ_REGISTER;
            break;

        case READ_REGISTER:
            smbus_register = USIDR;

            if (!smbus_data_ready && smbus_register == 0x10) {
                SETUP_SEND_ACK();
                state = PREPARE_READ_DATA;
            } else {
                SETUP_SEND_NACK();
                state = DONE;
            }
        
            break;

        case PREPARE_READ_DATA:
            SETUP_RECEIVE_BYTE();
            state = READ_DATA;
            break;

        case READ_DATA:
            smbus_data >>= 8;
            smbus_data |= ((uint16_t) USIDR) << 8;
            smbus_data_num++;

            SETUP_SEND_ACK();

            if (smbus_data_num == 2) {
                smbus_data_ready = 1;
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

void smbus_receive(struct smbus_message *message)
{
    while (!smbus_data_ready)
        ;

    message->address = (smbus_data >> 8) & 0xFF;
    message->code = smbus_data & 0xFF;
    smbus_data_ready = 0;
}


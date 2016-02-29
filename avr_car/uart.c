#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>

#include "nodeconfig.h"

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#ifndef BAUD
#define BAUD 9600
#endif
#include <util/setbaud.h>

#define UART_BUF_SIZE 64

uint8_t UART_RX_head, UART_RX_tail, UART_TX_head, UART_TX_tail;
uint8_t UART_RX_buf[UART_BUF_SIZE], UART_TX_buf[UART_BUF_SIZE];

void uart_init(void) {
    UART_RX_tail = UART_RX_head = UART_TX_tail = UART_TX_head = 0;

    UBRR0H = UBRRH_VALUE;
    UBRR0L = UBRRL_VALUE;

#if USE_2X
    UCSR0A |= _BV(U2X0);
#else
    UCSR0A &= ~(_BV(U2X0));
#endif

    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00); /* 8-bit data */
    UCSR0B = _BV(RXEN0) | _BV(TXEN0) | _BV(RXCIE0);   /* Enable RX and TX and RX complete interrupt */
}

void uart_disable(void) {
    UCSR0B = 0;
}

int uart_putchar(char c, FILE *stream __attribute__((unused)))
{
    uint8_t next_head = (UART_TX_head + 1) % UART_BUF_SIZE;

    while (next_head == UART_TX_tail ); /* wait for space */

    UART_TX_head = next_head;
    UART_TX_buf[UART_TX_head] = c;

    /* enable UDRE interrupt */
    UCSR0B |= _BV(UDRIE0);

    return 0;
}

int uart_getchar()
{
    if (UART_RX_head == UART_RX_tail) {
        return EOF;
    }

    UART_RX_tail = (UART_RX_tail + 1) % UART_BUF_SIZE;
    return UART_RX_buf[UART_RX_tail];
}

ISR(USART_RX_vect)
{
    uint8_t next_head = (UART_RX_head + 1) % UART_BUF_SIZE;

    if (next_head == UART_RX_tail) {
        /* receive buffer overflow */
    } else {
        UART_RX_head = next_head;
        UART_RX_buf[UART_RX_head] = UDR0;
    }
}

ISR(USART_UDRE_vect)
{
    if (UART_TX_head == UART_TX_tail) {
        /* buffer is empty, so disable interrupt */
        UCSR0B &= ~_BV(UDRIE0);
    } else {
        UART_TX_tail = (UART_TX_tail + 1) % UART_BUF_SIZE;
        UDR0 = UART_TX_buf[UART_TX_tail];
    }
}


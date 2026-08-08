#ifndef LIBERATION_BAR_H
#define LIBERATION_BAR_H

#include <stdint.h>

/* Consume exactly one failed number-guess attempt. */
uint8_t liberation_bar_consume_wrong_guess(uint8_t guesses_remaining);

#endif

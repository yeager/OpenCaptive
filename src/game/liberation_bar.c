#include "liberation_bar.h"

uint8_t liberation_bar_consume_wrong_guess(uint8_t guesses_remaining) {
    return guesses_remaining > 0 ? (uint8_t)(guesses_remaining - 1) : 0;
}

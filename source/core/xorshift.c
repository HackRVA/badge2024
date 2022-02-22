
/* George Marsaglia's xorshift PRNG algorithm, see: https://en.wikipedia.org/wiki/Xorshift#Example_implementation */

/* The state word must be initialized to non-zero */
unsigned int xorshift(unsigned int *state)
{
    /* Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs" */
    unsigned int x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}


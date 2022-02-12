
/* This is a simple pseudorandom number generator.
 * See https://en.wikipedia.org/wiki/Xorshift#Example_implementation.
 *
 * How to use it:
 *
 * Initialize your state variable to a random seed... should not be zero, should contain
 * a fair number of 1 bits and a fair number of 0 bits.
 *
 * unsigned int my_state = 0xa5a5a5a5; // For example.
 *
 * Then you can do something like this:
 *
 * int random_number_between_0_and_1000(void)
 * {
 *    return (xorshift(&my_state) % 1000);
 * }
 *
 */
unsigned int xorshift(unsigned int *state);


#include <stdint.h>
#include <accelerometer.h>

void accelerometer_init_gpio()
{
    return;
}

void accelerometer_init()
{
    return;
}

uint8_t accelerometer_whoami()
{
    return 0x33;
}

union acceleration accelerometer_last_sample()
{
    union acceleration a = {0};
    return a;
}

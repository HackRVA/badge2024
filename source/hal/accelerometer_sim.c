#include <stdint.h>
#include <accelerometer.h>

static union acceleration a = {0};

static int convert_to_accel_value(float gees)
{
	if (gees > 2.0)
		gees = 2.0;
	if (gees < -2.0)
		gees = -2.0;

	return 1000.0f * gees;
}

#if TARGET_SIMULATOR
void set_simulated_accelerometer_values(float x, float y, float z)
{
	a.x = convert_to_accel_value(x);
	a.y = convert_to_accel_value(y);
	a.z = convert_to_accel_value(z);
}
#endif

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
    return a;
}

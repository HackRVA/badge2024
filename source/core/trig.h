#ifndef TRIG_H__
#define TRIG_H__

/* Sine, cosine and arctan2, adapted for non-floating point badge use.
 *
 * The angle should be in the range 0-127 (mapped to 0 to 360 degrees).
 * The returned value is the sin or cosine * 256, as a signed short. 
 * 
 * So if you have for example a bullet moving at speed v, angle a,
 * and you want to compute vx, and vy, on a real computer you'd do:
 *
 * vx = v * cos(a);
 * vy = v * -sin(a);
 *
 * On the badge, you do:
 *
 * vx = (v * cosine(a)) / 256;  // or >> 8. Compiler is probably smart enough to do that anyway.
 * vy = (v * -sine(a)) / 256; 
 *
 * The angles returned by arctan2 are in the range -64 to +64, corresponding to -180 and +180 degrees.
 */

short cosine(int angle);
short sine(int angle);
short arctan2(short y, short x);

#endif

/* 3d vector... only used by simulator for accellerometer emulation */

#include <math.h>

#include "vec3.h"

/* init vector v */
void vec3_init(union vec3 *v, float x, float y, float z)
{
	v->v.x = x;
	v->v.y = y;
	v->v.z = z;
}

/* normalize vector v */
void vec3_normalize_self(union vec3 *v)
{
	float len = sqrtf(v->v.x * v->v.x + v->v.y * v->v.y + v->v.z * v->v.z);
	v->v.x /= len;
	v->v.y /= len;
	v->v.z /= len;
}

/* returns square of the length of a vector */
float vec3_len2(const union vec3 *v)
{
	return v->v.x * v->v.x + v->v.y * v->v.y + v->v.z * v->v.z;
}

float vec3_dot(const union vec3 *v1, const union vec3 *v2)
{
	return v1->vec[0] * v2->vec[0] + v1->vec[1] * v2->vec[1] + v1->vec[2] * v2->vec[2];
}

union vec3* vec3_cross(union vec3 *vo, const union vec3 *v1, const union vec3 *v2)
{
	vo->vec[0] = v1->vec[1]*v2->vec[2] - v1->vec[2]*v2->vec[1];
	vo->vec[1] = v1->vec[2]*v2->vec[0] - v1->vec[0]*v2->vec[2];
	vo->vec[2] = v1->vec[0]*v2->vec[1] - v1->vec[1]*v2->vec[0];
	return vo;
}


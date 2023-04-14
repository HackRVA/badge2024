#ifndef VEC3_H__
#define VEC3_H__
/* 3d vector... only used by simulator for accellerometer emulation */

union vec3 {
	struct {
		float x, y, z;
	} v;
	float vec[3];
};

/* init vector v */
void vec3_init(union vec3 *v, float x, float y, float z);

/* normalize vector v */
void vec3_normalize_self(union vec3 *v);

/* returns square of the length of a vector */
float vec3_len2(const union vec3 *v);


float vec3_dot(const union vec3 *v1, const union vec3 *v2);

union vec3* vec3_cross(union vec3 *vo, const union vec3 *v1, const union vec3 *v2);

#endif

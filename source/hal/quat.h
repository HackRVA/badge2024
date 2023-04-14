#ifndef QUAT_H__
#define QUAT_H__

/* quaternion functions, only used by simulator, for the orientation of the badge,
 * for the accellerometer emulation.
 */

#include "vec3.h"

union quat {
	struct {
		float q0, q1, q2, q3;
	} q;
	struct {
		float w, x, y, z;
	} v;
	float vec[4];
};

#define IDENTITY_QUAT_INITIALIZER { { 1.0, 0.0, 0.0, 0.0 } }

/* initialize a quaterion with axis (x, y, z) and angle */
void quat_init_axis(union quat *q, float x, float y, float z, float angle);

/* rotate vector vin by quaternion q, storing result in vout */
void quat_rot_vec(union vec3 *vout, union vec3 *vin, const union quat * const q);

/* rotate vector v by quaternion q, storing result in v */
void quat_rot_vec_self(union vec3 *v, const union quat * const q);

/* o = q1 * q2 */
void quat_mul(union quat *o, const union quat *q1, const union quat *q2);

/* q = q * qi */
void quat_mul_self(union quat *q, union quat *qi);

/* q_out = q_in^-1 */
void quat_inverse(union quat *q_out, const union quat *q_in);

/* Change a quaternion's coordinate system.
 * The rotation is converted to new_coordinate_system, and returned in qo
 */
union quat *quat_conjugate(union quat *qo, union quat *rotation, union quat *new_coordinate_system);

/* o = normalized quaternion q */
void quat_normalize(union quat *o, const union quat *q);

/* normalize quaternion q */
void quat_normalize_self(union quat *q);

/* scale quaternion q by f, result returned in o */
void quat_scale(union quat *o, const union quat *q, float f);

/* Return length of quaternion q */
float quat_len(const union quat *q);

/* Calculate the quaternion q which would rotate vector u to parallel vector v */
void quat_from_u2v(union quat *q, const union vec3 *u, const union vec3 *v);

#endif

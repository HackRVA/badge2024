/* quaternion functions, only used by simulator, for the orientation of the badge,
 * for the accellerometer emulation.
 */
#include <math.h>

#include "quat.h"

static const union quat identity_quat = IDENTITY_QUAT_INITIALIZER;

/* initialize a quaterion with axis (x, y, z) and angle */
void quat_init_axis(union quat *q, float x, float y, float z, float angle)
{
	/* see: http://www.euclideanspace.com/maths/geometry/rotations
		/conversions/angleToQuaternion/index.htm */
	float a2 = angle * 0.5f;
	float s = sin(a2);
	q->v.x = x * s;
	q->v.y = y * s;
	q->v.z = z * s;
	q->v.w = cos(a2);
}

/* rotate vector vin by quaternion q, storing result in vout */
void quat_rot_vec(union vec3 *vout, union vec3 *vin, const union quat * const q)
{
	/* see: https://github.com/qsnake/ase/blob/master/ase/quaternions.py */
	const float vx = vin->v.x, vy = vin->v.y, vz = vin->v.z;
	const float qw = q->v.w, qx = q->v.x, qy = q->v.y, qz = q->v.z;
	const float qww = qw * qw, qxx = qx * qx, qyy = qy * qy, qzz = qz * qz;
	const float qwx = qw * qx, qwy = qw * qy, qwz = qw * qz, qxy = qx * qy;
	const float qxz = qx * qz, qyz = qy * qz;
	vout->v.x = (qww + qxx - qyy - qzz) * vx + 2 * ((qxy - qwz) * vy + (qxz + qwy) * vz);
	vout->v.y = (qww - qxx + qyy - qzz) * vy + 2 * ((qxy + qwz) * vx + (qyz - qwx) * vz);
	vout->v.z = (qww - qxx - qyy + qzz) * vz + 2 * ((qxz - qwy) * vx + (qyz + qwx) * vy);
}

/* rotate vector v by quaternion q, storing result in v */
void quat_rot_vec_self(union vec3 *v, const union quat * const q)
{
	union vec3 vo;
	quat_rot_vec(&vo, v, q);
	*v = vo;
}

/* o = q1 * q2 */
void quat_mul(union quat *o, const union quat *q1, const union quat *q2)
{
	/* see: http://www.euclideanspace.com/maths/algebra/realNormedAlgebra/quaternions/code/index.htm#mul */
	o->v.x =  q1->v.x * q2->v.w + q1->v.y * q2->v.z - q1->v.z * q2->v.y + q1->v.w * q2->v.x;
	o->v.y = -q1->v.x * q2->v.z + q1->v.y * q2->v.w + q1->v.z * q2->v.x + q1->v.w * q2->v.y;
	o->v.z =  q1->v.x * q2->v.y - q1->v.y * q2->v.x + q1->v.z * q2->v.w + q1->v.w * q2->v.z;
	o->v.w = -q1->v.x * q2->v.x - q1->v.y * q2->v.y - q1->v.z * q2->v.z + q1->v.w * q2->v.w;
}

/* q = q * qi */
void quat_mul_self(union quat *q, union quat *qi)
{
	union quat qo;
	quat_mul(&qo, q, qi);
	*q = qo;
}

/* q_out = q_in^-1 */
void quat_inverse(union quat *q_out, const union quat *q_in)
{
	q_out->v.x = -q_in->v.x;
	q_out->v.y = -q_in->v.y;
	q_out->v.z = -q_in->v.z;
	q_out->v.w = q_in->v.w;
}

/* Change a quaternion's coordinate system.
 * The rotation is converted to new_coordinate_system, and returned in qo
 */
union quat *quat_conjugate(union quat *qo, union quat *rotation, union quat *new_coordinate_system)
{
	union quat temp, inverse;

	/* Convert rotation to new coordinate system */
	quat_mul(&temp, new_coordinate_system, rotation);
	quat_inverse(&inverse, new_coordinate_system);
	quat_mul(qo, &temp, &inverse);
	return qo;
}

void quat_scale(union quat *o, const union quat *q, float f)
{
	/* see: http://www.euclideanspace.com/maths/algebra/
		realNormedAlgebra/quaternions/code/index.htm#scale*/
	for (int i = 0; i < 4; ++i)
		o->vec[i] = q->vec[i] * f;
}

float quat_len(const union quat *q)
{
	float s = 0.0f;

	for (int i = 0; i < 4; ++i)
		s += q->vec[i] * q->vec[i];
	return sqrtf(s);
}

void quat_normalize(union quat *o, const union quat *q)
{
	/* see: http://www.euclideanspace.com/maths/algebra/
		realNormedAlgebra/quaternions/code/index.htm#normalise */
	quat_scale(o, q, 1.0f / quat_len(q));
}

/* normalize quaternion q */
void quat_normalize_self(union quat *q)
{
	quat_normalize(q, q);
}

/* Calculate the quaternion q which would rotate vector u to parallel vector v */
void quat_from_u2v(union quat *q, const union vec3 *u, const union vec3 *v)
{
	/* See: http://lolengine.net/blog/2013/09/18/beautiful-maths-quaternion-from-vectors */
	union vec3 w;

	if (vec3_len2(u) < 0.001 || vec3_len2(v) < 0.001) {
		*q = identity_quat;
		return;
	}
	vec3_cross(&w, u, v);
	q->v.w = vec3_dot(u, v);
	q->v.x = w.v.x;
	q->v.y = w.v.y;
	q->v.z = w.v.z;
	q->v.w += quat_len(q);
	if (quat_len(q) < 0.00001) {
		*q = identity_quat;
		return;
	}
	quat_normalize_self(q);
}


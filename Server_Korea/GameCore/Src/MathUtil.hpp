#ifndef _MATH_UTIL_HPP_
#define _MATH_UTIL_HPP_

#include "Math.hpp"
#include <sys/types.h>
#include <string>

void Vector3f_Normalize(Vector3f *v);

float Vector3f_PowerDistance(const Vector3f *v1, const Vector3f *v2);
float Vector3f_Distance(const Vector3f *v1, const Vector3f *v2);

void Vector3f_Print(const Vector3f *v);
void Vector2i_Print(const Vector2i *v);

static inline int64_t TwoInt32ToInt64(int32_t h, int32_t l) {
	return ((int64_t)(u_int32_t)l) | ((int64_t)h << 32);
}

static inline void Int64ToTwoInt32(int64_t src, int32_t *h, int32_t *l) {
	*h = (int32_t)((src & 0xffffffff00000000) >> 32);
	*l = (int32_t)(src & 0xffffffff);
}

const char * base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len);

#endif

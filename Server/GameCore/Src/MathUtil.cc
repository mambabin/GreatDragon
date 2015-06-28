#include "MathUtil.hpp"
#include "Math.hpp"
#include "Config.hpp"
#include <cmath>
#include <cstdio>
#include <string>

using namespace std;

void Vector3f_Normalize(Vector3f *v) {
	if (v->x() == 0.0f && v->y() == 0.0f && v->z() == 0.0f)
		return;

	float len = sqrt(v->x() * v->x() + v->y() * v->y() + v->z() * v->z());
	v->set_x(v->x() / len);
	v->set_y(v->y() / len);
	v->set_z(v->z() / len);
}

float Vector3f_PowerDistance(const Vector3f *v1, const Vector3f *v2) {
	if (v1 == NULL || v2 == NULL)
		return 0.0f;

	return (v1->x() - v2->x()) * (v1->x() - v2->x()) + (v1->z() - v2->z()) * (v1->z() - v2->z());
}

float Vector3f_Distance(const Vector3f *v1, const Vector3f *v2) {
	if (v1 == NULL || v2 == NULL)
		return 0.0f;

	return sqrt((v1->x() - v2->x()) * (v1->x() - v2->x()) + (v1->z() - v2->z()) * (v1->z() - v2->z()));
}

void Vector3f_Print(const Vector3f *v) {
	printf("(%f, %f, %f)\n", v->x(), v->y(), v->z());
}

void Vector2i_Print(const Vector2i *v) {
	printf("(%d, %d)\n", v->x(), v->y());
}

static const char *base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static inline bool is_base64(unsigned char c) {
	    return (isalnum(c) || (c == '+') || (c == '/'));
}

const char * base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len) {
	static char buf[CONFIG_FIXEDARRAY * 10];
	int ret = 0;
	int i = 0;
	int j = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];

	while (in_len--) {
		char_array_3[i++] = *(bytes_to_encode++);
		if (i == 3) {
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;

			for (i = 0; (i <4) ; i++)
				buf[ret++] = base64_chars[char_array_4[i]];
			i = 0;
		}
	}

	if (i)
	{
		for (j = i; j < 3; j++)
			char_array_3[j] = '\0';

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;

		for (j = 0; (j < i + 1); j++)
			buf[ret++] = base64_chars[char_array_4[j]];

		while ((i++ < 3))
			buf[ret++] = '=';

	}

	buf[ret++] = '\0';
	return buf;
}

#pragma once

class quaternion_t {
public:
	float x, y, z, w;
};

class vec3_t {
public:
	// data member variables
	float x, y, z;

public:
	// ctors.
	__forceinline vec3_t() : x{}, y{}, z{} {}
	__forceinline vec3_t(float x, float y, float z) : x{ x }, y{ y }, z{ z } {}

	// at-accesors.
	__forceinline vec3_t CrossProduct(const vec3_t& a, const vec3_t& b)
	{
		return vec3_t(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
	}
	__forceinline float& at(const size_t index) {
		return ((float*)this)[index];
	}
	__forceinline float& at(const size_t index) const {
		return ((float*)this)[index];
	}

	// index operators.
	__forceinline float& operator( )(const size_t index) {
		return at(index);
	}
	__forceinline const float& operator( )(const size_t index) const {
		return at(index);
	}
	__forceinline float& operator[ ](const size_t index) {
		return at(index);
	}
	__forceinline const float& operator[ ](const size_t index) const {
		return at(index);
	}

	// equality operators.
	__forceinline bool operator==(const vec3_t& v) const {
		return v.x == x && v.y == y && v.z == z;
	}
	__forceinline bool operator!=(const vec3_t& v) const {
		return v.x != x || v.y != y || v.z != z;
	}

	// copy assignment.
	__forceinline vec3_t& operator=(const vec3_t& v) {
		x = v.x;
		y = v.y;
		z = v.z;
		return *this;
	}

	__forceinline void Init(float ix = 0.0f, float iy = 0.0f, float iz = 0.0f) {
		x = ix; y = iy; z = iz;
	}

	__forceinline void validate_vec()
	{
		if (std::isnan(this->x)
			|| std::isnan(this->y))
			this->clear();

		if (std::isinf(this->x)
			|| std::isinf(this->y))
			this->clear();
	}


	__forceinline bool is_zero(float tolerance = 0.01f) const {
		return (this->x > -tolerance && this->x < tolerance&&
			this->y > -tolerance && this->y < tolerance&&
			this->z > -tolerance && this->z < tolerance);
	}


	// negation-operator.
	__forceinline vec3_t operator-() const {
		return vec3_t{ -x, -y, -z };
	}

	// arithmetic operators.
	__forceinline vec3_t operator+(const vec3_t& v) const {
		return {
			x + v.x,
			y + v.y,
			z + v.z
		};
	}

	__forceinline vec3_t operator-(const vec3_t& v) const {
		return {
			x - v.x,
			y - v.y,
			z - v.z
		};
	}

	__forceinline vec3_t operator*(const vec3_t& v) const {
		return {
			x * v.x,
			y * v.y,
			z * v.z
		};
	}

	__forceinline vec3_t operator/(const vec3_t& v) const {
		return {
			x / v.x,
			y / v.y,
			z / v.z
		};
	}

	// compound assignment operators.
	__forceinline vec3_t& operator+=(const vec3_t& v) {
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}

	__forceinline vec3_t& operator-=(const vec3_t& v) {
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}

	__forceinline vec3_t& operator*=(const vec3_t& v) {
		x *= v.x;
		y *= v.y;
		z *= v.z;
		return *this;
	}

	__forceinline vec3_t& operator/=(const vec3_t& v) {
		x /= v.x;
		y /= v.y;
		z /= v.z;
		return *this;
	}

	// arithmetic operators w/ float.
	__forceinline vec3_t operator+(float f) const {
		return {
			x + f,
			y + f,
			z + f
		};
	}

	__forceinline vec3_t operator-(float f) const {
		return {
			x - f,
			y - f,
			z - f
		};
	}

	__forceinline vec3_t operator*(float f) const {
		return {
			x * f,
			y * f,
			z * f
		};
	}

	__forceinline vec3_t operator/(float f) const {
		return {
			x / f,
			y / f,
			z / f
		};
	}

	// compound assignment operators w/ float.
	__forceinline vec3_t& operator+=(float f) {
		x += f;
		y += f;
		z += f;
		return *this;
	}

	__forceinline vec3_t& operator-=(float f) {
		x -= f;
		y -= f;
		z -= f;
		return *this;
	}

	__forceinline vec3_t& operator*=(float f) {
		x *= f;
		y *= f;
		z *= f;
		return *this;
	}

	__forceinline vec3_t& operator/=(float f) {
		x /= f;
		y /= f;
		z /= f;
		return *this;
	}

	// methods.
	__forceinline void clear() {
		x = y = z = 0.f;
	}

	__forceinline float length_sqr() const {
		return ((x * x) + (y * y) + (z * z));
	}

	__forceinline float length_2d_sqr() const {
		return ((x * x) + (y * y));
	}

	__forceinline float length() const {
		return std::sqrt(length_sqr());
	}

	__forceinline float length_2d() const {
		return std::sqrt(length_2d_sqr());
	}

	__forceinline float dot(const vec3_t& v) const {
		return (x * v.x + y * v.y + z * v.z);
	}

	__forceinline float dot(float* v) const {
		return (x * v[0] + y * v[1] + z * v[2]);
	}

	__forceinline float vec3_t::Dot(const float* fOther) const
	{
		const vec3_t& a = *this;

		return(a.x * fOther[0] + a.y * fOther[1] + a.z * fOther[2]);
	}

	__forceinline vec3_t cross(const vec3_t &v) const {
		return {
			(y * v.z) - (z * v.y),
			(z * v.x) - (x * v.z),
			(x * v.y) - (y * v.x)
		};
	}

	__forceinline float dist_to(const vec3_t& v) const {
		vec3_t delta;

		delta.x = x - v.x;
		delta.y = y - v.y;
		delta.z = z - v.z;

		return delta.length_2d();
	}

	__forceinline float dist(const vec3_t& v) const {
		vec3_t delta;

		delta.x = x - v.x;
		delta.y = y - v.y;
		delta.z = z - v.z;

		return delta.length();
	}

	__forceinline float normalize() {
		float len = length();

		(*this) /= (length() + std::numeric_limits< float >::epsilon());

		return len;
	}

	__forceinline bool valid() const {
		return x != 0 && y != 0 && z != 0;
	}

	__forceinline vec3_t normalized() const {
		auto vec = *this;

		vec.normalize();

		return vec;
	}

	// shit pasted from other projects.
	__forceinline void to_vectors(vec3_t& right, vec3_t& up) {
		vec3_t tmp;
		if (x == 0.f && y == 0.f)
		{
			// pitch 90 degrees up/down from identity.
			right[0] = 0.f;
			right[1] = -1.f;
			right[2] = 0.f;
			up[0] = -z;
			up[1] = 0.f;
			up[2] = 0.f;
		}
		else
		{
			tmp[0] = 0; tmp[1] = 0; tmp[2] = 1;

			// get directions vector using cross product.
			right = cross(tmp).normalized();
			up = right.cross(*this).normalized();
		}
	}
};

__forceinline vec3_t operator*(float f, const vec3_t& v) {
	return v * f;
}

class __declspec(align(16)) vec_aligned_t : public vec3_t {
public:
	__forceinline vec_aligned_t() {}

	__forceinline vec_aligned_t(const vec3_t& vec) {
		x = vec.x;
		y = vec.y;
		z = vec.z;
		w = 0.f;
	}

	float w;
};
#pragma once

class ang_t {
public:
	// data member variables.
	float x, y, z;

public:
	// constructors.
	__forceinline ang_t() : x{}, y{}, z{} {}
	__forceinline ang_t(float x, float y, float z) : x{ x }, y{ y }, z{ z } {}

	// at-accesors.
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
	__forceinline bool operator==(const ang_t& v) const {
		return v.x == x && v.y == y && v.z == z;
	}

	__forceinline bool is_zero(float tolerance = 0.01f) const {
		return (this->x > -tolerance && this->x < tolerance&&
			this->y > -tolerance && this->y < tolerance&&
			this->z > -tolerance && this->z < tolerance);
	}

	__forceinline bool operator!=(const ang_t& v) const {
		return v.x != x || v.y != y || v.z != z;
	}

	__forceinline bool operator !() const {
		return !x && !y && !z;
	}

	// copy assignment.
	__forceinline ang_t& operator=(const ang_t& v) {
		x = v.x;
		y = v.y;
		z = v.z;

		return *this;
	}

	// negation-operator.
	__forceinline ang_t operator-() const {
		return ang_t(-x, -y, -z);
	}

	// arithmetic operators.
	__forceinline ang_t operator+(const ang_t& v) const {
		return {
			x + v.x,
			y + v.y,
			z + v.z
		};
	}

	__forceinline ang_t operator-(const ang_t& v) const {
		return {
			x - v.x,
			y - v.y,
			z - v.z
		};
	}

	__forceinline ang_t operator*(const ang_t& v) const {
		return {
			x * v.x,
			y * v.y,
			z * v.z
		};
	}

	__forceinline ang_t operator/(const ang_t& v) const {
		return {
			x / v.x,
			y / v.y,
			z / v.z
		};
	}

	// compound assignment operators.
	__forceinline ang_t& operator+=(const ang_t& v) {
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}

	__forceinline ang_t& operator-=(const ang_t& v) {
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}

	__forceinline ang_t& operator*=(const ang_t& v) {
		x *= v.x;
		y *= v.y;
		z *= v.z;
		return *this;
	}

	__forceinline ang_t& operator/=(const ang_t& v) {
		x /= v.x;
		y /= v.y;
		z /= v.z;
		return *this;
	}

	// arithmetic operators w/ float.
	__forceinline ang_t operator+(float f) const {
		return {
			x + f,
			y + f,
			z + f
		};
	}

	__forceinline ang_t operator-(float f) const {
		return {
			x - f,
			y - f,
			z - f
		};
	}

	__forceinline ang_t operator*(float f) const {
		return {
			x * f,
			y * f,
			z * f
		};
	}

	__forceinline ang_t operator/(float f) const {
		return {
			x / f,
			y / f,
			z / f
		};
	}

	// compound assignment operators w/ float.
	__forceinline ang_t& operator+=(float f) {
		x += f;
		y += f;
		z += f;
		return *this;
	}

	__forceinline ang_t& operator-=(float f) {
		x -= f;
		y -= f;
		z -= f;
		return *this;
	}

	__forceinline ang_t& operator*=(float f) {
		x *= f;
		y *= f;
		z *= f;
		return *this;
	}

	__forceinline ang_t& operator/=(float f) {
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

	__forceinline void normalize() {
		math::NormalizeAngle(x);
		math::NormalizeAngle(y);
		math::NormalizeAngle(z);
	}

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
			up = right.cross(vec3_t(x, y, z)).normalized();
		}
	}

	__forceinline ang_t normalized() const {
		auto vec = *this;
		vec.normalize();
		return vec;
	}

	__forceinline void clamp() {
		math::clamp(x, -89.f, 89.f);
		math::clamp(y, -180.f, 180.f);
		math::clamp(z, -90.f, 90.f);
	}

	__forceinline void SanitizeAngle() {
		math::NormalizeAngle(y);
		clamp();
	}
};
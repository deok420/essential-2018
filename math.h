#pragma once

#define M_PIRAD     0.01745329251f
#define M_RADPI     57.295779513082f
#define M_PI		(float)3.14159265358979323846f

constexpr auto RadPi = 3.14159265358979323846;
constexpr auto DegPi = 180.0;

namespace math {
    // pi constants.
	constexpr float pi = 3.1415926535897932384f; // pi
	constexpr float pi_2 = pi * 2.f;               // pi * 2
	constexpr float invtwopi = 0.1591549f;
	constexpr float halfpi = 1.570796f;
	constexpr float threehalfpi = 4.7123889f;
	static const __m128 signmask = _mm_castsi128_ps(_mm_set1_epi32(0x80000000));

    // degrees to radians.
    __forceinline constexpr float deg_to_rad(float val) {
        return val * (pi / 180.f);
    }

    // radians to degrees.
    __forceinline constexpr float rad_to_deg(float val) {
        return val * (180.f / pi);
    }

    // angle mod ( shitty normalize ).
    __forceinline float AngleMod(float angle) {
        return (360.f / 65536) * ((int)(angle * (65536.f / 360.f)) & 65535);
    }

    typedef __declspec(align(16)) union {
        float f[4];
        __m128 v;
    } m128;

    inline __m128 sqrt_ps(const __m128 squared) {
        return _mm_sqrt_ps(squared);
    }

    template<typename T>
    T ToDegrees(T radians) {
        return (radians * (static_cast<T>(DegPi) / static_cast<T>(RadPi)));
    }

	template < typename T >
	T Clamp(const T& val, const T& minVal, const T& maxVal) {
		if ((T)val < minVal)
			return minVal;
		else if ((T)val > maxVal)
			return maxVal;
		else
			return val;
	}


	void VectorVectors(const vec3_t& forward, vec3_t& right, vec3_t& up);

	__forceinline __m128 cos_52s_ps(const __m128 x)
	{
		const auto c1 = _mm_set1_ps(0.9999932946f);
		const auto c2 = _mm_set1_ps(-0.4999124376f);
		const auto c3 = _mm_set1_ps(0.0414877472f);
		const auto c4 = _mm_set1_ps(-0.0012712095f);
		const auto x2 = _mm_mul_ps(x, x);
		return _mm_add_ps(c1, _mm_mul_ps(x2, _mm_add_ps(c2, _mm_mul_ps(x2, _mm_add_ps(c3, _mm_mul_ps(c4, x2))))));
	}


	bool  IntersectLineWithBB(vec3_t& vStart, vec3_t& vEndDelta, vec3_t& vMin, vec3_t& vMax);
	vec3_t vector_rotate(const vec3_t& in1, const matrix3x4_t& in2);
	vec3_t vector_rotate(const vec3_t& in1, const ang_t& in2);
	void vector_i_rotate(const vec3_t& in1, const matrix3x4_t& in2, vec3_t& out);
	void angle_matrix(const ang_t& angles, matrix3x4_t& matrix);


	__forceinline void sincos_ps(__m128 angle, __m128 *sin, __m128 *cos) {
		const auto anglesign = _mm_or_ps(_mm_set1_ps(1.f), _mm_and_ps(signmask, angle));
		angle = _mm_andnot_ps(signmask, angle);
		angle = _mm_sub_ps(angle, _mm_mul_ps(_mm_cvtepi32_ps(_mm_cvttps_epi32(_mm_mul_ps(angle, _mm_set1_ps(invtwopi)))), _mm_set1_ps(pi_2)));

		auto cosangle = angle;
		cosangle = _mm_xor_ps(cosangle, _mm_and_ps(_mm_cmpge_ps(angle, _mm_set1_ps(halfpi)), _mm_xor_ps(cosangle, _mm_sub_ps(_mm_set1_ps(pi), angle))));
		cosangle = _mm_xor_ps(cosangle, _mm_and_ps(_mm_cmpge_ps(angle, _mm_set1_ps(pi)), signmask));
		cosangle = _mm_xor_ps(cosangle, _mm_and_ps(_mm_cmpge_ps(angle, _mm_set1_ps(threehalfpi)), _mm_xor_ps(cosangle, _mm_sub_ps(_mm_set1_ps(pi_2), angle))));

		auto result = cos_52s_ps(cosangle);
		result = _mm_xor_ps(result, _mm_and_ps(_mm_and_ps(_mm_cmpge_ps(angle, _mm_set1_ps(halfpi)), _mm_cmplt_ps(angle, _mm_set1_ps(threehalfpi))), signmask));
		*cos = result;

		const auto sinmultiplier = _mm_mul_ps(anglesign, _mm_or_ps(_mm_set1_ps(1.f), _mm_and_ps(_mm_cmpgt_ps(angle, _mm_set1_ps(pi)), signmask)));
		*sin = _mm_mul_ps(sinmultiplier, sqrt_ps(_mm_sub_ps(_mm_set1_ps(1.f), _mm_mul_ps(result, result))));
	}


    float NormalizeYaw(float angle);

    void AngleMatrix(const ang_t& ang, const vec3_t& pos, matrix3x4_t& out);

    // normalizes an angle.
    void NormalizeAngle(float& angle);

	template<class T>
	void Normalize(T& vec)
	{
		for (auto i = 0; i < 3; i++) {
			while (vec[i] < -180.0f) vec[i] += 360.0f;
			while (vec[i] > 180.0f) vec[i] -= 360.0f;
		}
		vec[2] = 0.f;
	}

	void AngleVectors69(const ang_t& angles, vec3_t& forward);
	void VectorAngles3(const vec3_t& forward, vec3_t& up, ang_t& angles);
	float random_float(float min, float max);
	int random_int(int min, int max);
	void random_seed(int seed);

    vec3_t vector_angles(const vec3_t& v);
    vec3_t angle_vectors(const vec3_t& angles);
    template<class T, class U>
    static T dont_break(const T& in, const U& low, const U& high)
    {

        if (in <= low)
            return low;

        if (in >= high)
            return high;

        return in;
    }

    __forceinline float NormalizedAngle(float angle) {
        NormalizeAngle(angle);
        return angle;
    }

    __forceinline float angle_diff(float f, float w) {
        float delta;

        delta = fmodf(f - w, 360.0f);
        if (f > w) {
            if (delta >= 180)
                delta -= 360;
        }
        else {
            if (delta <= -180)
                delta += 360;
        }
        return delta;
    }


    bool IntersectSegmentToSegment(vec3_t s1, vec3_t s2, vec3_t k1, vec3_t k2, float radius);
    bool IntersectSegmentSphere(const vec3_t& vecRayOrigin, const vec3_t& vecRayDelta, const vec3_t& vecSphereCenter, float radius);
    bool IntersectSegmentCapsule(const vec3_t& start, const vec3_t& end, const vec3_t& min, const vec3_t& max, float radius);
    bool IntersectionBoundingBox(const vec3_t& start, const vec3_t& dir, const vec3_t& min, const vec3_t& max, vec3_t* hit_point = nullptr);
    static float normalize_float(float angle)
    {
        auto revolutions = angle / 360.f;
        if (angle > 180.f || angle < -180.f)
        {
            revolutions = round(abs(revolutions));
            if (angle < 0.f)
                angle = (angle + 360.f * revolutions);
            else
                angle = (angle - 360.f * revolutions);
            return angle;
        }
        return angle;
    }

    __forceinline float SimpleSpline(float value) {
        float valueSquared = value * value;

        // Nice little ease-in, ease-out spline-like curve
        return (3 * valueSquared - 2 * valueSquared * value);
    }

    __forceinline float SimpleSplineRemapValClamped(float val, float A, float B, float C, float D) {
        if (A == B)
            return val >= B ? D : C;
        float cVal = (val - A) / (B - A);
        cVal = std::clamp(cVal, 0.0f, 1.0f);
        return C + (D - C) * SimpleSpline(cVal);
    }

    vec3_t CalcAngle(const vec3_t& vecSource, const vec3_t& vecDestination);
    void CalcAngle3(const vec3_t src, const vec3_t dst, ang_t& angles);
    float ApproachAngle(float target, float value, float speed);
    void VectorAngles1(const vec3_t& vecForward, vec3_t& vecAngles);
    void AngleVectors1(const vec3_t& angles, vec3_t* forward);
    void  VectorAngles(const vec3_t& forward, ang_t& angles, vec3_t* up = nullptr);
    void NormalizeVector(vec3_t& vec);
    void AngleVectors(const ang_t& angles, vec3_t& forward);
    void  AngleVectors(const ang_t& angles, vec3_t* forward, vec3_t* right = nullptr, vec3_t* up = nullptr);
    inline void SinCos(float radians, float* sine, float* cosine);
    void AngleVectorKidua(ang_t& vAngle, vec3_t& vForward);
    float GetFOV(const ang_t& view_angles, const vec3_t& start, const vec3_t& end);
    void  VectorTransform(const vec3_t& in, const matrix3x4_t& matrix, vec3_t& out);
    float SegmentToSegment(const vec3_t s1, const vec3_t s2, const vec3_t k1, const vec3_t k2);
    void  VectorITransform(const vec3_t& in, const matrix3x4_t& matrix, vec3_t& out);
    void  MatrixAngles(const matrix3x4_t& matrix, ang_t& angles);
    void  MatrixCopy(const matrix3x4_t& in, matrix3x4_t& out);
    void  ConcatTransforms(const matrix3x4_t& in1, const matrix3x4_t& in2, matrix3x4_t& out);

    // computes the intersection of a ray with a box ( AABB ).
    bool IntersectRayWithBox(const vec3_t& start, const vec3_t& delta, const vec3_t& mins, const vec3_t& maxs, float tolerance, BoxTraceInfo_t* out_info);
    bool IntersectRayWithBox(const vec3_t& start, const vec3_t& delta, const vec3_t& mins, const vec3_t& maxs, float tolerance, CBaseTrace* out_tr, float* fraction_left_solid = nullptr);

    // computes the intersection of a ray with a oriented box ( OBB ).
    bool IntersectRayWithOBB(const vec3_t& start, const vec3_t& delta, const matrix3x4_t& obb_to_world, const vec3_t& mins, const vec3_t& maxs, float tolerance, CBaseTrace* out_tr);
    bool IntersectRayWithOBB(const vec3_t& start, const vec3_t& delta, const vec3_t& box_origin, const ang_t& box_rotation, const vec3_t& mins, const vec3_t& maxs, float tolerance, CBaseTrace* out_tr);

    // returns whether or not there was an intersection of a sphere against an infinitely extending ray. 
    // returns the two intersection points.
    bool IntersectInfiniteRayWithSphere(const vec3_t& start, const vec3_t& delta, const vec3_t& sphere_center, float radius, float* out_t1, float* out_t2);

    // Returns A + (B-A)*flPercent.
  // float Lerp( float flPercent, float A, float B );
    template <class T>
    __forceinline T Lerp(float flPercent, T const& A, T const& B)
    {
        return A + (B - A) * flPercent;
    }

	template <typename t>
	static t lerp2(float progress, const t& t1, const t& t2) {
		return t1 + (t2 - t1) * progress;
	}

    // returns whether or not there was an intersection, also returns the two intersection points ( clamped 0.f to 1.f. ).
    // note: the point of closest approach can be found at the average t value.
    bool IntersectRayWithSphere(const vec3_t& start, const vec3_t& delta, const vec3_t& sphere_center, float radius, float* out_t1, float* out_t2);

    vec3_t Interpolate(const vec3_t from, const vec3_t to, const float percent);

	ang_t Interpolate(const ang_t from, const ang_t to, const float percent);

	float Interpolate(const float from, const float to, const float percent);

	float AngleDiff(float destAngle, float srcAngle);

	void MatrixSetOrigin(vec3_t pos, matrix3x4_t& matrix);

	vec3_t MatrixGetOrigin(const matrix3x4_t& src);

	template < typename t >
	__forceinline void clamp(t& n, const t& lower, const t& upper) {
		n = std::max(lower, std::min(n, upper));
	}

	template < class T >
	__forceinline T interpolate(const T& flCurrent, const T& flTarget, const int iProgress, const int iMaximum)
	{
		return flCurrent + ((flTarget - flCurrent) / iMaximum) * iProgress;
	}
}
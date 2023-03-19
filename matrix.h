#pragma once
#include <DirectXMath.h>

class matrix3x4_t {
public:
	__forceinline matrix3x4_t() {}

	__forceinline matrix3x4_t(float m00, float m01, float m02, float m03, float m10, float m11, float m12, float m13, float m20, float m21, float m22, float m23) {
		m_flMatVal[0][0] = m00;
		m_flMatVal[0][1] = m01;
		m_flMatVal[0][2] = m02;
		m_flMatVal[0][3] = m03;
		m_flMatVal[1][0] = m10;
		m_flMatVal[1][1] = m11;
		m_flMatVal[1][2] = m12;
		m_flMatVal[1][3] = m13;
		m_flMatVal[2][0] = m20;
		m_flMatVal[2][1] = m21;
		m_flMatVal[2][2] = m22;
		m_flMatVal[2][3] = m23;
	}

	__forceinline void Init(const vec3_t& x, const vec3_t& y, const vec3_t& z, const vec3_t& origin) {
		m_flMatVal[0][0] = x.x;
		m_flMatVal[0][1] = y.x;
		m_flMatVal[0][2] = z.x;
		m_flMatVal[0][3] = origin.x;
		m_flMatVal[1][0] = x.y;
		m_flMatVal[1][1] = y.y;
		m_flMatVal[1][2] = z.y;
		m_flMatVal[1][3] = origin.y;
		m_flMatVal[2][0] = x.z;
		m_flMatVal[2][1] = y.z;
		m_flMatVal[2][2] = z.z;
		m_flMatVal[2][3] = origin.z;
	}

	__forceinline matrix3x4_t(const vec3_t& x, const vec3_t& y, const vec3_t& z, const vec3_t& origin) {
		m_flMatVal[0][0] = x.x;
		m_flMatVal[0][1] = y.x;
		m_flMatVal[0][2] = z.x;
		m_flMatVal[0][3] = origin.x;
		m_flMatVal[1][0] = x.y;
		m_flMatVal[1][1] = y.y;
		m_flMatVal[1][2] = z.y;
		m_flMatVal[1][3] = origin.y;
		m_flMatVal[2][0] = x.z;
		m_flMatVal[2][1] = y.z;
		m_flMatVal[2][2] = z.z;
		m_flMatVal[2][3] = origin.z;
	}

	__forceinline matrix3x4_t ConcatTransforms(matrix3x4_t in) const {
		matrix3x4_t out;
		out[0][0] = m_flMatVal[0][0] * in[0][0] + m_flMatVal[0][1] * in[1][0] + m_flMatVal[0][2] * in[2][0];
		out[0][1] = m_flMatVal[0][0] * in[0][1] + m_flMatVal[0][1] * in[1][1] + m_flMatVal[0][2] * in[2][1];
		out[0][2] = m_flMatVal[0][0] * in[0][2] + m_flMatVal[0][1] * in[1][2] + m_flMatVal[0][2] * in[2][2];
		out[0][3] = m_flMatVal[0][0] * in[0][3] + m_flMatVal[0][1] * in[1][3] + m_flMatVal[0][2] * in[2][3] + m_flMatVal[0][3];
		out[1][0] = m_flMatVal[1][0] * in[0][0] + m_flMatVal[1][1] * in[1][0] + m_flMatVal[1][2] * in[2][0];
		out[1][1] = m_flMatVal[1][0] * in[0][1] + m_flMatVal[1][1] * in[1][1] + m_flMatVal[1][2] * in[2][1];
		out[1][2] = m_flMatVal[1][0] * in[0][2] + m_flMatVal[1][1] * in[1][2] + m_flMatVal[1][2] * in[2][2];
		out[1][3] = m_flMatVal[1][0] * in[0][3] + m_flMatVal[1][1] * in[1][3] + m_flMatVal[1][2] * in[2][3] + m_flMatVal[1][3];
		out[2][0] = m_flMatVal[2][0] * in[0][0] + m_flMatVal[2][1] * in[1][0] + m_flMatVal[2][2] * in[2][0];
		out[2][1] = m_flMatVal[2][0] * in[0][1] + m_flMatVal[2][1] * in[1][1] + m_flMatVal[2][2] * in[2][1];
		out[2][2] = m_flMatVal[2][0] * in[0][2] + m_flMatVal[2][1] * in[1][2] + m_flMatVal[2][2] * in[2][2];
		out[2][3] = m_flMatVal[2][0] * in[0][3] + m_flMatVal[2][1] * in[1][3] + m_flMatVal[2][2] * in[2][3] + m_flMatVal[2][3];
		return out;
	}

	__forceinline void SetOrigin(const vec3_t& p) {
		m_flMatVal[0][3] = p.x;
		m_flMatVal[1][3] = p.y;
		m_flMatVal[2][3] = p.z;
	}

	__forceinline vec3_t GetOrigin() {
		return { m_flMatVal[0][3], m_flMatVal[1][3], m_flMatVal[2][3] };
	}

	__forceinline float* operator[](int i) {
		return m_flMatVal[i];
	}

	__forceinline const float* operator[](int i) const {
		return m_flMatVal[i];
	}

	__forceinline float* Base() {
		return &m_flMatVal[0][0];
	}

	__forceinline const float* Base() const {
		return &m_flMatVal[0][0];
	}

	void MatrixSetColumn(const vec3_t& in, int column) {
		m_flMatVal[0][column] = in.x;
		m_flMatVal[1][column] = in.y;
		m_flMatVal[2][column] = in.z;
	}

	void AngleMatrix(const ang_t& angles) {
		float sr, sp, sy, cr, cp, cy;
		DirectX::XMScalarSinCos(&sy, &cy, math::deg_to_rad(angles[1]));
		DirectX::XMScalarSinCos(&sp, &cp, math::deg_to_rad(angles[0]));
		DirectX::XMScalarSinCos(&sr, &cr, math::deg_to_rad(angles[2]));

		// matrix = (YAW * PITCH) * ROLL
		m_flMatVal[0][0] = cp * cy;
		m_flMatVal[1][0] = cp * sy;
		m_flMatVal[2][0] = -sp;

		float crcy = cr * cy;
		float crsy = cr * sy;
		float srcy = sr * cy;
		float srsy = sr * sy;
		m_flMatVal[0][1] = sp * srcy - crsy;
		m_flMatVal[1][1] = sp * srsy + crcy;
		m_flMatVal[2][1] = sr * cp;

		m_flMatVal[0][2] = (sp * crcy + srsy);
		m_flMatVal[1][2] = (sp * crsy - srcy);
		m_flMatVal[2][2] = cr * cp;

		m_flMatVal[0][3] = 0.0f;
		m_flMatVal[1][3] = 0.0f;
		m_flMatVal[2][3] = 0.0f;
	}

	void AngleMatrix(const ang_t& angles, const vec3_t& position) {
		AngleMatrix(angles);
		MatrixSetColumn(position, 3);
	}

public:
	float m_flMatVal[3][4];
};

class BoneArray : public matrix3x4_t {
public:
	bool get_bone(vec3_t& out, int bone = 0) {
		if (bone < 0 || bone >= 128)
			return false;

		matrix3x4_t* bone_matrix = &this[bone];

		if (!bone_matrix)
			return false;

		out = { bone_matrix->m_flMatVal[0][3], bone_matrix->m_flMatVal[1][3], bone_matrix->m_flMatVal[2][3] };

		return true;
	}
};

class __declspec(align(16)) matrix3x4a_t : public matrix3x4_t {
public:
	__forceinline matrix3x4a_t& operator=(const matrix3x4_t& src) {
		std::memcpy(Base(), src.Base(), sizeof(float) * 3 * 4);
		return *this;
	};
};

class VMatrix {
public:
	__forceinline float* operator[](int i) {
		return m[i];
	}
	__forceinline const float* operator[](int i) const {
		return m[i];
	}

	__forceinline float* Base() {
		return &m[0][0];
	}
	__forceinline const float* Base() const {
		return &m[0][0];
	}

public:
	float m[4][4];
};
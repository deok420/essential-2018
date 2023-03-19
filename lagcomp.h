#pragma once

class AimPlayer;

class LagCompensation {
public:
	enum LagType : size_t {
		INVALID = 0,
		CONSTANT,
		ADAPTIVE,
		RANDOM,
	};

public:
	bool StartPrediction( AimPlayer* player );
	void PlayerMove( LagRecord* record );
	void AirAccelerate( LagRecord* record, ang_t angle, float fmove, float smove );
	void PredictAnimations( CCSGOPlayerAnimState* state, LagRecord* record );
	void BackupOperation(bool restore);
	void update_lerp();


};

class C_LagRecord {
public:

	Player* player = nullptr;
	vec3_t m_vecMins, m_vecMaxs;
	vec3_t m_vecOrigin;
	ang_t m_absAngles;

	alignas(16) matrix3x4_t m_pMatrix[128];

	void Setup(Player* player, bool isBackup = false);
	void Apply(Player* player);
};

extern LagCompensation g_lagcomp;
#pragma once

class ShotRecord {
public:
	__forceinline ShotRecord() : m_target{}, m_hitbox{}, m_record_valid{}, m_occlusion_miss{}, m_aim_point {}, m_hitgroup{}, m_record{}, m_shoot_pos{}, m_time{}, m_lat{}, m_damage{}, m_range{}, m_pos{}, m_impact_pos{}, m_confirmed{}, m_hurt{}, m_impacted{} {}

public:
	Player*    m_target;
	LagRecord* m_record;
	float      m_time, m_lat, m_damage, m_range;
	vec3_t     m_pos, m_impact_pos, m_shoot_pos;
	vec3_t      m_aim_point;
	bool       m_confirmed, m_hurt, m_impacted, m_record_valid, m_occlusion_miss;
	int        m_hitbox, m_hitgroup;
	int chance;
};

class VisualImpactData_t {
public:
	vec3_t m_impact_pos, m_shoot_pos;
	int    m_tickbase;
	bool   m_ignore, m_hit_player;

public:
	__forceinline VisualImpactData_t(const vec3_t& impact_pos, const vec3_t& shoot_pos, int tickbase) :
		m_impact_pos{ impact_pos }, m_shoot_pos{ shoot_pos }, m_tickbase{ tickbase }, m_ignore{ false }, m_hit_player{ false } {}
};

class Shots {
public:
	void OnShotFire(Player* target, float damage, int bullets, LagRecord* record, int hitbox, int hitgroup, vec3_t aim_point);
	void OnImpact(IGameEvent* evt);
	void OnHurt(IGameEvent* evt);
	void OnWeaponFire(IGameEvent* evt);
	void OnShotMiss(ShotRecord& shot);
	void Think();

public:
	std::array< std::string, 11 > m_groups = {
	 XOR("generic"),
	 XOR("head"),
	 XOR("chest"),
	 XOR("stomach"),
	 XOR("left arm"),
	 XOR("right arm"),
	 XOR("left leg"),
	 XOR("right leg"),
	 XOR("neck"),
	 XOR("unknown"),
	 XOR("gear")
	};

	int headshots;
	std::deque< ShotRecord >          m_shots;
	std::vector< VisualImpactData_t > m_vis_impacts;
};

extern Shots g_shots;
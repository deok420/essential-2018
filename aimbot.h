#pragma once

enum HitscanMode : int {
	NORMAL  = 0,
	LETHAL  = 1,
	PREFER  = 2
};


enum target_select : int {
	DEFAULT = 0,
	LETHAL_ALL = 1,
	LETHAL_BAIM = 2,
	CYCLE = 3
};






struct AnimationBackup_t {
	vec3_t           m_origin, m_abs_origin;
	vec3_t           m_velocity, m_abs_velocity;
	int              m_flags, m_eflags;
	float            m_duck, m_body;
	C_AnimationLayer m_layers[ 13 ];
};

struct HitscanData_t {
	float  m_damage;
	int    m_hitbox;
	int    m_hitgroup;
	int    m_awall_index;
	vec3_t m_pos;

	__forceinline HitscanData_t() : m_damage{ 0.f }, m_pos{}, m_hitbox{}, m_hitgroup{}, m_awall_index{} {}
};

struct HitscanBox_t {
	int         m_index;
	HitscanMode m_mode;

	__forceinline bool operator==( const HitscanBox_t& c ) const {
		return m_index == c.m_index && m_mode == c.m_mode;
	}
};


struct HitscanPoint_t {
	int         m_index;
	vec3_t      m_point;
	bool        m_center;
	bool        m_not_capsule;
	int       m_damage;


	__forceinline bool operator==(HitscanPoint_t& c) {
		return m_index == c.m_index && m_point == c.m_point && m_center == c.m_center && m_not_capsule == c.m_not_capsule;
	}

};


struct SimulationRestore {

	int m_fFlags;
	int m_nStrafeSequence;
	float m_flDuckAmount;
	float m_flFeetCycle;
	float m_flFeetYawRate;
	float m_flStrafeChangeWeight;
	float m_flStrafeChangeCycle;
	float m_flAccelerationWeight;
	ang_t m_angEyeAngles;
	vec3_t m_vecOrigin;
	vec3_t m_vecAbsOrigin;

	void Setup(Player* player) {
		m_fFlags = player->m_fFlags();
		m_flDuckAmount = player->m_flDuckAmount();
		m_vecOrigin = player->m_vecOrigin();
		m_angEyeAngles = player->m_angEyeAngles();
		m_vecAbsOrigin = player->GetAbsOrigin();

		auto animState = player->m_PlayerAnimState();
		m_flFeetCycle = animState->m_primary_cycle;
		m_flFeetYawRate = animState->m_move_weight;
		m_nStrafeSequence = animState->m_strafe_sequence;
		m_flStrafeChangeWeight = animState->m_strafe_change_weight;
		m_flStrafeChangeCycle = animState->m_strafe_change_cycle;
		m_flAccelerationWeight = animState->m_acceleration_weight;
	}

	void Apply(Player* player) const {
		player->m_fFlags() = m_fFlags;
		player->m_flDuckAmount() = m_flDuckAmount;
		player->m_vecOrigin() = m_vecOrigin;
		player->m_angEyeAngles() = m_angEyeAngles;
		player->SetAbsOrigin(m_vecAbsOrigin);

		auto animState = player->m_PlayerAnimState();
		animState->m_primary_cycle = m_flFeetCycle;
		animState->m_move_weight = m_flFeetYawRate;
		animState->m_strafe_sequence = m_nStrafeSequence;
		animState->m_strafe_change_weight = m_flStrafeChangeWeight;
		animState->m_strafe_change_cycle = m_flStrafeChangeCycle;
		animState->m_acceleration_weight = m_flAccelerationWeight;
	}
};


class AimPlayer {
public:
	using records_t   = std::deque< std::shared_ptr< LagRecord > >;
	using hitboxcan_t = stdpp::unique_vector< HitscanBox_t >;

public:
	// essential data.
	Player*   m_player;
	float	  m_spawn;
	float     m_last_reliable_pitch;
	int m_ticks_since_dormant;
	records_t m_records;

	float m_old_sim;
	float m_cur_sim;
	float m_sim_rate;
	float m_sim_cycle;
	
	// aimbot data.
	hitboxcan_t m_hitboxes;
	bool        m_prefer_body;
	bool        m_prefer_head;
	vec3_t      m_pos{};
	
	// resolve data.
	int       m_shots;
	int       m_missed_shots;
	int       m_test_index;
	// LagRecord m_walk_record;

	float     m_body_update;
	bool      m_moved;

	float m_flCycle;
	float m_flPlaybackRate;

	int m_edge_index;
	int m_logic_index;
	int m_reversefs_index;
	int m_fam_reverse_index;
	bool m_should_correct;
	int m_testfs_index;
	int m_spin_index;
	float m_last_angle;
	int m_reversefs_at_index;
	int m_stand3_reversefs;
	int m_stand3_back;
	int m_airlast_index;
	int m_air_index;
	int m_airlby_index;
	int m_airback_index;
	int m_back_index;
	int m_back_at_index;
	int m_lastmove_index;
	int m_sidelast_index;
	int m_lby_index;
	float update_body;
	int m_lowlby_index;
	bool valid_flick;
	int m_lbyticks;
	bool m_broke_lby;
	bool m_has_body_updated;
	bool ever_flicked;
	int m_stand_index1;
	int m_stand_index2;
	float m_fakeflick_body;
	int m_stand_index3;
	int m_stand_index4;
	int m_body_index;
	int m_fakewalk_index;
	int m_moving_index;

	// data about the LBY proxy.
	float m_body;
	float m_old_body;
	float m_anti_fs_angle;
	bool freestand_data;
	bool is_last_moving_lby_valid;
	bool is_air_previous_valid;
	float m_valid_pitch;

	float m_stored_body;
	float m_stored_body_old;

	float m_body_proxy;
	float m_body_proxy_old;
	bool  m_body_proxy_updated;

	float m_moving_time;
	float m_moving_body;
	vec3_t m_moving_origin;

	C_LagRecord backup_record = {};
public:
	void CorrectAnimlayers(LagRecord* record);
	void CorrectVelocity(Player* m_player, LagRecord* m_record, LagRecord* m_previous, LagRecord* m_pre_previous, float max_speed);
	void UpdateAnimations( LagRecord* record );
	void OnNetUpdate( Player* player );
	void OnRoundStart( Player* player );
	void SetupHitboxes( LagRecord* record, bool history );
	bool SetupHitboxPoints(LagRecord* record, int index, std::vector<vec3_t>& points);
	bool GetBestAimPosition( vec3_t& aim, float& damage, int& hitbox, int& hitgroup, int& awall_hitbox, LagRecord* record );

public:
	void reset( ) {
		m_player       = nullptr;
		m_spawn        = 0.f;
		this->m_ticks_since_dormant = 0;
		this->m_moved = false;
		this->m_body_update = FLT_MAX;
		this->m_moving_time = -1;
		this->m_has_body_updated = false;
		this->m_fam_reverse_index = 0;
		this->is_last_moving_lby_valid = false;
		this->m_airback_index = 0;
		this->m_testfs_index = 0;
		this->m_lowlby_index = 0;
		this->m_logic_index = 0;
		this->m_stand_index4 = 0;
		this->m_air_index = 0;
		this->m_airlby_index = 0;
		this->m_broke_lby = false;
		this->m_spin_index = 0;
		this->m_stand_index1 = 0;
		this->m_stand_index2 = 0;
		this->m_stand_index3 = 0;
		this->m_edge_index = 0;
		this->m_back_index = 0;
		this->m_reversefs_index = 0;
		this->m_back_at_index = 0;
		this->m_reversefs_at_index = 0;
		this->m_lastmove_index = 0;
		this->m_lby_index = 0;
		this->m_airlast_index = 0;
		this->m_body_index = 0;
		this->m_lbyticks = 0;
		this->m_sidelast_index = 0;
		this->m_body_proxy_updated = false;
		this->m_moving_index = 0;

		m_records.clear( );
		m_hitboxes.clear( );
	}
};

class Aimbot {
private:
	struct target_t {
		Player*    m_player;
		AimPlayer* m_data;
	};

	struct knife_target_t {
		target_t  m_target;
		LagRecord m_record;
	};

	struct table_t {
		uint8_t swing[ 2 ][ 2 ][ 2 ]; // [ first ][ armor ][ back ]
		uint8_t stab[ 2 ][ 2 ];		  // [ armor ][ back ]
	};

	const table_t m_knife_dmg{ { { { 25, 90 }, { 21, 76 } }, { { 40, 90 }, { 34, 76 } } }, { { 65, 180 }, { 55, 153 } } };

	std::array< ang_t, 12 > m_knife_ang{
		ang_t{ 0.f, 0.f, 0.f }, ang_t{ 0.f, -90.f, 0.f }, ang_t{ 0.f, 90.f, 0.f }, ang_t{ 0.f, 180.f, 0.f },
		ang_t{ -80.f, 0.f, 0.f }, ang_t{ -80.f, -90.f, 0.f }, ang_t{ -80.f, 90.f, 0.f }, ang_t{ -80.f, 180.f, 0.f },
		ang_t{ 80.f, 0.f, 0.f }, ang_t{ 80.f, -90.f, 0.f }, ang_t{ 80.f, 90.f, 0.f }, ang_t{ 80.f, 180.f, 0.f }
	};

public:
	std::array< AimPlayer, 64 > m_players;
	std::vector< AimPlayer* >   m_targets;

	// target selection stuff.
	int target_selection;
	float m_best_dist;
	float m_best_fov;
	float m_best_damage;
	int   m_best_hp;
	float m_best_lag;
	float m_best_height;
	
	// found target stuff.
	Player*    m_target;
	ang_t      m_angle;
	int        m_hitbox, m_hitgroup, m_awall_hitbox;
	vec3_t     m_aim;
	float      m_hitchance;
	float      m_damage;
	bool       m_damage_toggle;
	LagRecord* m_record;
	bool	   m_force_body;

	// fake latency stuff.
	bool       m_fake_latency;

	bool m_stop;

public:
	__forceinline void reset( ) {
		// reset aimbot data.
		init( );

		// reset all players data.
		for( auto& p : m_players )
			p.reset( );
	}

	__forceinline bool IsValidTarget( Player* player ) {
		if (!player)
			return false;

		if (!player->IsPlayer())
			return false;

		if (!player->alive())
			return false;

		if (player->m_bIsLocalPlayer())
			return false;

		if (!player->enemy(g_cl.m_local))
			return false;

		if (player->dormant())
			return false;

		if (!player->GetClientClass())
			return false;

		if (player->m_fImmuneToGunGameDamageTime())
			return false;

		return true;
	}

public:
	// aimbot.
	void init( );
	void StripAttack( );
	void think( );
	bool SelectTarget(LagRecord* record, const vec3_t& aim, float damage);
	bool CanHit(const vec3_t start, const vec3_t end, LagRecord* animation, int box, bool in_shot, BoneArray* bones);
	bool CanHitPlayer(LagRecord* pRecord, const vec3_t& vecEyePos, const vec3_t& vecEnd, int iHitboxIndex);
	void find( );
	float GetIdealAccuracyBoost();
	bool CheckHitchance( Player* player, const ang_t& angle, int aimed_hitgroup );
	void apply( );
	void start_target_selection();
	void finish_target_selection();
	void NoSpread( );

	void update_shoot_pos();

	// knifebot.
	void knife( );
	bool CanKnife( LagRecord* record, ang_t angle, bool& stab );
	bool KnifeTrace( vec3_t dir, bool stab, CGameTrace* trace );
	bool KnifeIsBehind( LagRecord* record );


	int to_hitgroup(int hitbox) {

		switch (hitbox) {
		case HITBOX_HEAD:
		case HITBOX_NECK:
		case HITBOX_LOWER_NECK:
			return HITGROUP_HEAD;
		case HITBOX_UPPER_CHEST:
		case HITBOX_CHEST:
		case HITBOX_THORAX:
			return HITGROUP_CHEST;
		case HITBOX_PELVIS:
		case HITBOX_BODY:
			return HITGROUP_STOMACH;
		case HITBOX_L_THIGH:
		case HITBOX_L_CALF:
		case HITBOX_L_FOOT:
			return HITGROUP_LEFTLEG;
		case HITBOX_R_CALF:
		case HITBOX_R_FOOT:
		case HITBOX_R_THIGH:
			return HITGROUP_RIGHTLEG;
		case HITBOX_L_FOREARM:
		case HITBOX_L_HAND:
		case HITBOX_L_UPPER_ARM:
		case HITBOX_R_UPPER_ARM:
		case HITBOX_R_FOREARM:
		case HITBOX_R_HAND:
			return HITGROUP_LEFTARM; // lol ghetto
		default:
			return HITGROUP_GENERIC;
		}
	}

};



extern Aimbot g_aimbot;
#pragma once

#include "CPlayerResource.h"

class Sequence {
public:
	float m_time;
	int   m_state;
	int   m_seq;

public:
	__forceinline Sequence( ) : m_time{}, m_state{}, m_seq{} {};
	__forceinline Sequence( float time, int state, int seq ) : m_time{ time }, m_state{ state }, m_seq{ seq } {};
};

class NetPos {
public:
	float  m_time;
	vec3_t m_pos;

public:
	__forceinline NetPos( ) : m_time{}, m_pos{} {};
	__forceinline NetPos( float time, vec3_t pos ) : m_time{ time }, m_pos{ pos } {};
};

class C_AnimationLayer {
public:
	float   m_anim_time;			// 0x0000
	float   m_fade_out_time;		// 0x0004
	int     m_flags;				// 0x0008
	int     m_activty;				// 0x000C
	int     m_priority;				// 0x0010
	int     m_order;				// 0x0014
	int     m_sequence;				// 0x0018
	float   m_prev_cycle;			// 0x001C
	float   m_weight;				// 0x0020
	float   m_weight_delta_rate;	// 0x0024
	float   m_playback_rate;		// 0x0028
	float   m_cycle;				// 0x002C
	Entity *m_owner;				// 0x0030
	int     m_bits;					// 0x0034
}; // size: 0x0038

class Client {
public:
	// hack thread.
	static ulong_t __stdcall init( void* arg );

	void StartMove( CUserCmd* cmd );
	void EndMove( CUserCmd* cmd );
	void DoMove( );
	void DrawHUD( );
	void animate();
	void UpdateInformation( );
	void SetAngles( );
	void UpdateAnimations( );
	void KillFeed( );

	void OnPaint( );
	void OnMapload( );
	void OnTick( CUserCmd* cmd );

	// debugprint function.
	void print( const std::string text, ... );

	// check if we are able to fire this tick.
	bool CanFireWeapon(float curtime);
	void MouseFix(CUserCmd* cmd);
	void UpdateRevolverCock( );
	void UpdateIncomingSequences( );

public:
	// local player variables.
	Player*          m_local;
	bool	         m_processing;
	int	             m_flags;
	int	             m_old_flags;
	vec3_t	         m_shoot_pos;
	bool	         m_player_fire;
	float m_last_simulation;
	float m_last_in_air_time;
	bool	         m_shot;
	bool	         m_old_shot;
	float            m_abs_yaw;
	float            m_poses[24];
	C_AnimationLayer m_layers[13];
	bool			 m_updating_bones;

	bool m_should_animate;
	vec3_t m_real_bone_pos[256];
	quaternion_t m_real_bone_rot[256];
	bool m_pressing_move;
	bool csgobuild_invalid = false;
	// active weapon variables.
	Weapon*     m_weapon;
	int         m_weapon_id;
	WeaponInfo* m_weapon_info;
	int         m_weapon_type;
	bool        m_weapon_fire, m_update_anims, m_update_enemy;

	// revolver variables.
	int	 m_revolver_cock;
	int	 m_revolver_query;
	bool m_revolver_fire;

	float m_flPreviousDuckAmount = 0.0f;

	// general game varaibles.
	bool     m_round_end;
	Stage_t	 m_stage;
	int	     m_max_lag;
	int      m_lag;
	int	     m_old_lag;
	bool*    m_packet;
	bool*    m_final_packet;
	bool	 m_old_packet;
	float	 m_lerp;
	float    m_latency;
	int      m_latency_ticks;
	int      m_server_tick;
	int      m_arrival_tick;
	int      m_width, m_height;

	// usercommand variables.
	CUserCmd* m_cmd;
	int	      m_tick;
	int	      m_buttons;
	int       m_old_buttons;
	ang_t     m_view_angles;
	ang_t	  m_strafe_angles;
	ang_t	  m_original_angles;
	vec3_t	  m_forward_dir;

    penetration::PenetrationOutput_t m_pen_data;

	std::deque< Sequence > m_sequences;
	std::deque< NetPos >   m_net_pos;

	// animation variables.
	ang_t  m_angle, m_real_angle;
	ang_t  m_rotation;
	ang_t  m_radar;
	float  m_body;
	int    m_flick_since_moving;
	bool   m_should_def;
	float  m_update_time;
	float  m_body_pred;
	float  m_speed;
	float  m_anim_time;
	float  m_anim_frame;
	float  m_spawn;
	bool   m_ground;
	bool   m_lagcomp;
	
	// prediction variables.
	float backup_curtime, backup_frametime;
	int backup_tickbase;
	vec3_t backup_origin;
	vec3_t backup_aim_punch_velocity, backup_view_offset;
	ang_t backup_aim_punch;
	float backup_accuracy_penalty, backup_recoil_index, backup_duck_amount;

	// hack username.
	std::string m_user;
	BoneArray m_shoot_pos_bones[128];
	BoneArray m_real_bones[128];
	CSPlayerResource** m_resource = nullptr;
};

extern Client g_cl;
#pragma once

class LagRecord {
public:

	// data.
	Player* m_player;
	float   m_immune;
	int     m_tick;
	int     m_sim_ticks;
	int     m_anim_ticks;
	float   m_lag_time;
	bool    m_dormant;
	float   m_shift;

	// netvars.
	int    m_anim_flags;
	float  m_sim_time;
	float  m_old_sim_time;
	int    m_flags;
	vec3_t m_origin;
	vec3_t m_old_origin;
	vec3_t m_velocity;
	vec3_t m_mins;
	vec3_t m_maxs;
	ang_t  m_eye_angles;
	ang_t  m_abs_ang;
	Color m_resolver_color;
	bool  m_resolved;
	std::string m_resolver_mode;
	float  m_body;
	float  m_duck;

	// anim stuff.
	C_AnimationLayer m_server_layers[ 13 ];
	float            m_poses[ 24 ];

	// bone stuff.
	bool       m_setup;

	BoneArray* m_bones;
	// BoneArray* m_bones;

	// lagfix stuff.
	bool   m_broke_lc;
	bool   m_ground;
	vec3_t m_pred_origin;
	vec3_t m_pred_velocity;
	float  m_pred_time;
	int    m_pred_flags;

	// resolver stuff.
	size_t m_mode;
	float  m_animation_speed, m_max_speed;
	bool   m_fake_walk, m_fake_flick;
	bool   m_shot;

	// other stuff.
	float  m_interp_time;
	bool   m_sideway;
	float  m_anim_time;
	Weapon* m_weapon;
public:

	// default ctor.
	__forceinline LagRecord( ) : 
		m_setup{ false }, 
		m_broke_lc{ false },
		m_fake_walk{ false },
		m_fake_flick{ false },
		m_shot{ false }, 
		m_sim_ticks{}, 
		m_bones{} {}

	// ctor.
	__forceinline LagRecord( Player* player ) : 
		m_setup{ false }, 
		m_broke_lc{ false },
		m_fake_walk{ false },
		m_fake_flick{ false },
		m_shot{ false }, 
		m_sim_ticks{}, 
		m_bones{} {

		store( player );
	}

	// dtor.
	__forceinline ~LagRecord( ){
		// free heap allocated game mem.
		g_csgo.m_mem_alloc->Free( m_bones );
	}

	__forceinline void invalidate( ) {
		// free heap allocated game mem.
		g_csgo.m_mem_alloc->Free( m_bones );

		// mark as not setup.
		m_setup = false;

		// allocate new memory.
		m_bones = ( BoneArray* )g_csgo.m_mem_alloc->Alloc( sizeof( BoneArray ) * 128 );
	}

	// function: allocates memory for SetupBones and stores relevant data.
	void store( Player* player, bool backup = false ) {
		// allocate game heap.
		// m_bones = ( BoneArray* )g_csgo.m_mem_alloc->Alloc( sizeof( BoneArray ) * 128 );

		if (backup)
			memcpy(m_bones, m_player->m_CachedBoneData().m_mem.m_ptr, sizeof(matrix3x4_t) * m_player->m_CachedBoneData().m_size);
		else
			m_bones = (BoneArray*)g_csgo.m_mem_alloc->Alloc(sizeof(BoneArray) * 128);

		// player data.
		m_player    = player;
		m_immune    = player->m_fImmuneToGunGameDamageTime( );
		m_tick      = g_csgo.m_cl->m_server_tick;
	
		// netvars.
		m_pred_time     = m_sim_time = player->m_flSimulationTime( );
		m_old_sim_time  = player->m_flOldSimulationTime( );
		m_pred_flags    = m_flags  = player->m_fFlags( );
		m_pred_origin   = m_origin = player->m_vecOrigin( );
		m_old_origin    = player->m_vecOldOrigin( );
		m_eye_angles    = player->m_angEyeAngles( );
		m_abs_ang       = player->GetAbsAngles( );
		m_body          = player->m_flLowerBodyYawTarget( );
		m_mins          = player->m_vecMins( );
		m_maxs          = player->m_vecMaxs( );
		m_duck          = player->m_flDuckAmount( );
		m_pred_velocity = m_velocity = player->m_vecVelocity( );

		// save networked animlayers.
		player->GetAnimLayers( m_server_layers );

		// normalize eye angles.
		m_eye_angles.normalize( );
		math::clamp( m_eye_angles.x, -90.f, 90.f );

		// get lag.
		m_lag_time = m_sim_time - m_old_sim_time;
		m_sim_ticks = game::TIME_TO_TICKS(m_lag_time);
		m_anim_ticks = game::TIME_TO_TICKS(m_lag_time);

		// compute animtime.
		m_anim_time = m_old_sim_time + g_csgo.m_globals->m_interval;
	}

	// function: restores 'predicted' variables to their original.
	__forceinline void predict( ) {
		// @evitable: we already check for lc in animation fix.
		m_broke_lc      = broke_lc();
		// ???
		m_ground        = m_flags & FL_ONGROUND;
		m_pred_origin   = m_origin;
		m_pred_velocity = m_velocity;
		m_pred_time     = m_sim_time;
		m_pred_flags    = m_flags;
	}

	__forceinline bool dormant( ) {
		return m_dormant;
	}

	__forceinline bool immune( ) {
		return m_immune > 0.f;
	}

	__forceinline bool broke_lc() {
		return ( this->m_origin - this->m_old_origin ).length_sqr( ) > 4096.f;
	}

	// function: checks if LagRecord obj is hittable if we were to fire at it now.
	bool valid( ) {
		if (!this)
			return false;

		INetChannel* info = g_csgo.m_engine->GetNetChannelInfo();
		if (!info || !g_cl.m_local)
			return false;

		if (m_sim_ticks > 20 || m_sim_ticks < 0)
			return false;

		// get correct based on out latency + in latency + lerp time and clamp on sv_maxunlag
		float correct = 0;

		// add out latency
		correct += info->GetLatency(INetChannel::FLOW_OUTGOING);

		// add in latency
		correct += info->GetLatency(INetChannel::FLOW_INCOMING);
		
		// add interpolation amount
		correct += g_cl.m_lerp;

		// clamp this shit
		correct = std::clamp(correct, 0.0f, g_csgo.sv_maxunlag->GetFloat());

		// def cur time
		float curtime = g_cl.m_local->alive() ? game::TICKS_TO_TIME(g_cl.m_local->m_nTickBase()) : g_csgo.m_globals->m_curtime;

		// get delta time
		float delta_time = correct - (curtime - m_sim_time);

		// if out of range
		// https://github.com/perilouswithadollarsign/cstrike15_src/blob/f82112a2388b841d72cb62ca48ab1846dfcc11c8/game/server/player_lagcompensation.cpp#L292
		if (fabs(delta_time) > 0.2f)
			return false;

		// just ignore dead time or not?
		return true;
	}
};
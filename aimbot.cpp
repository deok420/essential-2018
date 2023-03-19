#include "includes.h"
#include <execution>

Aimbot g_aimbot{ };

void AimPlayer::UpdateAnimations( LagRecord* record ) {

	CCSGOPlayerAnimState* state = this->m_player->m_PlayerAnimState( );
	if (!state)
		return;

	// player respawned.
	if ( this->m_player->m_flSpawnTime( ) != this->m_spawn ) {

		// reset animation state.
		game::ResetAnimationState( state );

		// note new spawn time.
		this->m_spawn = m_player->m_flSpawnTime( );
	}

	SimulationRestore SimulationRecordBackup;
	SimulationRecordBackup.Setup(m_player);

	// s/o onetap
	const float m_flRealtime = g_csgo.m_globals->m_realtime;
	const float m_flCurtime = g_csgo.m_globals->m_curtime;
	const float m_flFrametime = g_csgo.m_globals->m_frametime;
	const float m_flAbsFrametime = g_csgo.m_globals->m_abs_frametime;
	const int m_iFramecount = g_csgo.m_globals->m_frame;
	const int m_iTickcount = g_csgo.m_globals->m_tick_count;
	const float interpolation = g_csgo.m_globals->m_interp_amt;

	// @ruka: simtime & player simtime are the same btw
	g_csgo.m_globals->m_realtime = record->m_sim_time;
	g_csgo.m_globals->m_curtime = record->m_sim_time;
	g_csgo.m_globals->m_frametime = g_csgo.m_globals->m_interval;
	g_csgo.m_globals->m_abs_frametime = g_csgo.m_globals->m_interval;
	g_csgo.m_globals->m_frame = game::TIME_TO_TICKS( record->m_sim_time );
	g_csgo.m_globals->m_tick_count = game::TIME_TO_TICKS( record->m_sim_time );
	g_csgo.m_globals->m_interp_amt = 0.f;

	// backup stuff that we do not want to fuck with.
	AnimationBackup_t backup;

	backup.m_origin = this->m_player->m_vecOrigin( );
	backup.m_abs_origin = this->m_player->GetAbsOrigin( );
	backup.m_velocity = this->m_player->m_vecVelocity( );
	backup.m_abs_velocity = this->m_player->m_vecAbsVelocity( );
	backup.m_flags = this->m_player->m_fFlags( );
	backup.m_eflags = this->m_player->m_iEFlags( );
	backup.m_duck = this->m_player->m_flDuckAmount( );
	backup.m_body = this->m_player->m_flLowerBodyYawTarget( );
	this->m_player->GetAnimLayers( backup.m_layers );


	LagRecord* previous = nullptr;
	LagRecord* pre_previous = nullptr;

	// safety check!!
	if ( this->m_ticks_since_dormant > 2 ) {

		// get previous record.
		if ( this->m_records.size() >= 2 )
			previous = this->m_records[1].get( )->dormant( ) ? nullptr : this->m_records[1].get( );

		// get pre-previous record.
		if ( this->m_records.size() >= 3 )
			pre_previous = this->m_records[2].get( )->dormant( ) ? nullptr : this->m_records[2].get( );
	}

	// is player a bot?
	bool bot = game::IsFakePlayer( this->m_player->index( ) );

	// reset fakewalk state.
	record->m_fake_walk = false;
	record->m_fake_flick = false;
	record->m_mode = Resolver::Modes::RESOLVE_NONE;
	record->m_resolver_mode = XOR( "NONE" );
	record->m_resolved = false;
	record->m_max_speed = 260.f;
	record->m_broke_lc = false;
	record->m_weapon = ( Weapon* )( this->m_player->m_hActiveWeapon( ).Get( ) );

	// do we have a valid weapon?
	if ( record->m_weapon ) {

		// get data of that weapon
		WeaponInfo* wpn_data = record->m_weapon->GetWpnData( );

		// get weapon max speed
		if ( wpn_data )
			record->m_max_speed = this->m_player->m_bIsScoped( ) ? wpn_data->m_max_player_speed_alt : wpn_data->m_max_player_speed;
	}


	// @ruka: recalculating anim lag based on animlayers
	// @ruka: credits magma.digital (memesis)
	if ( previous ) {

		// get network lag
		record->m_sim_ticks = game::TIME_TO_TICKS( m_player->m_flSimulationTime( ) - m_player->m_flOldSimulationTime( ) );

		// make sure alive loop is valid and that we should correct
		if ( previous->m_server_layers[11].m_playback_rate == record->m_server_layers[11].m_playback_rate ) {

			if ( record->m_weapon == previous->m_weapon
				&& record->m_server_layers[11].m_playback_rate > 0.f
				&& record->m_server_layers[11].m_cycle > previous->m_server_layers[11].m_cycle ) {

				const int network_lag = record->m_sim_ticks + 1;
				const int anim_lag = game::TIME_TO_TICKS(
					( record->m_server_layers[11].m_cycle - previous->m_server_layers[11].m_cycle )
					/ record->m_server_layers[11].m_playback_rate
				);

				// should we correct it?
				if ( anim_lag > network_lag && anim_lag <= 19 )
					record->m_anim_ticks = anim_lag;
			}
		}
	}

	// @ruka: velocity correction
	// @ruka: credits magma.digital (memesis) & kaaba.su (amnesia)
	if ( record->m_sim_ticks <= 20 )
		CorrectVelocity( this->m_player, record, previous, pre_previous, record->m_max_speed );

	// fix various issues with the game
	// these issues can only occur when a player is choking data.
	if ( record->m_sim_ticks >= 2 && record->m_sim_ticks <= 20 && previous ) {

		// detect fakewalk.
		float speed = record->m_velocity.length_2d( );

		if ( record->m_flags & FL_ONGROUND
			&& record->m_server_layers[6].m_weight == 0.f
			&& record->m_sim_ticks > 2
			&& speed > 0.f
			&& speed <= record->m_max_speed * 0.34f )
			record->m_fake_walk = true;

		// @evitable: better this way =D
		// @ruka: moved this here to make sure
		record->m_broke_lc = record->broke_lc( );

		// @ruka: this is practically useless but owell
		if ( speed < 20.f
			&& speed > 0.f
			&& record->m_server_layers[6].m_weight != 1.0f && record->m_server_layers[6].m_weight != 0.0f
			&& record->m_server_layers[6].m_weight != previous->m_server_layers[6].m_weight
			&& ( record->m_flags & FL_ONGROUND ) )
			record->m_fake_flick = true;
	
		// reset velocity if fakewalking or fakeflicking
		if ( record->m_fake_walk/* || record->m_fake_flick*/ )
			record->m_velocity.Init( );

		// if last or current record is in air that means we should correct his flag
		if ( ( previous->m_flags & FL_ONGROUND ) != ( record->m_flags & FL_ONGROUND ) ) {

			// setup vars for corrections
			bool ground = record->m_flags & FL_ONGROUND;
			bool landed = false;
			float land_time = 0.f;

			// air cycle is lower than 0.5f
			if ( record->m_server_layers[4].m_cycle < 0.5f ) {

				// set potential land time
				land_time = record->m_sim_time - ( record->m_server_layers[4].m_playback_rate / record->m_server_layers[4].m_cycle );
						
				// set land state based on previous time
				landed = land_time >= previous->m_sim_time;
			}

			// if land state is true correct our flags
			if ( landed ) {

				// land time expired, meaning we landed
				if ( land_time <= record->m_sim_time ) 
					ground = true;
				else // else set to previous
					ground = previous->m_flags & FL_ONGROUND;
			}

			// correct our final flags
			if ( ground ) 
				m_player->m_fFlags( ) |= FL_ONGROUND;
			else
				m_player->m_fFlags( ) &= ~FL_ONGROUND;
		}
		
	}

	bool fake = !bot && g_menu.main.aimbot.correct.get( );

	// if using fake angles, correct angles.
	if ( fake ) {

		g_resolver.ResolveAngles( this->m_player, record );

		if ( ( record->m_mode == Resolver::Modes::RESOLVE_BODY || record->m_mode == Resolver::Modes::RESOLVE_BODY_PRED ) && record->m_sim_ticks >= 15 )
			record->m_resolved = false;
		else
			record->m_resolved = record->m_mode == Resolver::Modes::RESOLVE_WALK
			|| record->m_mode == Resolver::Modes::RESOLVE_LBY
			|| record->m_mode == Resolver::Modes::RESOLVE_LOW_LBY
			|| record->m_mode == Resolver::Modes::RESOLVE_BODY
			|| record->m_mode == Resolver::Modes::RESOLVE_BODY_PRED;
	}

	// set sideway state
	record->m_sideway = g_resolver.IsYawSideways( record->m_player, record->m_eye_angles.y );

	// set stuff before animating.
	this->m_player->m_vecOrigin( ) = record->m_origin;
	this->m_player->m_vecVelocity( ) = this->m_player->m_vecAbsVelocity() = record->m_velocity;
	this->m_player->m_flLowerBodyYawTarget( ) = record->m_body;

	// force to use correct abs origin and velocity ( no CalcAbsolutePosition and CalcAbsoluteVelocity calls )
	this->m_player->m_iEFlags() &= ~( EFL_DIRTY_ABSTRANSFORM | EFL_DIRTY_ABSVELOCITY );

	// write potentially resolved angles.
	this->m_player->m_angEyeAngles( ) = record->m_eye_angles;

	// fix animating in same frame.
	if ( state->m_last_update_frame >= g_csgo.m_globals->m_frame )
		state->m_last_update_frame = g_csgo.m_globals->m_frame - 1;

	// 'm_animating' returns true if being called from SetupVelocity, passes raw velocity to animstate.
	this->m_player->m_bClientSideAnimation( ) = true;
	g_bones.m_updating_anims = true;
	this->m_player->UpdateClientSideAnimation( );
	g_bones.m_updating_anims = false;
	this->m_player->m_bClientSideAnimation( ) = false;

	// restore server layers
	this->m_player->SetAnimLayers( record->m_server_layers );

	// store updated/animated poses and rotation in lagrecord.
	this->m_player->GetPoseParameters( record->m_poses );
	record->m_abs_ang = ang_t( 0.f, state->m_abs_yaw, 0.f );

	// anims have updated.
	SimulationRecordBackup.Apply(m_player);
	this->m_player->InvalidatePhysicsRecursive( InvalidatePhysicsBits_t::ANIMATION_CHANGED );

	// setup bones for this record
	// @ruka: DO THIS HERE AND NOT OUTSIDE FOR ACCURATE DATA 
	g_bones.m_running = true;
	record->m_setup = g_bones.BuildBones( this->m_player, BONE_USED_BY_ANYTHING, record->m_bones, record );
	g_bones.m_running = false;
	// record->m_setup = g_bones.BuildBones( this->m_player, record->m_bones, 128, BONE_USED_BY_ANYTHING & ~BONE_USED_BY_BONE_MERGE, record->m_sim_time, BoneSetupFlags::UseCustomOutput );

	// restore backup data.
	this->m_player->m_vecOrigin( ) = backup.m_origin;
	this->m_player->m_vecVelocity( ) = backup.m_velocity;
	this->m_player->m_vecAbsVelocity( ) = backup.m_abs_velocity;
	this->m_player->m_fFlags( ) = backup.m_flags;
	this->m_player->m_iEFlags( ) = backup.m_eflags;
	this->m_player->m_flDuckAmount( ) = backup.m_duck;
	this->m_player->m_flLowerBodyYawTarget( ) = backup.m_body;
	this->m_player->SetAbsOrigin( backup.m_abs_origin );

	// restore globals.
	g_csgo.m_globals->m_realtime = m_flRealtime;
	g_csgo.m_globals->m_curtime = m_flCurtime;
	g_csgo.m_globals->m_frametime = m_flFrametime;
	g_csgo.m_globals->m_abs_frametime = m_flAbsFrametime;
	g_csgo.m_globals->m_frame = m_iFramecount;
	g_csgo.m_globals->m_tick_count = m_iTickcount;
	g_csgo.m_globals->m_interp_amt = interpolation;
}

void AimPlayer::OnNetUpdate(Player* player) {
	bool reset = (!g_menu.main.aimbot.enable.get() || player->m_lifeState() == LIFE_DEAD || !player->enemy(g_cl.m_local));

	// if this happens, delete all the lagrecords.
	if (reset) {
		player->m_bClientSideAnimation() = true;
		this->m_records.clear();
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

		return;
	}

	// update player ptr if required.
	// reset player if changed.
	if (this->m_player != player)
		this->m_records.clear();

	// update player ptr.
	this->m_player = player;

	// indicate that this player has been out of pvs.
	// insert dummy record to separate records
	// to fix stuff like animation and prediction.
	if (player->dormant() || player->m_flSimulationTime() == 0.0f) {
		bool insert = true;

		this->m_moved = false;
		this->m_moving_time = -1;
		this->m_body_update = FLT_MAX;
		this->m_has_body_updated = false;
		this->m_fam_reverse_index = 0;
		this->m_airback_index = 0;
		this->m_testfs_index = 0;
		this->m_lowlby_index = 0;
		this->m_logic_index = 0;
		this->m_stand_index4 = 0;
		this->m_ticks_since_dormant = 0;
		this->is_last_moving_lby_valid = false;
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

		// we have any records already?
		if (this->m_records.empty() || !this->m_records.front()->dormant()) {
			// add new record.
			this->m_records.emplace_front(std::make_shared< LagRecord >(player));

			// get reference to newly added record.
			LagRecord* current = this->m_records.front().get();

			// mark as non dormant.
			current->m_dormant = true;

		}

		// limit to 1 record
		while (this->m_records.size() > 1)
			this->m_records.pop_back();

		return;
	}

	// not needed but i dont rly care, will get skipped by lagcomp if invalid
	bool update = this->m_records.empty()
		|| this->m_records.front()->m_server_layers[11].m_cycle != player->m_AnimOverlay()[11].m_cycle
		|| this->m_records.front()->m_server_layers[11].m_playback_rate != player->m_AnimOverlay()[11].m_playback_rate
		|| this->m_records.front()->m_sim_time != player->m_flSimulationTime()
		|| player->m_flSimulationTime() != player->m_flOldSimulationTime()
		|| player->m_vecOrigin() != this->m_records.front()->m_origin; // holy fuck thats a shit ton of check

	// this is the first data update we are receving
	// OR we received data with a newer simulation context.
	if (update) {
		// add new record.
		this->m_records.emplace_front(std::make_shared< LagRecord >(player));

		// get reference to newly added record.
		LagRecord* current = this->m_records.front().get();

		// mark as non dormant.
		current->m_dormant = false;

		// update animations on current record.
		// call resolver.
		UpdateAnimations(current);

		// set shifting tickbase record.
		current->m_shift = game::TIME_TO_TICKS(current->m_sim_time) - g_csgo.m_globals->m_tick_count;

		this->m_ticks_since_dormant++;
	}

	while (this->m_records.size() > 256)
		this->m_records.pop_back();
}

void AimPlayer::OnRoundStart(Player* player) {

	this->m_player = player;
	this->m_moving_time = -1;
	this->m_shots = 0;
	this->m_missed_shots = 0;
	this->m_moved = false;
	this->m_body_update = FLT_MAX;
	this->m_test_index = 0;

	// reset stand and body index.
	this->m_has_body_updated = false;
	this->m_fam_reverse_index = 0;
	this->m_airback_index = 0;
	this->m_fakewalk_index = 0;
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
	this->m_ticks_since_dormant = 0;
	this->m_moving_index = 0;
	this->m_body_proxy_updated = false;

	this->m_records.clear();
	this->m_hitboxes.clear();

	// IMPORTANT: DO NOT CLEAR LAST HIT SHIT.
}

void AimPlayer::SetupHitboxes(LagRecord* record, bool history) {
	// reset hitboxes & prefer body state.
	m_hitboxes.clear();
	m_prefer_body = false;

	if (g_cl.m_weapon_id == ZEUS) {
		// hitboxes for the zeus.
		m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::NORMAL });
		m_hitboxes.push_back({ HITBOX_CHEST, HitscanMode::NORMAL });
		m_hitboxes.push_back({ HITBOX_THORAX, HitscanMode::NORMAL });
		m_hitboxes.push_back({ HITBOX_PELVIS, HitscanMode::NORMAL });
		return;
	}


	// prefer, always.
	if (g_menu.main.aimbot.baim1.get(0)) {
		m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::PREFER });
		m_prefer_body = true;
	}

	// prefer, fake.
	if (g_menu.main.aimbot.baim1.get(1) && !record->m_resolved) {
		m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::PREFER });
		m_prefer_body = true;
	}

	// prefer, in air.
	if (g_menu.main.aimbot.baim1.get(2) && !(record->m_flags & FL_ONGROUND)) {
		m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::PREFER });
		m_prefer_body = true;
	}

	bool force_body{ false };

	// only, always.
	if (g_menu.main.aimbot.baim2.get(0))
		force_body = true;

	// only, health.
	if (g_menu.main.aimbot.baim2.get(1) && m_player->m_iHealth() <= (int)g_menu.main.aimbot.baim_hp.get())
		force_body = true;

	// only, fake.
	if (g_menu.main.aimbot.baim2.get(2) && !record->m_resolved)
		force_body = true;

	// only, in air.
	if (g_menu.main.aimbot.baim2.get(3) && !(record->m_flags & FL_ONGROUND))
		force_body = true;

	// only, missed x shots to flick.
	if (g_menu.main.aimbot.baim2.get(4) && m_body_index >= g_menu.main.aimbot.body_misses.get())
		force_body = true;

	// only, on key.
	if (g_aimbot.m_force_body)
		force_body = true;

	// NOTE: i will push hitbox in damage % order
	bool ignore_limbs = record->m_velocity.length_2d() > record->m_max_speed * 0.34f && g_menu.main.aimbot.ignor_limbs.get();

	// @xetraprobine / who's the retard who removed HitscanMode::NORMAL ????
	// hang yourself if you're the one who did it
	// look at getbestaimposition func.
	// @ruka: HEY FUCK YOU THE RAGEBOT WASNT EVEN SUPPOSED TO USE THOSE
	// @ruka: if anything blame eviltable for not using my ragebot.

	// head has 400% of weapon damage, lets push it first
	if (g_menu.main.aimbot.hitbox.get(0) && !force_body)
		m_hitboxes.push_back({ HITBOX_HEAD, HitscanMode::NORMAL });

	// stomach and pelvis have 125% of weapon damage, lets push them in second
	if (g_menu.main.aimbot.hitbox.get(2)) {
		m_hitboxes.push_back({ HITBOX_PELVIS, HitscanMode::NORMAL });
		m_hitboxes.push_back({ HITBOX_BODY, HitscanMode::NORMAL });
	}

	// chest hitboxes have 100% of weapon damage, lets push them in third
	if (g_menu.main.aimbot.hitbox.get(1)) {
		m_hitboxes.push_back({ HITBOX_CHEST, HitscanMode::NORMAL });
		m_hitboxes.push_back({ HITBOX_THORAX, HitscanMode::NORMAL });
		m_hitboxes.push_back({ HITBOX_UPPER_CHEST, HitscanMode::NORMAL });
	}

	// legs have 75% of weapon damage, but its better than arms, so lets push them in 4th 
	if (g_menu.main.aimbot.hitbox.get(4)) {
		m_hitboxes.push_back({ HITBOX_L_THIGH, HitscanMode::NORMAL });
		m_hitboxes.push_back({ HITBOX_R_THIGH, HitscanMode::NORMAL });
		m_hitboxes.push_back({ HITBOX_L_CALF, HitscanMode::NORMAL });
		m_hitboxes.push_back({ HITBOX_R_CALF, HitscanMode::NORMAL });
	}

	// foot.
	if (g_menu.main.aimbot.hitbox.get(5) && !ignore_limbs) {
		m_hitboxes.push_back({ HITBOX_L_FOOT, HitscanMode::NORMAL });
		m_hitboxes.push_back({ HITBOX_R_FOOT, HitscanMode::NORMAL });
	}

	// arms are the least prefered hitbox we want, so push them last
	if (g_menu.main.aimbot.hitbox.get(3) && !ignore_limbs) {
		m_hitboxes.push_back({ HITBOX_L_UPPER_ARM, HitscanMode::NORMAL });
		m_hitboxes.push_back({ HITBOX_R_UPPER_ARM, HitscanMode::NORMAL });
	}
}

void Aimbot::init() {
	// clear old targets.
	m_targets.clear();

	m_target = nullptr;
	m_aim = vec3_t{ };
	m_angle = ang_t{ };
	m_damage = 0.f;
	m_record = nullptr;
	m_stop = false;

	m_best_dist = std::numeric_limits< float >::max();
	m_best_fov = 180.f + 1.f;
	m_best_damage = 0.f;
	m_best_hp = 100 + 1;
	m_best_lag = std::numeric_limits< float >::max();
	m_best_height = std::numeric_limits< float >::max();
	m_hitchance = 0;
}

void Aimbot::start_target_selection() {
	LagRecord* front{ };
	AimPlayer* data{ };

	// setup bones for all valid targets.
	for (int i{ 1 }; i <= g_csgo.m_globals->m_max_clients; ++i) {
		Player* player = g_csgo.m_entlist->GetClientEntity< Player* >(i);

		if (!IsValidTarget(player))
			continue;

		data = &m_players[i - 1];

		// we have no data, or the player ptr in data is invalid
		if (!data || !data->m_player || data->m_player->index() != player->index())
			continue;

		// mostly means he just went out of dormancy
		if (data->m_records.empty())
			continue;

		// get our front record
		front = data->m_records.front().get();

		// front record is invalid, skip this player
		if (!front || front->dormant() || front->immune() || !front->m_setup)
			continue;

		if (data->m_ticks_since_dormant <= 2)
			continue;


		// store player as potential target this tick.
		m_targets.emplace_back(data);
	}
}

bool Aimbot::SelectTarget(LagRecord* record, const vec3_t& aim, float damage) {
	return false;
}

void Aimbot::finish_target_selection() {

	// NOTE: what you can do here is make multiples modes
	//       but most of them will be shit when using target limit
	//       ^ pandora did this mistake on their target limit
	auto sort_targets = [&](const AimPlayer* a, const AimPlayer* b) {


		// player b and player a are the same
		// do nothing
		if (a == b)
			return false;

		// player a is invalid, if player b is valid, prioritize him
		// else do nothing
		if (!a)
			return b ? true : false;

		// player b is invalid, if player a is valid, prioritize him
		// else do nothing
		if (!b)
			return a ? true : false;

		// this is the same player
		// in that case, do nothing
		if (a->m_player == b->m_player || a->m_player->index() == b->m_player->index())
			return false;

		// get fov of player a
		float fov_a = math::GetFOV(g_cl.m_view_angles, g_cl.m_shoot_pos, a->m_player->WorldSpaceCenter());

		// get fov of player b
		float fov_b = math::GetFOV(g_cl.m_view_angles, g_cl.m_shoot_pos, b->m_player->WorldSpaceCenter());

		// if player a fov lower than player b fov prioritize him
		return fov_a < fov_b;
	};

	// if we have only 3 targets or less, no need to sort
	if (m_targets.size() == 1)
		return;

	// std::execution::par -> parallel sorting (multithreaded)
	// NOTE: not obligated, std::sort doesnt take alot of cpu power but its still better
	std::sort(std::execution::par, m_targets.begin(), m_targets.end(), sort_targets);

	// target limit based on our prioritized targets
	while (m_targets.size() > g_menu.main.aimbot.target_limit.get())
		m_targets.pop_back();
}

void Aimbot::StripAttack() {
	if (g_cl.m_weapon_id == REVOLVER)
		g_cl.m_cmd->m_buttons &= ~IN_ATTACK2;

	else
		g_cl.m_cmd->m_buttons &= ~IN_ATTACK;
}


void Aimbot::think() {
	// do all startup routines.
	init();

	// sanity.
	if (!g_cl.m_weapon)
		return;

	// no grenades or bomb.
	if (g_cl.m_weapon_type == WEAPONTYPE_GRENADE || g_cl.m_weapon_type == WEAPONTYPE_C4)
		return;

	if (!g_cl.m_weapon_fire)
		StripAttack();

	// we have no aimbot enabled.
	if (!g_menu.main.aimbot.enable.get())
		return;


	// animation silent aim, prevent the ticks with the shot in it to become the tick that gets processed.
	// we can do this by always choking the tick before we are able to shoot.
	bool revolver = g_cl.m_weapon_id == REVOLVER && g_cl.m_revolver_cock != 0;

	// one tick before being able to shoot.
	if (revolver && g_cl.m_revolver_cock > 0 && g_cl.m_revolver_cock == g_cl.m_revolver_query) {
		*g_cl.m_packet = false;
		return;
	}

	// we have a normal weapon or a non cocking revolver
	// choke if its the processing tick.
	if (g_cl.m_weapon_fire && !g_cl.m_lag && !revolver) {
		*g_cl.m_packet = false;
		StripAttack();
		return;
	}

	// no point in aimbotting if we cannot fire this tick.
	if (!g_cl.m_weapon_fire)
		return;

	// add players in our target selection.
	start_target_selection();

	// dont run ragebot if we have no targets.
	if (m_targets.empty() || m_targets.size() <= 0)
		return;

	// sort our potential targets
	finish_target_selection();

	// run knifebot.
	if (g_cl.m_weapon_type == WEAPONTYPE_KNIFE && g_cl.m_weapon_id != ZEUS) {

		if (g_menu.main.aimbot.knifebot.get())
			knife();

		return;
	}

	// scan available targets... if we even have any.
	find();

	// finally set data when shooting.
	apply();
}

void Aimbot::find() {
	struct BestTarget_t { Player* player; vec3_t pos; float damage; int hitbox; int hitgroup; LagRecord* record; int awall_index; };

	vec3_t       tmp_pos;
	float        tmp_damage;
	int          tmp_hitbox, tmp_hitgroup;
	int          hitbox_wall;
	BestTarget_t best;
	best.player = nullptr;
	best.damage = -1.f;
	best.pos = vec3_t{ };
	best.record = nullptr;
	best.awall_index = -1;

	if (m_targets.empty())
		return;

	if (g_cl.m_weapon_id == ZEUS && !g_menu.main.aimbot.zeusbot.get())
		return;


	// 0 (default): never stops scanning 
	// 1 (lethal): stops as soon as a lethal hitbox was found
	// 2 (lethal body): same as above but for body only
	// 3 (cycle): stop as soon as a target is found
	target_selection = g_menu.main.aimbot.target_selection.get();

	// set this to 0
	int iterated_players{ 0 };

	// iterate all targets.
	for (const auto& t : m_targets) {

		if (t->m_records.empty())
			continue;

		// this player broke lagcomp.
		// his bones have been resetup by our lagcomp.
		// therfore now only the front record is valid.
		if (g_menu.main.aimbot.lagfix.get() == 2 && g_lagcomp.StartPrediction(t)) {
			LagRecord* front = t->m_records.front().get();

			t->SetupHitboxes(front, false);
			if (t->m_hitboxes.empty())
				continue;

			// rip something went wrong..
			if (t->GetBestAimPosition(tmp_pos, tmp_damage, tmp_hitbox, tmp_hitgroup, hitbox_wall, front)) {


				// we did more damage or this target is lethal
				if (tmp_damage > best.damage || tmp_damage >= t->m_player->m_iHealth()) {
					// if we made it so far, set shit.
					best.player = t->m_player;
					best.pos = tmp_pos;
					best.damage = tmp_damage;
					best.record = front;
					best.hitbox = tmp_hitbox;
					best.hitgroup = tmp_hitgroup;
					best.awall_index = hitbox_wall;

			
					// if the entity is lethal no need to scan more entities
					if ( target_selection > DEFAULT ) {

						if ( target_selection == CYCLE 
							|| ( best.damage >= best.player->m_iHealth( ) 
								&& ( target_selection == LETHAL || best.hitbox > 2 ) ) )
							break;
					}
				}
			}

			// go to next
			continue;
		}


		if (g_menu.main.aimbot.lagfix.get() == 1 && t->m_records.front().get()->broke_lc())
			continue;

		LagRecord* first = g_resolver.FindIdealRecord(t);
		if (!first)
			continue;

		t->SetupHitboxes(first, false);
		if (t->m_hitboxes.empty())
			continue;

		bool first_is_good = false;
		bool hit_first = t->GetBestAimPosition(tmp_pos, tmp_damage, tmp_hitbox, tmp_hitgroup, hitbox_wall, first);

		// try to select best record as target.
		if (hit_first) {

			// we did more damage or this target is lethal
			if (tmp_damage > best.damage || tmp_damage >= t->m_player->m_iHealth()) {

				// indicate that first was better than previous target
				first_is_good = true;

				// if we made it so far, set shit.
				best.player = t->m_player;
				best.pos = tmp_pos;
				best.damage = tmp_damage;
				best.record = first;
				best.hitbox = tmp_hitbox;
				best.hitgroup = tmp_hitgroup;
				best.awall_index = hitbox_wall;

		
				// if the entity is lethal no need to scan more entities
				if ( target_selection > DEFAULT ) {

					if ( target_selection == CYCLE 
						|| ( best.damage >= best.player->m_iHealth( ) 
							&& ( target_selection == LETHAL || best.hitbox > 2 ) ) )
						break;
				}
			}
		}

		// we scanned half of our prioritized targets
		// ignore last record on all remaining targets
		if (g_menu.main.aimbot.limit_backtrack.get() && iterated_players >= (int)std::round(m_targets.size() / 2.f))
			continue;

		LagRecord* second = g_resolver.FindLastRecord(t);
		if (!second || second == first)
			continue;

		t->SetupHitboxes(second, true);
		if (t->m_hitboxes.empty())
			continue;

		bool hit_second = t->GetBestAimPosition(tmp_pos, tmp_damage, tmp_hitbox, tmp_hitgroup, hitbox_wall, second);

		// we hit last record
		if (hit_second) {

			// we did more damage, did not hit ideal or we did and priority is the same
			if ((tmp_damage > best.damage || tmp_damage >= t->m_player->m_iHealth())
				&& (!first_is_good
					|| second->m_mode == first->m_mode
					|| second->m_resolved == first->m_resolved)) {

				// if we made it so far, set shit.
				best.player = t->m_player;
				best.pos = tmp_pos;
				best.damage = tmp_damage;
				best.record = second;
				best.hitbox = tmp_hitbox;
				best.hitgroup = tmp_hitgroup;
				best.awall_index = hitbox_wall;

				// if the entity is lethal no need to scan more entities
				if ( target_selection > DEFAULT ) {

					if (target_selection == CYCLE 
						|| ( best.damage >= best.player->m_iHealth() 
							&& ( target_selection == LETHAL || best.hitbox > 2 ) ) )
						break;
				}
			}
		}

		++iterated_players;

	}

	// verify our target and set needed data.
	if (best.player && best.record) {

		// calculate aim angle.
		math::VectorAngles(best.pos - g_cl.m_shoot_pos, m_angle);

		// set member vars.
		this->m_target = best.player;
		this->m_aim = best.pos;
		this->m_damage = best.damage;
		this->m_record = best.record;
		this->m_hitbox = best.hitbox;
		this->m_hitgroup = best.hitgroup;
		this->m_awall_hitbox = best.awall_index;

		if (m_target) {
			AimPlayer* data = &g_aimbot.m_players[this->m_target->index() - 1];

			if (data)
				data->m_pos = best.pos;
		}

		// set autostop shit.
		if (g_menu.main.movement.autostop_always_on.get()) {
			if (g_cl.m_weapon_fire || g_menu.main.movement.autostop_between_shots.get() && g_csgo.m_globals->m_curtime >= g_cl.m_local->m_flNextAttack())
				this->m_stop = !(g_cl.m_buttons & IN_JUMP);
		}


		bool on = g_menu.main.aimbot.hitchance.get() && g_menu.main.config.mode.get() == 0;
		bool hit = CheckHitchance(this->m_target, this->m_angle, this->m_hitbox);

		// if we can scope.
		bool can_scope = !g_cl.m_local->m_bIsScoped() && (g_cl.m_weapon_id == AUG || g_cl.m_weapon_id == SG553 || g_cl.m_weapon_type == WEAPONTYPE_SNIPER_RIFLE);

		// do not scope while in air ^_^
		if (can_scope && g_menu.main.aimbot.zoom.get() == 1 && g_cl.m_local->m_fFlags() & (1 << 0) && g_cl.m_flags & (1 << 0)) {
			g_cl.m_cmd->m_buttons |= IN_ATTACK2;
			return;
		}

		if (can_scope) {
			// always.
			if (g_menu.main.aimbot.zoom.get() == 1) {
				g_cl.m_cmd->m_buttons |= IN_ATTACK2;
				return;
			}

			// hitchance fail.
			else if (g_menu.main.aimbot.zoom.get() == 2 && on && !hit) {
				g_cl.m_cmd->m_buttons |= IN_ATTACK2;
				return;
			}
		}

		if (hit || !on) {
			// right click attack.
			if (g_menu.main.config.mode.get() == 1 && g_cl.m_weapon_id == REVOLVER)
				g_cl.m_cmd->m_buttons |= IN_ATTACK2;

			// left click attack.
			else
				g_cl.m_cmd->m_buttons |= IN_ATTACK;
		}
	}
}

float Aimbot::GetIdealAccuracyBoost() {

	// @ruka: ok so this is rly dumb but most p2cs do it
	// @ruka: find good values for each weapon etc..
	// @ruka: for now it will be 50%
	// @evitable : use slider then nigger.

	if (g_cl.m_weapon_id == REVOLVER)
		return 0.07f; // kaaba / pandora

	return g_menu.main.aimbot.dmg_accuracy.get() / 100.f; //0.5f;
}

bool Aimbot::CheckHitchance(Player* player, const ang_t& angle, int aimed_hitbox) {
	if (!player || !player->alive())
		return false;



	AimPlayer* data = &g_aimbot.m_players[player->index() - 1];


	if (!data)
		return false;

	constexpr float HITCHANCE_MAX = 100.f;
	constexpr int   SEED_MAX = 255;

	// we cant fire so nop
	if (!g_cl.m_weapon_fire)
		return false;

	// thx alpha
	float percentage = 0.34f;
	float v4 = percentage * (g_cl.m_local->m_bIsScoped() ? g_cl.m_weapon_info->m_max_player_speed_alt : g_cl.m_weapon_info->m_max_player_speed);

	if (g_menu.main.aimbot.wait_accurate.get()) {
		if (g_cl.m_local->m_vecVelocity().length() > v4) {
			return false;
		}
	}


	if (g_menu.main.aimbot.delay_unduck.get() && g_cl.m_local->m_flDuckAmount() >= 0.125f) {
		if (g_cl.m_flPreviousDuckAmount > g_cl.m_local->m_flDuckAmount()) {
			return false;
		}
	}

	float hc = g_menu.main.aimbot.hitchance_amount.get();

	if (hc <= 0.f)
		return true;

	if (g_cl.m_weapon_id == ZEUS)
		hc = g_menu.main.aimbot.zeusbot_hc.get();
	else if (!(g_cl.m_local->m_fFlags() & FL_ONGROUND))
		hc = g_menu.main.aimbot.hitchance_amount_air.get();

	// @ruka: its cancer but i rly dont understand how to fix this
	if (g_cl.m_flags & FL_ONGROUND && g_cl.m_old_flags & FL_ONGROUND) {
		if (g_cl.m_local->m_flDuckAmount() && g_cl.m_local->m_vecVelocity().length_2d() > 40.f) {

			// @ruka: if its been less than 1s, and we have a duck amount + we're fast
			// @ruka: add some chance to "compensate" for landing
			if (g_csgo.m_globals->m_realtime - g_cl.m_last_in_air_time <= 1.f)
				hc += 30.f;
		}
	}

	// get player hp.
	float goal_damage = 1.f;
	int hp = std::min(100, player->m_iHealth());

	if (g_cl.m_weapon_id == ZEUS) {
		goal_damage = hp + 1;
	}
	else {
		goal_damage = g_aimbot.m_damage_toggle ? g_menu.main.aimbot.override_dmg_value.get() : g_menu.main.aimbot.minimal_damage.get();

		if (goal_damage >= 100 || g_menu.main.aimbot.minimal_damage_hp.get())
			goal_damage = hp + (goal_damage - 100);
	}

	float accuracy_boost = GetIdealAccuracyBoost();
	hc = std::clamp(hc, 1.f, 100.f);

	vec3_t     start{ g_cl.m_shoot_pos }, end, fwd, right, up, dir, wep_spread;
	float      inaccuracy, spread;
	CGameTrace tr;
	float     total_hits{ }, needed_hits{ (hc / HITCHANCE_MAX) * SEED_MAX };

	// get needed directional vectors.
	math::AngleVectors(angle, &fwd, &right, &up);

	// store off inaccuracy / spread ( these functions are quite intensive and we only need them once ).
	inaccuracy = g_cl.m_weapon->GetInaccuracy();
	spread = g_cl.m_weapon->GetSpread();
	float recoil_index = g_cl.m_weapon->m_flRecoilIndex();

	const vec3_t backup_origin = player->m_vecOrigin();
	const vec3_t backup_abs_origin = player->GetAbsOrigin();
	const ang_t backup_abs_angles = player->GetAbsAngles();
	const vec3_t backup_obb_mins = player->m_vecMins();
	const vec3_t backup_obb_maxs = player->m_vecMaxs();
	const auto backup_cache = player->m_BoneCache().m_pCachedBones;

	// quick function
	auto restore = [&]() -> void {
		player->m_vecOrigin() = backup_origin;
		player->SetAbsOrigin(backup_abs_origin);
		player->SetAbsAngles(backup_abs_angles);
		player->m_vecMins() = backup_obb_mins;
		player->m_vecMaxs() = backup_obb_maxs;
		player->m_BoneCache().m_pCachedBones = backup_cache;
	};


	player->SetAbsOrigin(m_record->m_origin);
	player->SetAbsAngles(m_record->m_abs_ang);
	player->m_vecOrigin() = m_record->m_origin;
	player->m_vecMins() = m_record->m_mins;
	player->m_vecMaxs() = m_record->m_maxs;
	player->m_BoneCache().m_pCachedBones = m_record->m_bones;

	// iterate all possible seeds.
	for (int i{ 0 }; i <= SEED_MAX; i++) {

		// get spread.
		wep_spread = g_cl.m_weapon->CalculateSpread(i, inaccuracy, spread);

		// get spread direction.
		dir = (fwd + (right * wep_spread.x) + (up * wep_spread.y)).normalized();

		// get end of trace.
		end = start + (dir * g_cl.m_weapon_info->m_range);

		// setup ray and trace.
		g_csgo.m_engine_trace->ClipRayToEntity(Ray(start, end), MASK_SHOT_HULL | CONTENTS_HITBOX, player, &tr);


		// if hitgroup is the same or we have strict hitbox off
		bool accurate_hitbox = (to_hitgroup(tr.m_hitbox) == to_hitgroup(g_aimbot.m_hitbox) || g_menu.main.aimbot.hitbox_accuracy.get() == 0);

		// check if we hit a valid player / hitgroup on the player and increment total hits.
		if (tr.m_entity == player && accurate_hitbox ) {

			penetration::PenetrationInput_t in;
			in.m_damage = 1.f;
			in.m_damage_pen = 1.f;
			in.m_can_pen = g_cl.m_weapon_id == ZEUS ? false : true;
			in.m_target = player;
			in.m_from = g_cl.m_local;
			in.m_pos = end;

			penetration::PenetrationOutput_t out;

			bool hit = penetration::run(&in, &out);

			if (g_menu.main.aimbot.enable_accuracy.get()) {
				if (hit && (out.m_damage >= goal_damage * accuracy_boost))
					++total_hits;
			}
			else if (hit)
				++total_hits;
		}

		// we cant make it anymore.
		if ((SEED_MAX - i + total_hits) < needed_hits) {
			restore();
			return false;
		}
	}

	restore();
	m_hitchance = (total_hits / static_cast<float>(SEED_MAX)) * HITCHANCE_MAX;
	return total_hits >= needed_hits;
}

bool AimPlayer::SetupHitboxPoints(LagRecord* record, int index, std::vector< vec3_t >& points) {
	// reset points.
	points.clear();

	const model_t* model = this->m_player->GetModel();
	if (!model)
		return false;

	studiohdr_t* hdr = g_csgo.m_model_info->GetStudioModel(model);
	if (!hdr)
		return false;

	mstudiohitboxset_t* set = hdr->GetHitboxSet(this->m_player->m_nHitboxSet());
	if (!set)
		return false;

	mstudiobbox_t* bbox = set->GetHitbox(index);
	if (!bbox)
		return false;

	// get hitbox scales.
	float scale = g_menu.main.aimbot.scale.get() / 100.f;

	// big inair fix.
	if (!(record->m_pred_flags & FL_ONGROUND) && scale > 0.7f)
		scale = 0.7f;

	float bscale = g_menu.main.aimbot.body_scale.get() / 100.f;

	// big duck fix.
	//if (!(record->m_pred_flags & FL_DUCKING) && bscale > 0.6f)
	//	bscale = 0.6f;

	// these indexes represent boxes.
	if (bbox->m_radius <= 0.f) {
		// references: 
		//      https://developer.valvesoftware.com/wiki/Rotation_Tutorial
		//      CBaseAnimating::GetHitboxBonePosition
		//      CBaseAnimating::DrawServerHitboxes

		// convert rotation angle to a matrix.
		matrix3x4_t rot_matrix;
		g_csgo.AngleMatrix(bbox->m_angle, rot_matrix);

		// apply the rotation to the entity input space ( local ).
		matrix3x4_t matrix;
		math::ConcatTransforms(record->m_bones[bbox->m_bone], rot_matrix, matrix);

		// extract origin from matrix.
		vec3_t origin = matrix.GetOrigin();

		// compute raw center point.
		vec3_t center = (bbox->m_mins + bbox->m_maxs) / 2.f;

		// the feet hiboxes have a side, heel and the toe.
		if (index == HITBOX_R_FOOT || index == HITBOX_L_FOOT) {

			// side is more optimal then center.
			points.push_back({ center.x, center.y, center.z });

			if (g_menu.main.aimbot.multipoint.get(3)) {

				// get point offset relative to center point
				// and factor in hitbox scale.
				float d2 = (bbox->m_mins.x - center.x) * scale;

				// heel.
				points.push_back({ center.x + d2, center.y, center.z });
			}
		}

		// nothing to do here we are done.
		if (points.empty())
			return false;

		// rotate our bbox points by their correct angle
		// and convert our points to world space.
		for (auto& p : points) {
			// VectorRotate.
			// rotate point by angle stored in matrix.
			p = { p.dot(matrix[0]), p.dot(matrix[1]), p.dot(matrix[2]) };

			// transform point to world space.
			p += origin;
		}
	}

	// these hitboxes are capsules.
	else {
		// factor in the pointscale.
		float r = bbox->m_radius * scale;
		float br = bbox->m_radius * bscale;

		// compute raw center point.
		vec3_t center = (bbox->m_mins + bbox->m_maxs) / 2.f;

		// head has 5 points.
		if (index == HITBOX_HEAD) {
			// add center.
			points.push_back(center);

			if (g_menu.main.aimbot.multipoint.get(0)) {
				// rotation matrix 45 degrees.
				// https://math.stackexchange.com/questions/383321/rotating-x-y-points-45-degrees
				// std::cos( deg_to_rad( 45.f ) )
				constexpr float rotation = 0.70710678f;

				// top/back 45 deg.
				// this is the best spot to shoot at.
				points.push_back({ bbox->m_maxs.x + (rotation * r), bbox->m_maxs.y + (-rotation * r), bbox->m_maxs.z });

				// right.
				points.push_back({ bbox->m_maxs.x, bbox->m_maxs.y, bbox->m_maxs.z + r });

				// left.
				points.push_back({ bbox->m_maxs.x, bbox->m_maxs.y, bbox->m_maxs.z - r });

				// back.
				points.push_back({ bbox->m_maxs.x, bbox->m_maxs.y - r, bbox->m_maxs.z });

				// if he's backward, standing & down pitch, add this point
				if (record->m_velocity.length() <= 0.1f && record->m_eye_angles.x >= 85.f && !record->m_sideway) {
				
					// top point.
					points.push_back({ bbox->m_maxs.x + r, bbox->m_maxs.y, bbox->m_maxs.z });
				}
			}
		}

		// body has 4 points
		// center + back + left + right
		else if (index == HITBOX_PELVIS) {
			if (g_menu.main.aimbot.multipoint.get(2)) {
				points.push_back({ center.x, center.y, bbox->m_maxs.z + br });
				points.push_back({ center.x, center.y, bbox->m_mins.z - br });
				points.push_back({ center.x, bbox->m_maxs.y - br, center.z });
			}
		}
		else if (index == HITBOX_CHEST) {
			if (g_menu.main.aimbot.multipoint.get(1)) {
				points.push_back({ center.x, center.y, bbox->m_maxs.z + br });
				points.push_back({ center.x, center.y, bbox->m_mins.z - br });
				points.push_back({ center.x, bbox->m_maxs.y - br, center.z });
			}
		}
		// exact same as pelvis but inverted ( god knows what theyre doing at valve )
		else if (index == HITBOX_BODY) {
			points.push_back(center);

			if (g_menu.main.aimbot.multipoint.get(2)) {
				points.push_back({ center.x, bbox->m_maxs.y - br, center.z });
			}
		}

		// other stomach/chest hitboxes have 3 points.
		else if (index == HITBOX_THORAX) {
			// add center.
			points.push_back(center);

			// add extra point on back.
			if (g_menu.main.aimbot.multipoint.get(1)) {
				points.push_back({ center.x, bbox->m_maxs.y - br, center.z });
			}
		}

		else if (index == HITBOX_R_CALF || index == HITBOX_L_CALF) {
			// add center.
			points.push_back(center);

			// half bottom.
			if (g_menu.main.aimbot.multipoint.get(3))
				points.push_back({ bbox->m_maxs.x - (bbox->m_radius / 2.f), bbox->m_maxs.y, bbox->m_maxs.z });
		}

		else if (index == HITBOX_R_THIGH || index == HITBOX_L_THIGH) {
			// add center.
			points.push_back(center);
		}

		// arms get only one point.
		else if (index == HITBOX_R_UPPER_ARM || index == HITBOX_L_UPPER_ARM) {
			// elbow.
			points.push_back({ bbox->m_maxs.x + bbox->m_radius, center.y, center.z });
		}

		// nothing left to do here.
		if (points.empty())
			return false;

		// transform capsule points.`
		for (auto& p : points)
			math::VectorTransform(p, record->m_bones[bbox->m_bone], p);
	}

	return true;
}

// thanks alpha
bool Aimbot::CanHit(const vec3_t start, const vec3_t end, LagRecord* record, int box, bool in_shot, BoneArray* bones)
{
	if (!record || !record->m_player)
		return false;

	// always try to use our aimbot matrix first.
	auto matrix = record->m_bones;

	// this is basically for using a custom matrix.
	if (in_shot)
		matrix = bones;

	if (!matrix)
		return false;

	const model_t* model = record->m_player->GetModel();
	if (!model)
		return false;

	studiohdr_t* hdr = g_csgo.m_model_info->GetStudioModel(model);
	if (!hdr)
		return false;

	mstudiohitboxset_t* set = hdr->GetHitboxSet(record->m_player->m_nHitboxSet());
	if (!set)
		return false;

	mstudiobbox_t* bbox = set->GetHitbox(box);
	if (!bbox)
		return false;

	vec3_t min, max;
	const auto is_capsule = bbox->m_radius != -1.f;

	if (is_capsule)
	{
		math::VectorTransform(bbox->m_mins, record->m_bones[bbox->m_bone], min);
		math::VectorTransform(bbox->m_maxs, record->m_bones[bbox->m_bone], max);
		const auto dist = math::SegmentToSegment(start, end, min, max);

		// should be <=
		if (dist <= bbox->m_radius)
			return true;

		return false;
	}
	else
	{
		CGameTrace trace;
		g_csgo.m_engine_trace->ClipRayToEntity(Ray(start, end), MASK_SHOT_HULL | CONTENTS_HITBOX, record->m_player, &trace);
		return (trace.m_entity && trace.m_entity->index() == record->m_player->index());
	}
}

// @evitable: use this it's better and more accurate.
// @evitable: s/o kaaba.su <3
bool Aimbot::CanHitPlayer(LagRecord* pRecord, const vec3_t& vecEyePos, const vec3_t& vecEnd, int iHitboxIndex) {
	auto hdr = *(studiohdr_t**)pRecord->m_player->m_studioHdr();
	if (!hdr)
		return false;

	auto pHitboxSet = hdr->GetHitboxSet(pRecord->m_player->m_nHitboxSet());

	if (!pHitboxSet)
		return false;

	auto pHitbox = pHitboxSet->GetHitbox(iHitboxIndex);

	if (!pHitbox)
		return false;

	const auto pMatrix = pRecord->m_bones;
	if (!pMatrix)
		return false;

	const vec3_t vecServerImpact = vec3_t{ g_visuals.m_impacts.back().x, g_visuals.m_impacts.back().y, g_visuals.m_impacts.back().z };

	const float flRadius = pHitbox->m_radius;
	const bool bCapsule = flRadius != -1.f;

	vec3_t vecMin;
	vec3_t vecMax;

	math::VectorTransform(pHitbox->m_mins, pRecord->m_bones[pHitbox->m_bone], vecMin);
	math::VectorTransform(pHitbox->m_maxs, pRecord->m_bones[pHitbox->m_bone], vecMax);

	auto dir = vecServerImpact - vecEyePos;
	dir.normalize();

	const bool bIntersected = bCapsule ? math::IntersectSegmentToSegment(vecEyePos, vecServerImpact, vecMin, vecMax, flRadius) : math::IntersectionBoundingBox(vecEyePos, vecEnd, vecMin, vecMax);

	return bIntersected;

	//bHitIntersection = bIsCapsule ?
	//	math::IntersectSegmentToSegment(vecEyePos, vecEnd, vecMin, vecMax, pHitbox->m_radius * 1.01f) : math::IntersectionBoundingBox(vecEyePos, vecEnd, vecMin, vecMax);//( tr.hit_entity == pRecord->player && ( tr.hitgroup >= Hitgroup_Head && tr.hitgroup <= Hitgroup_RightLeg ) || tr.hitgroup == Hitgroup_Gear );
};


bool AimPlayer::GetBestAimPosition(vec3_t& aim, float& damage, int& hitbox, int& hitgroup, int& awall_hitbox, LagRecord* record) {
	bool                  skip_other_points{ false }, skip_other_hitboxes{ false };
	float                 dmg, pendmg;
	HitscanData_t         scan;
	std::vector< vec3_t > points;


	if (!this)
		return false;

	if (!this->m_player)
		return false;


	if (g_menu.main.aimbot.wait_lc.get() && record->broke_lc())
		return false;

	// get player hp.
	int hp = std::min(100, m_player->m_iHealth());

	if (g_cl.m_weapon_id == ZEUS) {
		dmg = pendmg = hp + 1;
	}
	else {
		dmg = g_aimbot.m_damage_toggle ? g_menu.main.aimbot.override_dmg_value.get() : g_menu.main.aimbot.minimal_damage.get();

		if (dmg >= 100 || g_menu.main.aimbot.minimal_damage_hp.get())
			dmg = hp + (dmg - 100);

		// fuck pen damage 
		pendmg = dmg;
	}


	const vec3_t backup_origin = m_player->m_vecOrigin();
	const vec3_t backup_abs_origin = m_player->GetAbsOrigin();
	const ang_t backup_abs_angles = m_player->GetAbsAngles();
	const vec3_t backup_obb_mins = m_player->m_vecMins();
	const vec3_t backup_obb_maxs = m_player->m_vecMaxs();
	const auto backup_cache = m_player->m_BoneCache().m_pCachedBones;

	// quick functions
	auto restore = [&]() -> void {

		m_player->m_vecOrigin() = backup_origin;
		m_player->SetAbsOrigin(backup_abs_origin);
		m_player->SetAbsAngles(backup_abs_angles);
		m_player->m_vecMins() = backup_obb_mins;
		m_player->m_vecMaxs() = backup_obb_maxs;
		m_player->m_BoneCache().m_pCachedBones = backup_cache;
	};


	m_player->SetAbsOrigin(record->m_origin);
	m_player->SetAbsAngles(record->m_abs_ang);
	m_player->m_vecOrigin() = record->m_origin;
	m_player->m_vecMins() = record->m_mins;
	m_player->m_vecMaxs() = record->m_maxs;
	m_player->m_BoneCache().m_pCachedBones = record->m_bones;

	bool done = false;

	// iterate hitboxes.
	for (const auto& it : m_hitboxes) {
		done = false;

		// setup points on hitbox.
		if (!SetupHitboxPoints(record, it.m_index, points))
			continue;

		// iterate points on hitbox.
		for (const auto& point : points) {
			penetration::PenetrationInput_t in;

			in.m_damage = dmg;
			in.m_damage_pen = dmg;
			in.m_can_pen = g_cl.m_weapon_id == ZEUS ? false : true;
			in.m_target = m_player;
			in.m_from = g_cl.m_local;
			in.m_pos = point;

			penetration::PenetrationOutput_t out;

			// we can hit p!
			if (penetration::run(&in, &out)) {

				// fix head behind body situations
				if (it.m_index <= HITBOX_LOWER_NECK && out.m_hitgroup != HITGROUP_HEAD && g_menu.main.aimbot.hitbox_accuracy.get())
					continue;

				// this hitbox requires lethality to get selected, if that is the case.
				// we are done, stop now.
				if (it.m_index > 2 && it.m_index <= 5 && out.m_damage >= m_player->m_iHealth() && g_menu.main.aimbot.baim1.get(3))
					done = true;

				// 2 shots will be sufficient to kill.
				else if (it.m_mode == HitscanMode::PREFER && (out.m_damage >= m_player->m_iHealth() / 2.f 
					|| g_menu.main.aimbot.prefer_mode.get() == 0)) 
					done = true; // strict baim mode & dmg >= hp / 2 or default baim mode

				// NOTE: cannot be worse since checking for higher damage lol nice retard evitable
				else if (out.m_damage > scan.m_damage) {
					// save new best data.
					scan.m_damage = out.m_damage;
					scan.m_pos = point;
					scan.m_hitbox = it.m_index;
					scan.m_hitgroup = out.m_hitgroup;
					scan.m_awall_index = out.m_hitgroup;

					/// if center lethal, skip other multipoints
					if (point == points.front() 
						&& out.m_damage >= m_player->m_iHealth())
						break; 
				}

				// we found a preferred / lethal hitbox.
				if (done) {
					// save new best data.
					scan.m_damage = out.m_damage;
					scan.m_pos = point;
					scan.m_hitbox = it.m_index;
					scan.m_hitgroup = out.m_hitgroup;
					scan.m_awall_index = out.m_hitgroup;
					break;
				}
			}
		}

		// ghetto break out of outer loop.
		if (done)
			break;
	}

	restore();

	// we found something that we can damage.
	// set out vars.
	if (scan.m_damage > 0.f) {
		aim = scan.m_pos;
		damage = scan.m_damage;
		hitbox = scan.m_hitbox;
		hitgroup = scan.m_hitgroup;
		awall_hitbox = scan.m_awall_index;
		return true;
	}

	return false;
}

void Aimbot::apply() {
	bool attack, attack2;

	// attack states.
	attack = (g_cl.m_cmd->m_buttons & IN_ATTACK);
	attack2 = (g_cl.m_weapon_id == REVOLVER && g_cl.m_cmd->m_buttons & IN_ATTACK2);

	// ensure we're attacking.
	if (attack || attack2) {
		// choke every shot.
		if (!g_input.GetKeyState(g_menu.main.movement.fakewalk.get()))
			*g_cl.m_packet = g_cl.m_lag >= 14 || !g_menu.main.antiaim.lag_shot.get();

		if (m_target) {
			// make sure to aim at un-interpolated data.
			// do this so BacktrackEntity selects the exact record.
			if (m_record && !m_record->m_broke_lc)
				g_cl.m_cmd->m_tick = game::TIME_TO_TICKS(m_record->m_sim_time + g_cl.m_lerp);

			// set angles to target.
			g_cl.m_cmd->m_view_angles = m_angle;

			// if not silent aim, apply the viewangles.
			if (!g_menu.main.aimbot.silent.get())
				g_csgo.m_engine->SetViewAngles(m_angle);

			if (g_menu.main.aimbot.debugaim.get())
				g_visuals.DrawHitboxMatrix(m_record, colors::red, 3.f);

			// store fired shot.
			g_shots.OnShotFire(m_target, m_damage, -1, m_record, m_hitbox, m_hitgroup, m_aim);
		}

		// norecoil.
		if (g_menu.main.aimbot.norecoil.get())
			g_cl.m_cmd->m_view_angles -= g_cl.m_local->m_aimPunchAngle() * g_csgo.weapon_recoil_scale->GetFloat();

		// set that we fired.
		g_cl.m_shot = true;
	}
}

void Aimbot::NoSpread() {
	bool    attack2;
	vec3_t  spread, forward, right, up, dir;

	// revolver state.
	attack2 = (g_cl.m_weapon_id == REVOLVER && (g_cl.m_cmd->m_buttons & IN_ATTACK2));

	// get spread.
	spread = g_cl.m_weapon->CalculateSpread(g_cl.m_cmd->m_random_seed, attack2);

	// compensate.
	g_cl.m_cmd->m_view_angles -= { -math::rad_to_deg(std::atan(spread.length_2d())), 0.f, math::rad_to_deg(std::atan2(spread.x, spread.y)) };
}

void Aimbot::update_shoot_pos() {


	const auto anim_state = g_cl.m_local->m_PlayerAnimState();
	if (!anim_state)
		return;

	const auto backup = g_cl.m_local->m_flPoseParameter()[12];
	const auto absorigin = g_cl.m_local->GetAbsOrigin();

	// set pitch, rotation etc
	g_cl.m_local->m_flPoseParameter()[12] = 0.5f;
	g_cl.m_local->SetAbsAngles(g_cl.m_rotation);
	g_cl.m_local->SetAbsOrigin(g_cl.m_local->m_vecOrigin());

	// ??
	if (g_cl.m_local->m_fFlags() & FL_ONGROUND) {
		anim_state->m_on_ground = true;
		anim_state->m_landing = false;
	}

	//g_bones.m_running = true;
	//g_cl.m_local->GameSetupBones(nullptr, 128, BONE_USED_BY_ANYTHING, g_csgo.m_globals->m_curtime);
	//g_bones.m_running = false;

	// get corrected shootpos
	g_cl.m_shoot_pos = g_cl.m_local->wpn_shoot_pos();

	// set to backup
	g_cl.m_local->SetAbsOrigin(absorigin);
	g_cl.m_local->m_flPoseParameter()[12] = backup;

	// set back bones to networked ones
	//std::memcpy(g_cl.m_local->m_BoneCache().m_pCachedBones, g_cl.m_real_bones, sizeof(BoneArray) * 128);
	//std::memcpy(g_cl.m_local->m_BoneAccessor().m_pBones, g_cl.m_real_bones, sizeof(BoneArray) * 128);
}
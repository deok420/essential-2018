#include "includes.h"

LagCompensation g_lagcomp{};;

void LagCompensation::update_lerp() {
	static auto cl_interp = g_csgo.m_cvar->FindVar(HASH("cl_interp"));
	static auto cl_updaterate = g_csgo.m_cvar->FindVar(HASH("cl_updaterate"));
	static auto cl_interp_ratio = g_csgo.m_cvar->FindVar(HASH("cl_interp_ratio"));

	const auto a2 = cl_updaterate->GetFloat();
	const auto a1 = cl_interp->GetFloat();
	const auto v2 = cl_interp_ratio->GetFloat() / a2;
	g_cl.m_lerp = fmaxf(a1, v2);
}


bool LagCompensation::StartPrediction( AimPlayer* data ) {
	// we have no data to work with.
	// this should never happen if we call this
	if (data->m_records.empty())
		return false;

	// meme.
	if (data->m_player->dormant())
		return false;

	// ez fix.
	if (data->m_records.size() < 2)
		return false;

	// compute the true amount of updated records
	// since the last time the player entered pvs.
	size_t size{};

	// iterate records.
	for (const auto &it : data->m_records) {
		if (it->dormant())
			break;

		// increment total amount of data.
		++size;
	}

	// get first record.
	LagRecord* record = data->m_records[0].get();

	if (record->immune() || record->dormant() || record->m_flags & FL_ONGROUND)
		return false;

	// reset all prediction related variables.
	// this has been a recurring problem in all my hacks lmfao.
	// causes the prediction to stack on eachother.
	record->predict();

	record->m_broke_lc = record->broke_lc();

	// we are not breaking lagcomp at this point.
	// return false so it can aim at all the records it once
	// since server-sided lagcomp is still active and we can abuse that.
	if (!record->m_broke_lc)
		return false;

	int simulation = game::TIME_TO_TICKS(record->m_sim_time);

	// this is too much lag to fix.
	if (std::abs(g_cl.m_arrival_tick - simulation) >= 128)
		return g_menu.main.aimbot.lagfix.get() == 1 ? false : true;

	// compute the amount of lag that we will predict for, if we have one set of data, use that.
	// if we have more data available, use the prevoius lag delta to counter weird fakelags that switch between 14 and 2.
	int lag = game::TIME_TO_TICKS(record->m_lag_time);

	if (lag <= 0 || lag > 17)
		return false;

	// clamp this just to be sure.
	math::clamp(lag, 1, 15);

	// get the delta in ticks between the last server net update
	// and the net update on which we created this record.
	int updatedelta = g_cl.m_server_tick - record->m_tick;

	// if the lag delta that is remaining is less than the current netlag
	// that means that we can shoot now and when our shot will get processed
	// the origin will still be valid, therefore we do not have to predict.
	if (g_cl.m_latency_ticks <= lag - updatedelta)
		return true; // shoot at it

	// the next update will come in, wait for it.
	int next = record->m_tick + 1;
	if (next + lag >= g_cl.m_arrival_tick)
		return true; // shoot at it

	return true;
}

void LagCompensation::PlayerMove( LagRecord* record ) {
	vec3_t                start, end, normal;
	CGameTrace            trace;
	CTraceFilterWorldOnly filter;

	// define trace start.
	start = record->m_pred_origin;

	// move trace end one tick into the future using predicted velocity.
	end = start + ( record->m_pred_velocity * g_csgo.m_globals->m_interval );

	// trace.
	g_csgo.m_engine_trace->TraceRay( Ray( start, end, record->m_mins, record->m_maxs ), CONTENTS_SOLID, &filter, &trace );

	// we hit shit
	// we need to fix hit.
	if( trace.m_fraction != 1.f ) {

		// fix sliding on planes.
		for( int i{}; i < 2; ++i ) {
			record->m_pred_velocity -= trace.m_plane.m_normal * record->m_pred_velocity.dot( trace.m_plane.m_normal );

			float adjust = record->m_pred_velocity.dot( trace.m_plane.m_normal );
			if( adjust < 0.f )
				record->m_pred_velocity -= ( trace.m_plane.m_normal * adjust );

			start = trace.m_endpos;
			end   = start + ( record->m_pred_velocity * ( g_csgo.m_globals->m_interval * ( 1.f - trace.m_fraction ) ) );

			g_csgo.m_engine_trace->TraceRay( Ray( start, end, record->m_mins, record->m_maxs ), CONTENTS_SOLID, &filter, &trace );
			if( trace.m_fraction == 1.f )
				break;
		}
	}

	// set new final origin.
	start = end = record->m_pred_origin = trace.m_endpos;

	// move endpos 2 units down.
	// this way we can check if we are in/on the ground.
	end.z -= 2.f;

	// trace.
	g_csgo.m_engine_trace->TraceRay( Ray( start, end, record->m_mins, record->m_maxs ), CONTENTS_SOLID, &filter, &trace );

	// strip onground flag.
	record->m_pred_flags &= ~FL_ONGROUND;

	// add back onground flag if we are onground.
	if( trace.m_fraction != 1.f && trace.m_plane.m_normal.z > 0.7f )
		record->m_pred_flags |= FL_ONGROUND;
}

void LagCompensation::AirAccelerate( LagRecord* record, ang_t angle, float fmove, float smove ) {
	vec3_t fwd, right, wishvel, wishdir;
	float  maxspeed, wishspd, wishspeed, currentspeed, addspeed, accelspeed;

	// determine movement angles.
	math::AngleVectors( angle, &fwd, &right );

	// zero out z components of movement vectors.
	fwd.z   = 0.f;
	right.z = 0.f;

	// normalize remainder of vectors.
	fwd.normalize( );
	right.normalize( );

	// determine x and y parts of velocity.
	for( int i{}; i < 2; ++i )       
		wishvel[ i ] = ( fwd[ i ] * fmove ) + ( right[ i ] * smove );

	// zero out z part of velocity.
	wishvel.z = 0.f;

	// determine maginitude of speed of move.
	wishdir   = wishvel;
	wishspeed = wishdir.normalize( );

	// get maxspeed.
	// TODO; maybe global this or whatever its 260 anyway always.
	maxspeed = record->m_player->m_flMaxspeed( );

	// clamp to server defined max speed.
	if( wishspeed != 0.f && wishspeed > maxspeed )
		wishspeed = maxspeed;

	// make copy to preserve original variable.
	wishspd = wishspeed;

	// cap speed.
	if( wishspd > 30.f )
		wishspd = 30.f;

	// determine veer amount.
	currentspeed = record->m_pred_velocity.dot( wishdir );

	// see how much to add.
	addspeed = wishspd - currentspeed;

	// if not adding any, done.
	if( addspeed <= 0.f )
		return;

	// Determine acceleration speed after acceleration
	accelspeed = g_csgo.sv_airaccelerate->GetFloat( ) * wishspeed * g_csgo.m_globals->m_interval;

	// cap it.
	if( accelspeed > addspeed )
		accelspeed = addspeed;

	// add accel.
	record->m_pred_velocity += ( wishdir * accelspeed );
}

void LagCompensation::PredictAnimations( CCSGOPlayerAnimState* state, LagRecord* record ) {
	struct AnimBackup_t {
		float  curtime;
		float  frametime;
		int    flags;
		int    eflags;
		vec3_t velocity;
	};

	// get player ptr.
	Player* player = record->m_player;

	// backup data.
	AnimBackup_t backup;
	backup.curtime = g_csgo.m_globals->m_curtime;
	backup.frametime = g_csgo.m_globals->m_frametime;
	backup.flags = player->m_fFlags();
	backup.eflags = player->m_iEFlags();
	backup.velocity = player->m_vecAbsVelocity();

	// set globals appropriately for animation.
	g_csgo.m_globals->m_curtime = record->m_pred_time;
	g_csgo.m_globals->m_frametime = g_csgo.m_globals->m_interval;

	// EFL_DIRTY_ABSVELOCITY
	// skip call to C_BaseEntity::CalcAbsoluteVelocity
	player->m_iEFlags() &= ~0x1000;

	// set predicted flags and velocity.
	player->m_fFlags() = record->m_pred_flags;
	player->m_vecAbsVelocity() = record->m_pred_velocity;

	// enable re-animation in the same frame if animated already.
	if (state->m_last_update_frame >= g_csgo.m_globals->m_frame)
		state->m_last_update_frame = g_csgo.m_globals->m_frame - 1;

	// set ground state
	if (record->m_ground) {
		record->m_flags |= FL_ONGROUND;
	}
	else {
		record->m_flags &= ~FL_ONGROUND;
	}

	// get resolver pointer
	bool fake = g_menu.main.aimbot.correct.get();

	// rerun the resolver since we edited the origin.
	if (fake)
		g_resolver.ResolveAngles(player, record);

	// update animations.
	game::UpdateAnimationState(state, record->m_eye_angles);

	// rerun the pose correction cuz we are re-setupping them.
	if (fake)
		g_resolver.ResolvePoses(player, record);

	// get new rotation poses and layers.
	player->GetPoseParameters(record->m_poses);
	player->GetAnimLayers(record->m_server_layers);
	record->m_abs_ang = player->GetAbsAngles();

	// restore globals.
	g_csgo.m_globals->m_curtime = backup.curtime;
	g_csgo.m_globals->m_frametime = backup.frametime;

	// restore player data.
	player->m_fFlags() = backup.flags;
	player->m_iEFlags() = backup.eflags;
	player->m_vecAbsVelocity() = backup.velocity;
}


void C_LagRecord::Setup(Player* pPlayer, bool isBackup) {
	if (!pPlayer)
		return;

	m_vecMins = pPlayer->m_vecMins();
	m_vecMaxs = pPlayer->m_vecMaxs();
	m_vecOrigin = pPlayer->m_vecOrigin();
	m_absAngles = pPlayer->GetAbsAngles();

	std::memcpy(m_pMatrix, pPlayer->m_MatrixBoneCache(),
		pPlayer->m_BoneCache().m_CachedBoneCount * sizeof(matrix3x4_t));

	this->player = pPlayer;
}

void C_LagRecord::Apply(Player* pPlayer) {
	if (!pPlayer || pPlayer == nullptr)
		return;

	if (!this->player || this->player == nullptr)
		return;

	pPlayer->m_vecMins() = this->m_vecMins;
	pPlayer->m_vecMaxs() = this->m_vecMaxs;
	pPlayer->SetAbsAngles(m_absAngles);
	pPlayer->SetAbsOrigin(m_vecOrigin);
	pPlayer->m_vecOrigin() = m_vecOrigin;

	if (m_pMatrix && pPlayer->m_MatrixBoneCache()) {

		std::memcpy(pPlayer->m_MatrixBoneCache(), m_pMatrix,
			pPlayer->m_BoneCache().m_CachedBoneCount * sizeof(matrix3x4_t));

		// force bone cache
		pPlayer->m_iMostRecentModelBoneCounter() = *(int*)g_csgo.bonecounter_shit;
		pPlayer->m_BoneAccessor().m_ReadableBones = pPlayer->m_BoneAccessor().m_WritableBones = 0xFFFFFFFF;
		pPlayer->m_flLastBoneSetupTime() = FLT_MAX;
	}
}

void LagCompensation::BackupOperation(bool restore)
{
	return;

	for (int i = 1; i <= 64; i++)
	{
		if (i == g_csgo.m_engine->GetLocalPlayer())
			continue;

		auto player = g_csgo.m_entlist->GetClientEntity(i)->as<Player*>();

		if (!g_aimbot.IsValidTarget(player))
			continue;

		AimPlayer* lag_data = &g_aimbot.m_players[i - 1];

		if (!lag_data)
			continue;

		if (lag_data->m_ticks_since_dormant <= 2)
			continue;

		if (!restore)
			lag_data->backup_record.Setup(player, true);
		else
			lag_data->backup_record.Apply(player);

	}
}

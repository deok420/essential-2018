#include "includes.h"

Resolver g_resolver{};;

int last_ticks[65];
void Resolver::FindBestAngle(Player* player, LagRecord* record) {
	AimPlayer* data = &g_aimbot.m_players[player->index() - 1];

	// constants.
	constexpr float STEP{ 4.f };
	constexpr float RANGE{ 28.f };

	// get the away angle for this record.
	float away = GetAwayAngle(record);

	vec3_t enemy_eyepos = player->wpn_shoot_pos();

	// construct vector of angles to test.
	std::vector< AdaptiveAngle > angles{ };
	// @ruka: removed, as expected people on edge get put forward instead of sideway lol
	// @evitable: shut the fuck up
	angles.emplace_back(away + 90.f);
	angles.emplace_back(away - 90.f);

	// start the trace at the enemy shoot pos.
	vec3_t start = g_cl.m_local->wpn_shoot_pos();

	// see if we got any valid result.
	// if this is false the path was not obstructed with anything.
	bool valid{ false };

	// iterate vector of angles.
	for (auto it = angles.begin(); it != angles.end(); ++it) {

		// compute the 'rough' estimation of where our head will be.
		vec3_t end{ enemy_eyepos.x + std::cos(math::deg_to_rad(it->m_yaw)) * RANGE,
			enemy_eyepos.y + std::sin(math::deg_to_rad(it->m_yaw)) * RANGE,
			enemy_eyepos.z };

		// draw a line for debugging purposes.
		//g_csgo.m_debug_overlay->AddLineOverlay( start, end, 255, 0, 0, true, 0.1f );

		// compute the direction.
		vec3_t dir = end - start;
		float len = dir.normalize();

		// should never happen.
		if (len <= 0.f)
			continue;

		// step thru the total distance, 4 units per step.
		for (float i{ 0.f }; i < len; i += STEP) {
			// get the current step position.
			vec3_t point = start + (dir * i);

			// get the contents at this point.
			int contents = g_csgo.m_engine_trace->GetPointContents(point, MASK_SHOT_HULL);

			// contains nothing that can stop a bullet.
			if (!(contents & MASK_SHOT_HULL))
				continue;

			float mult = 1.f;

			// over 50% of the total length, prioritize this shit.
			if (i > (len * 0.5f))
				mult = 1.25f;

			// over 90% of the total length, prioritize this shit.
			if (i > (len * 0.75f))
				mult = 1.5f;

			// over 90% of the total length, prioritize this shit.
			if (i > (len * 0.9f))
				mult = 2.f;

			// append 'penetrated distance'.
			it->m_dist += (STEP * mult);

			// mark that we found anything.
			valid = true;
		}
	}

	if (!valid) {
		data->freestand_data = false;
		data->m_anti_fs_angle = away + 180.f;
		return;
	}

	// put the most distance at the front of the container.
	std::sort(angles.begin(), angles.end(),
		[](const AdaptiveAngle& a, const AdaptiveAngle& b) {
		return a.m_dist > b.m_dist;
	});

	// the best angle should be at the front now.
	AdaptiveAngle* best = &angles.front();

	data->m_anti_fs_angle = math::NormalizedAngle(best->m_yaw);
	data->freestand_data = true;
}

void Resolver::AntiFreestand(LagRecord* record) {

	AimPlayer* data = &g_aimbot.m_players[record->m_player->index() - 1];

	if (!data)
		return;
}


bool Resolver::ShouldFreestand(Player* player, LagRecord* record)
{

	return true;
}

LagRecord* Resolver::FindIdealRecord(AimPlayer* data) {
	LagRecord* first_valid, *current;

	if (data->m_records.empty())
		return nullptr;

	// dont backtrack if we have almost no data
	if (data->m_records.size() <= 3)
		return data->m_records.front().get();



	LagRecord* front = data->m_records.front().get();

	// dont backtrack if first record invalid or breaking lagcomp
	if (!front || front->dormant() || front->immune())
		return nullptr;

	if (front->broke_lc())
		return data->m_records.front().get();

	first_valid = nullptr;

	// iterate records.
	for (const auto& it : data->m_records) {
		if (it->dormant() || it->immune() || !it->valid() || it->broke_lc() || !it->m_setup)
			continue;

		// get current record.
		current = it.get();

		// more than 1s delay between front and this record, skip it
		if (game::TIME_TO_TICKS(fabsf(front->m_sim_time - current->m_sim_time)) > 64)
			continue;

		// first record that was valid, store it for later.
		if (!first_valid)
			first_valid = current;

		// try to find a record with a shot, lby update, walking or no anti-aim.
		bool resolved = it->m_resolved;

		// NOTE: removed shot check cus shot doesnt mean they're resolved
		if (resolved)
			return current;
	}

	// none found above, return the first valid record if possible.
	return (first_valid) ? first_valid : nullptr;
}

LagRecord* Resolver::FindLastRecord(AimPlayer* data) {
	LagRecord* current;

	if (data->m_records.empty())
		return nullptr;

	// dont backtrack if we have almost no data
	if (data->m_records.size() <= 3)
		return nullptr;

	LagRecord* front = data->m_records.front().get();

	// dont backtrack if first record invalid or breaking lagcomp
	if (!front || front->dormant() || front->immune() || front->m_broke_lc || !front->m_setup)
		return nullptr;

	// iterate records in reverse.
	for (auto it = data->m_records.crbegin(); it != data->m_records.crend(); ++it) {

		current = it->get();

		// more than 1s delay between front and this record, skip it
		if (game::TIME_TO_TICKS(fabsf(front->m_sim_time - current->m_sim_time)) > 64)
			continue;

		// if this record is valid.
		// we are done since we iterated in reverse.
		if (current->valid() && !current->immune() && !current->dormant() && !current->broke_lc())
			return current;
	}

	return nullptr;
}

LagRecord* Resolver::FindFirstRecord(AimPlayer* data) {

	if (data->m_records.empty())
		return nullptr;

	if (data->m_records.size() <= 2)
		return nullptr;

	LagRecord* front = data->m_records[1].get();

	// dont backtrack if first record invalid or breaking lagcomp
	if (front && !front->dormant() && !front->immune() && !front->m_broke_lc && front->m_setup)
		return front;

	return nullptr;
}

void Resolver::OnBodyUpdate(Player* player, float value) {
	AimPlayer* data = &g_aimbot.m_players[player->index() - 1];

	// NOTE: removed cus i handle all of these in animation
	// set data.
	// data->m_old_body = data->m_body;
	// data->m_body = value;

	if (player->m_vecVelocity().length_2d() > 0.1f || !(player->m_fFlags() & FL_ONGROUND)) {
		data->m_body_proxy_updated = false;
		data->m_body_proxy_old = value;
		data->m_body_proxy = value;
		return;
	}

	// lol
	if (fabsf(math::angle_diff(value, data->m_body_proxy)) >= 22.5f) {
		data->m_body_proxy_old = data->m_body_proxy;
		data->m_body_proxy = value;
		data->m_body_proxy_updated = true;
	}
}

float Resolver::GetAwayAngle(LagRecord* record) {
	vec3_t pos;
	ang_t  away;

	math::VectorAngles(g_cl.m_local->m_vecOrigin() - record->m_origin, away);
	return away.y;
}

void Resolver::MatchShot(AimPlayer* data, LagRecord* record) {
	// do not attempt to do this in nospread mode.
	if (g_menu.main.config.mode.get() == 1)
		return;

	record->m_shot = false;
	Weapon* weapon = data->m_player->GetActiveWeapon();

	if (!weapon)
		return;


	if (fabsf(weapon->m_fLastShotTime() - record->m_sim_time) <= game::TICKS_TO_TIME(14)) {

		// negative / 0 = invalid so..
		if (record->m_sim_ticks == 1)
			record->m_shot = true;
		else
			record->m_eye_angles.x = data->m_valid_pitch;
	}
	else
		data->m_valid_pitch = data->m_player->m_angEyeAngles().x;
}

void Resolver::SetMode(LagRecord* record) {

	// the resolver has 3 modes to chose from.
	// these modes will vary more under the hood depending on what data we have about the player
	// and what kind of hack vs. hack we are playing (mm/nospread).
	if (record->m_flags & FL_ONGROUND) {

		if (record->m_velocity.length() > 0.1f)
			record->m_mode = Modes::RESOLVE_WALK;
		else
			record->m_mode = Modes::RESOLVE_STAND;
	}
	else
		record->m_mode = Modes::RESOLVE_AIR;
}

void Resolver::ResolveAngles(Player* player, LagRecord* record) {
	AimPlayer* data = &g_aimbot.m_players[player->index() - 1];

	// mark this record if it contains a shot.
	MatchShot(data, record);

	// next up mark this record with a resolver mode that will be used.
	SetMode(record);

	// run antifreestand
	FindBestAngle(player, record);

	// we arrived here we can do the actual resolve.
	switch (record->m_mode) {
	case Modes::RESOLVE_WALK:
		ResolveWalk(data, record, player);
		break;
	case Modes::RESOLVE_STAND:
		ResolveStand(data, record, player);
		break;
	case Modes::RESOLVE_AIR:
		ResolveAir(data, record, player);
		break;
	}

	// normalize the eye angles, doesn't really matter but its clean.
	math::NormalizeAngle(record->m_eye_angles.y);
}
void Resolver::ResolveWalk(AimPlayer* data, LagRecord* record, Player* player) {
	// apply lby to eyeangles.
	record->m_eye_angles.y = record->m_body;

	record->m_resolver_mode = "MOVING";
	record->m_resolver_color = colors::green;
	record->m_resolved = true;

	// reset stand and body index.
	if (record->m_velocity.length_2d() > 45.f) {
		data->m_test_index = 0;
		data->m_fakewalk_index = 0;
		data->m_testfs_index = 0;
		data->m_logic_index = 0;
		data->m_airback_index = 0;
		data->m_stand_index4 = 0;
		data->m_air_index = 0;
		data->m_lowlby_index = 0;
		data->m_airlby_index = 0;
		data->m_spin_index = 0;
		data->m_stand_index1 = 0;
		data->m_stand_index2 = 0;
		data->m_stand_index3 = 0;
		data->m_edge_index = 0;
		data->m_fam_reverse_index = 0;
		data->m_back_index = 0;
		data->m_reversefs_index = 0;
		data->m_back_at_index = 0;
		data->m_reversefs_at_index = 0;
		data->m_lastmove_index = 0;
		data->m_lby_index = 0;
		data->m_airlast_index = 0;
		data->m_body_index = 0;
		data->m_lbyticks = 0;
		data->m_sidelast_index = 0;
		data->m_moving_index = 0;

		// reset data about moving and lby
		// @ruka: lol this was never set to true anywhere
		// @ruka: remove that if it fucks up resolver somehow
		data->is_last_moving_lby_valid = true;
	}
	else if (data->m_moving_index >= 1) {
		data->m_moved = false;
		
		ResolveStand(data, record, record->m_player);
	}

	// @evitable: yes it did fuck up, so I put back the old supremacy way.
	// data->m_moved = true;

	// @ruka: reset those data
	data->m_broke_lby = false;
	data->m_has_body_updated = false;

	// @ruka: you might think its stupid
	// @ruka: but if they lby break after running it'll sync with server cycle
	data->m_body_update = FLT_MAX;
	data->m_old_body = record->m_body;

	// reset these too
	data->m_stored_body_old = record->m_body;
	data->m_stored_body = record->m_body;
	data->m_body_proxy_updated = false;
	data->m_body_proxy_old = record->m_body;
	data->m_body_proxy = record->m_body;



	// store em data!!
	data->m_moving_time = record->m_sim_time;
	data->m_moving_body = record->m_body;
	data->m_moving_origin = record->m_origin;

	// copy the last record that this player was walking
	// we need it later on because it gives us crucial data.
	// std::memcpy(&data->m_walk_record, record, sizeof(LagRecord));
}

float GetLBYRotatedYaw(float lby, float yaw)
{
	float delta = math::NormalizedAngle(yaw - lby);
	if (fabs(delta) < 25.f)
		return lby;

	if (delta > 0.f)
		return yaw + 25.f;

	return yaw;
}

void ResolveYawBruteforce(LagRecord* record, Player* player, AimPlayer* data)
{
	auto local_player = g_cl.m_local;
	if (!local_player)
		return;

	record->m_mode = g_resolver.RESOLVE_STAND3;
	float away = g_resolver.GetAwayAngle(record);

	const float at_target_yaw = math::CalcAngle(player->m_vecOrigin(), local_player->m_vecOrigin()).y;

	switch (data->m_stand_index3 % 7)
	{
	case 0:
		record->m_eye_angles.y = GetLBYRotatedYaw(player->m_flLowerBodyYawTarget(), at_target_yaw + 60.f);
		break;
	case 1:
		record->m_eye_angles.y = at_target_yaw + 140.f;
		break;
	case 2:
		record->m_eye_angles.y = at_target_yaw - 75.f;
		break;
	case 3:
		record->m_eye_angles.y = away - 180.f;
		break;
	case 4:
		record->m_eye_angles.y = away + 180.f;
		break;
	case 5:
		record->m_eye_angles.y = away - 90.f;
		break;
	case 6:
		record->m_eye_angles.y = away + 90.f;
		break;
	}
}

float Resolver::SnapToClosestYaw(int given_ang) {
	static std::vector<int> angles = { 0 , 45 , 90 , 135 ,180, 225 , 270 , 315 , 360 };
	int middle = 4;
	float result;
	float diff;
	bool over_mid;
	if (given_ang > angles[middle]) {
		over_mid = true;
		for (std::size_t i = middle; i < angles.size(); i++) {
			float differnece = angles[i] - given_ang;
			if (differnece > -23 && differnece <= 23) {
				result = differnece;
				diff = differnece;
			}
		}
	}
	else {
		over_mid = 0;
		for (std::size_t i = middle; i > 0; i--) {
			// 45 - 22 = 23
			float differnece = angles[i] - given_ang;
			if (differnece > -23 && differnece <= 23) {
				result = differnece;
				diff = differnece;
			}
		}
	}
	return given_ang + result;
}

float Resolver::FindPriorityAngle(LagRecord* record, float yaw, int mode) {
	AimPlayer* data = &g_aimbot.m_players[record->m_player->index() - 1];

	float safest_angle = data->m_anti_fs_angle;
	float away = GetAwayAngle(record);

	if (safest_angle != 0) {
		if (fabs(yaw - safest_angle) > 120) {
			return data->m_anti_fs_angle;
		}
		else {
			if (mode == 0) {
				return yaw;
			}
			else {
				auto diff = yaw - safest_angle - 125;
				return yaw + diff;
			}
		}
	}
	else {
		return away + 180;
	}
}

void Resolver::ResolveStand(AimPlayer* data, LagRecord* record, Player* player) {

	// get predicted away angle for the player.
	float away = GetAwayAngle(record);

	// pointer for easy access.
//	LagRecord* move = &data->m_walk_record;

	// we have a valid moving record.
	if (data->m_moving_time > 0.f) {
		vec3_t delta = data->m_moving_origin - record->m_origin;
		// @evitable: if we check if delta is > 128 we will literally go to !m_moved most of the time, so only reset it when we need to. (aka dormant)
		// DUMBASS RUKA.
		if (delta.length() <= 128.f && !record->m_fake_walk) {
			data->m_moved = true;
		}
	}

	LagRecord* previous_record = nullptr;

	// check
	if (data->m_records.size() >= 2)
		previous_record = data->m_records[1].get();

	if (previous_record)
	{
		// @ruka: BRO HELLO?
		// @evitable: stfu
		if (!record->dormant() && previous_record->dormant())
		{
			// if moved more than 32 units
			// @ruka: no no no no this is wrong this is fully wrong pls hang me
			// @evitable: bameware :DDD
			// @ruka: man it makes no sense it only runs this if dormant but resolver is never ran on dormant :(
			if (((record->m_origin - previous_record->m_origin).length_2d() > 16.f)) {
				data->is_last_moving_lby_valid = false;
			}
		}
	}

	// clean handling for lby delta check
	if (data->m_stored_body != record->m_body) {
		data->m_stored_body_old = data->m_stored_body;
		data->m_stored_body = record->m_body;
	}

	if (record->m_fake_walk)
		data->is_last_moving_lby_valid = false;

	if (previous_record)
	{

		// if proxy updated and we have a timer update
		// or anim lby changed
		bool timer_update = (data->m_body_proxy_updated && data->m_body_update <= record->m_anim_time);
		bool body_update = fabsf(math::AngleDiff(record->m_body, previous_record->m_body)) >= 35.f;

		// avoid micro fake updates (i guess?)
		// TODO: find good value for that
		if ((timer_update || body_update) && data->m_body_index <= 0)
		{
			// update old body.
			data->m_old_body = record->m_body;

			// set angles to current LBY.
			record->m_eye_angles.y = record->m_body;

			// set the resolve mode.
			record->m_mode = Modes::RESOLVE_BODY;
			record->m_resolver_mode = timer_update ? "LBY (P)" : "LBY (U)";
			record->m_resolver_color = colors::green;

			// delay body update.
			data->m_body_update = record->m_anim_time + 1.1f;

			// we've seen them update.
			++data->m_lbyticks;
			data->m_broke_lby = true;
			data->m_has_body_updated = true;

			// exit out of the resolver, thats it.
			return;
		}
	}

	// if he updated only twice, its mostly like direction changes
	// so hes probably still not breaking
	// no, fuck you - evitable.
	// bro if we miss once no need to shoot more - ruka.
	// if (data->m_lbyticks == 0 && data->m_lby_index <= 0)

	// lol this will die against fake 979 but rather count em as flicking 
	// even if theyre using low flick + fake 979
	bool above_120 = record->m_player->GetSequenceActivity(record->m_server_layers[3].m_sequence) == 979;

	if (!data->m_body_proxy_updated && data->m_lby_index <= 0 && !above_120 && data->m_moving_index <= 0)
	{
		record->m_mode = Modes::RESOLVE_LBY;
		record->m_resolver_mode = XOR("LBY");
		record->m_eye_angles.y = record->m_body;

		// kys - ruka
		record->m_resolved = true;
		return;
	}

	if (previous_record)
		if (!previous_record->m_fake_flick && record->m_fake_flick)
			data->m_fakeflick_body = previous_record->m_body;

	if ((data->m_lby_index >= 3 || data->m_body_index >= 2) && data->m_fakeflick_body != record->m_body && record->m_fake_flick) {
		record->m_mode = Modes::RESOLVE_FAKEFLICK;
		record->m_resolver_mode = XOR("FAKEFLICK");
		record->m_eye_angles.y = data->m_fakeflick_body;
		return;
	}

	bool is_sideways = IsYawSideways1(player, data->m_moving_body); //fabs(math::AngleDiff(data->m_moving_body, data->m_anti_fs_angle)) <= 45.f;

	// was !is_sideways
	// this is prob better tho i guess idk

	// @evitable: sometimes this wont work and it will go straight to mode 32 (we dont want that)
	bool is_backwards = !is_sideways;  //fabsf(math::AngleDiff(data->m_moving_body, away + 180.f)) <= 55.f;

	if (!data->m_moved || data->m_moving_index >= 1) {
		record->m_mode = Modes::RESOLVE_STAND3;
		record->m_resolver_mode = "STAND3";

		switch (data->m_stand_index3 % 7) {
		case 0:
			// record->m_eye_angles.y = data->freestand_data ? data->m_anti_fs_angle : away + 180.f;
			// if angle invalid we still set it to backward so :shrug:
			// ^ means if no fs data angle is applied to backward -> no need to check if data valid or not
			record->m_eye_angles.y = data->m_anti_fs_angle;
			break;
		case 1:
			// record->m_eye_angles.y = away + 180.f;
			record->m_eye_angles.y = data->m_anti_fs_angle + 90.f;
			break;
		case 2:
			// record->m_eye_angles.y = away;
			record->m_eye_angles.y = data->m_anti_fs_angle - 90.f;
			break;
		case 3:
			// lol this average between 90 and 45
			record->m_eye_angles.y = record->m_body - 67.f;
			break;
		case 4:
			record->m_eye_angles.y = record->m_body + 67.f;
			break;
		case 5:
			record->m_eye_angles.y = data->m_anti_fs_angle + 180.f;
			break;
		case 6:
			record->m_eye_angles.y = away;
			break;
		}
	}
	else
	{
		float anim_time = record->m_sim_time - data->m_moving_time;
		const float flMoveDelta = fabsf(math::AngleDiff(data->m_moving_body, record->m_body));

		/*if (anim_time < 0.22f) {
			record->m_mode = Modes::RESOLVE_LBY;
			record->m_eye_angles.y = record->m_body;
			record->m_resolver_mode = "LBY (A)";
			record->m_resolver_color = colors::green;
		}*/
		if (data->m_sidelast_index < 1 && IsYawSideways1(player, data->m_moving_body) && flMoveDelta < 12.5f && data->is_last_moving_lby_valid) {
			record->m_mode = Modes::RESOLVE_SIDE_LASTMOVE;
			record->m_eye_angles.y = data->m_moving_body;
			record->m_resolver_mode = "LASTMOVINGLBY(S)";
			record->m_resolver_color = colors::green;
		}
		else if (data->m_reversefs_index < 1 && is_sideways && data->freestand_data) {
			record->m_mode = Modes::RESOLVE_REVERSEFS;
			record->m_eye_angles.y = data->m_anti_fs_angle;
			record->m_resolver_mode = "REV-FS";
			record->m_resolver_color = colors::orange;
		}
		else if (data->m_lastmove_index < 1 && flMoveDelta < 15.f && data->is_last_moving_lby_valid) {
			record->m_mode = Modes::RESOLVE_LASTMOVE;
			record->m_eye_angles.y = data->m_moving_body;
			record->m_resolver_mode = "LASTMOVINGLBY";
			record->m_resolver_color = colors::green;
		}
		else if (data->m_back_index < 1 && (is_backwards || /*(is_sideways && */!data->freestand_data)) {

			// if lastmove backward or we have no freestand data
			record->m_mode = Modes::RESOLVE_BACK;
			record->m_resolver_mode = "BACKWARDS";
			record->m_resolver_color = colors::orange;
			record->m_eye_angles.y = away + 180.0f;
		}
		else if (data->m_lastmove_index >= 1 || data->m_back_index >= 1 && is_backwards && data->m_stand_index2 < 3)
		{
			//set resolve mode
			record->m_mode = Modes::RESOLVE_STAND2;
			switch (data->m_stand_index2 % 3) {
			case 0:
				record->m_eye_angles.y = away + 135.f;
				record->m_resolver_color = colors::orange;
				record->m_resolver_mode = "CROOKED:LEFT";
				break;
			case 1:
				record->m_eye_angles.y = away + 225.f;
				record->m_resolver_color = colors::transparent_red;
				record->m_resolver_mode = "CROOKED:RIGHT";
				break;
			case 2:
				record->m_eye_angles.y = away;
				record->m_resolver_color = colors::transparent_red;
				record->m_resolver_mode = "FORWARD";
				break;
			}
		}
		else if (data->m_reversefs_index >= 1 || data->m_sidelast_index >= 1 && is_sideways && data->m_stand_index1 < 8)
		{
			record->m_mode = Modes::RESOLVE_STAND1;
			switch (data->m_stand_index1 % 8) {
			case 0:
				record->m_eye_angles.y = away + 180.f;
				record->m_resolver_color = colors::orange;
				record->m_resolver_mode = "SIDE:BACK";
				break;
			case 1:
				record->m_eye_angles.y = away - 135.f;
				record->m_resolver_color = colors::orange;
				record->m_resolver_mode = "SIDE:CROOKEDLEFT";
				break;
			case 2:
				record->m_eye_angles.y = away + 225.f;
				record->m_resolver_color = colors::orange;
				record->m_resolver_mode = "SIDE:CROOKEDRIGHT";
				break;
			case 3:
				record->m_eye_angles.y = data->m_anti_fs_angle + 180.f;
				record->m_resolver_color = colors::transparent_red;
				record->m_resolver_mode = "SIDE:INVERT";
				break;
			case 4:
				record->m_eye_angles.y = away + 90.f;
				record->m_resolver_color = colors::transparent_red;
				record->m_resolver_mode = "SIDE:LEFT";
				break;
			case 5:
				record->m_eye_angles.y = away - 90.f;
				record->m_resolver_color = colors::transparent_red;
				record->m_resolver_mode = "SIDE:RIGHT";
				break;
			case 6:
				record->m_eye_angles.y = away + 110.f;
				record->m_resolver_color = colors::transparent_red;
				record->m_resolver_mode = "SIDE:HIGHLEFT";
				break;
			case 7:
				record->m_eye_angles.y = away - 110.f;
				record->m_resolver_color = colors::transparent_red;
				record->m_resolver_mode = "SIDE:HIGHRIGHT";
				break;
			default:
				break;
			}
		}
		else
		{

			record->m_mode = Modes::RESOLVE_TEST_FS;

			// s/o kaaba.
			// @ruka: changed stuff here cus lm + 180 is like the most unsafe bs ever
			switch (data->m_test_index % 6) {
			case 0:
				record->m_resolver_mode = data->is_last_moving_lby_valid ? "TEST:VALID" : "TEST:REVFS";
				// record->m_eye_angles.y = data->is_last_moving_lby_valid ? data->m_moving_body : away + 180.f;
				record->m_eye_angles.y = data->is_last_moving_lby_valid ? data->m_moving_body : data->m_anti_fs_angle;
				break;
			case 1:
				// @ruka: maybe invert side? idk, might fuck up if both are close
			// 	record->m_eye_angles.y = data->freestand_data ? data->m_anti_fs_angle : data->m_moving_body + 180.f;
				record->m_resolver_mode = data->is_last_moving_lby_valid ? "TEST:REVFS" : "TEST:VALID";
				record->m_eye_angles.y = data->is_last_moving_lby_valid ? data->m_anti_fs_angle : data->m_moving_body;
				break;
			case 2:
				record->m_resolver_mode = "TEST:LEFT";
				record->m_eye_angles.y = data->m_moving_body + 90.f;

				break;
			case 3:
				record->m_resolver_mode = "TEST:RIGHT";
				record->m_eye_angles.y = data->m_moving_body - 90.f;
				break;
			case 4:
				if (flMoveDelta > 35.f) {
					record->m_resolver_mode = "TEST:DELTARIGHT";
					record->m_eye_angles.y = data->m_moving_body - flMoveDelta;
				}
				else {
					record->m_resolver_mode = "TEST:HIGHRIGHT";
					record->m_eye_angles.y = data->m_moving_body - 115.f;
				}
				break;
			case 5:
				if (flMoveDelta > 35.f) {
					record->m_resolver_mode = "TEST:DELTALEFT";
					record->m_eye_angles.y = data->m_moving_body + flMoveDelta;
				}
				else {
					record->m_resolver_mode = "TEST:HIGHLEFT";
					record->m_eye_angles.y = data->m_moving_body + 115.f;
				}
				break;
			}
		}
	}
}

void Resolver::StandNS(AimPlayer* data, LagRecord* record) {

	return;
}

void Resolver::ResolveAir(AimPlayer* data, LagRecord* record, Player* player) {


	float away = GetAwayAngle(record);

	record->m_resolver_mode = XOR("AIR");
	record->m_resolved = false;

	// blabla do more stuff later here pls
	data->m_moving_time = -1;
	data->is_last_moving_lby_valid = false;
	data->m_stored_body_old = record->m_body;
	data->m_stored_body = record->m_body;
	data->m_body_proxy_updated = false;
	data->m_body_proxy_old = record->m_body;
	data->m_body_proxy = record->m_body;
	data->m_moved = false;

	LagRecord* previous = nullptr;

	if (data->m_records.size() >= 2) {
		previous = data->m_records[1].get();
	}

	// @xetraprobine: may we use the real previous record and not move pls
	// ima do that and do some checks.
	// @ruka: note tried to clean up in air but idk 

	bool back_is_close = fabsf(math::AngleDiff(record->m_body, away + 180.f)) <= 20.f;

	record->m_resolver_color = colors::transparent_red;
	switch (data->m_air_index % 9) {

	case 0:
		if (fabsf(record->m_body - data->m_moving_body) <= 12.5f)
			record->m_eye_angles.y = data->m_moving_body;
		else
			record->m_eye_angles.y = back_is_close ? away + 180.f : record->m_body;
		break;
	case 1:
		record->m_eye_angles.y = back_is_close ? record->m_body : away + 180.f; // revert
		break;
	case 2:
		record->m_eye_angles.y = away - 150.f;
		break;
	case 3:
		record->m_eye_angles.y = away + 165.f;
		break;
	case 4:
		record->m_eye_angles.y = away - 165.f;
		break;
	case 5:
		record->m_eye_angles.y = away + 135.f;
		break;
	case 6:
		record->m_eye_angles.y = away - 135.f;
		break;
	case 7:
		record->m_eye_angles.y = away + 90.f;
		break;
	case 8:
		record->m_eye_angles.y = away - 90.f;
		break;
	}
}

void Resolver::AirNS(AimPlayer* data, LagRecord* record) {

	return;
}

void Resolver::ResolvePoses(Player* player, LagRecord* record) {

	// @ruka: basically this randomizes lean & lby (for some reasons lol)
	// @ruka: you can add that back but not sure of how revelant it is lmao
	return;
}

bool Resolver::IsYawSideways(Player* entity, float yaw)
{
	const auto at_target_yaw = math::CalcAngle(g_cl.m_local->m_vecOrigin(), entity->m_vecOrigin()).y;
	const float delta = fabs(math::AngleDiff(at_target_yaw, yaw));

	return delta > 25.f && delta < 165.f;
}

bool Resolver::is_sideways(LagRecord* record, float yaw) {
	float diff = std::floor(std::abs(math::AngleDiff(GetAwayAngle(record), yaw)));
	return diff >= 45.f && diff <= 145.f;
}

bool Resolver::IsYawSideways1(Player* entity, float yaw)
{
	const auto at_target_yaw = math::CalcAngle(g_cl.m_local->m_vecOrigin(), entity->m_vecOrigin()).y;
	const float delta = fabs(math::AngleDiff(at_target_yaw, yaw));

	// return delta > 45.f && delta < 135.f;
	return delta > 35.f && delta < 145.f;
}

bool Resolver::IsYawBackwards(Player* entity, float yaw) {
	const auto at_target_yaw = math::CalcAngle(g_cl.m_local->m_vecOrigin(), entity->m_vecOrigin()).y;
	const float delta = fabs(math::AngleDiff(at_target_yaw, yaw));

	return delta > 25.f && delta < 165.f;
}

bool Resolver::IsYawUnknown(Player* entity, float yaw, float antifs_yaw) {
	const float delta = fabs(math::AngleDiff(antifs_yaw, yaw));

	return delta > 45.f && delta < 135.f;
}

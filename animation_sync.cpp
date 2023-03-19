#include "includes.h"

void AimPlayer::CorrectAnimlayers(LagRecord* record) {

	if (!record)
		return;

	CCSGOPlayerAnimState* state = m_player->m_PlayerAnimState();
	LagRecord* previous{ nullptr };

	if (this->m_records.size() > 2) {
		previous = this->m_records[1].get();
	}

	if (previous && !previous->dormant() && previous->m_setup)
	{
		state->m_primary_cycle = previous->m_server_layers[6].m_cycle;
		state->m_move_weight = previous->m_server_layers[6].m_weight;
		state->m_strafe_sequence = previous->m_server_layers[7].m_sequence;
		state->m_strafe_change_weight = previous->m_server_layers[7].m_weight;
		state->m_strafe_change_cycle = previous->m_server_layers[7].m_cycle;
		state->m_acceleration_weight = previous->m_server_layers[12].m_weight;
		this->m_player->SetAnimLayers(previous->m_server_layers);
	}
	else
	{

		if (record->m_flags & FL_ONGROUND) {
			state->m_on_ground = true;
			state->m_landing = false;
		}

		state->m_primary_cycle = record->m_server_layers[6].m_cycle;
		state->m_move_weight = record->m_server_layers[6].m_weight;
		state->m_strafe_sequence = record->m_server_layers[7].m_sequence;
		state->m_strafe_change_weight = record->m_server_layers[7].m_weight;
		state->m_strafe_change_cycle = record->m_server_layers[7].m_cycle;
		state->m_acceleration_weight = record->m_server_layers[12].m_weight;
		state->m_duration_in_air = 0.f;
		this->m_player->m_flPoseParameter()[6] = 0.f;
		this->m_player->SetAnimLayers(record->m_server_layers);

		state->m_last_update_time = (record->m_sim_time - g_csgo.m_globals->m_interval);
	}

}

void AimPlayer::CorrectVelocity(Player* m_player, LagRecord* record, LagRecord* previous, LagRecord* pre_previous, float max_speed) {
	
	constexpr float CS_PLAYER_SPEED_DUCK_MODIFIER = 0.34f;
	constexpr float CS_PLAYER_SPEED_WALK_MODIFIER = 0.52f;


	// set velocity to networked velocity
	record->m_velocity = m_player->m_vecVelocity();

	// we do not have any data to work with
	if (!previous) {

		// is he moving?
		if (record->m_server_layers[6].m_playback_rate > 0.0f && record->m_velocity.length() > 0.f) {

			// can we attempt to fix velocity with our only data?
			if (record->m_server_layers[6].m_weight > 0.0f) {
				float max_anim_speed = max_speed;

				// uh, this might be wrong
				if (m_player->m_flDuckAmount() > 0.f)
					max_anim_speed *= CS_PLAYER_SPEED_DUCK_MODIFIER;
				else if (m_player->m_bIsWalking())
					max_anim_speed *= CS_PLAYER_SPEED_WALK_MODIFIER;

				record->m_velocity *= (record->m_server_layers[6].m_weight * max_anim_speed) / record->m_velocity.length();
			}

			// if onground remove gravity
			if (record->m_flags & FL_ONGROUND)
				record->m_velocity.z = 0.f;
		}
		else // reset velocity if not moving 
			record->m_velocity.Init();

		// we are done
		return;
	}

	// check if player has been on ground for two consecutive ticks
	bool on_ground = record->m_flags & FL_ONGROUND && previous->m_flags & FL_ONGROUND;

	if (on_ground && record->m_server_layers[6].m_playback_rate <= 0.0f)
		record->m_velocity.Init();

	// entity teleported, reset his velocity (thats what server does)
	if ((m_player->m_fEffects() & 8) != 0
		|| m_player->m_ubEFNoInterpParity() != m_player->m_ubEFNoInterpParityOld())
	{
		record->m_velocity.Init();
		return;
	}

	// we are done.
	if (record->m_sim_ticks <= 1) 
		return;

	// get animation lag in time
	float anim_lag_time = game::TICKS_TO_TIME(record->m_anim_ticks);

	// is he in the bounds?
	if (anim_lag_time > 0.f && anim_lag_time < 1.f) {

		record->m_velocity = (record->m_origin - previous->m_origin) / anim_lag_time;

		// they're bhopping
		if (!on_ground) {

			// and spamming duck / changing duck amount
			if ((previous->m_flags & FL_DUCKING) != (record->m_flags & FL_DUCKING))
			{
				// they didnt duck this tick
				float duck_modifier{ -9.f };

				// they ducked this tick
				if (record->m_flags & FL_DUCKING)
					duck_modifier = 9.f;

				// adjust gravity based on this
				record->m_velocity.z += duck_modifier;
			}
		}
	}

	// determine if we can calculate animation speed modifier using server anim overlays
	if (on_ground
		&& record->m_server_layers[11].m_weight > 0.0f
		&& record->m_server_layers[11].m_weight < 1.0f
		&& record->m_server_layers[11].m_playback_rate == record->m_server_layers[11].m_playback_rate)
	{
		// calculate animation speed yielded by anim overlays
		float reversed_weight = 0.35f * (1.0f - record->m_server_layers[11].m_weight);

		// if the reverse weight is valid, calc animation speed
		if (reversed_weight > 0.f && reversed_weight < 1.f)
			record->m_animation_speed = max_speed * (reversed_weight + 0.55f);
	}

	// this velocity is valid ONLY IN ANIMFIX UPDATE TICK!!!
	// so don't store it to record as m_vecVelocity. i added vecAbsVelocity for that, it acts as a animationVelocity.
	if (record->m_animation_speed > 0.0f) {
		record->m_animation_speed /= record->m_velocity.length_2d();
		record->m_velocity.x *= record->m_animation_speed;
		record->m_velocity.y *= record->m_animation_speed;
	}

	// calculate average player direction when bunny hopping at high speed
	if (pre_previous && record->m_velocity.length() >= 320.f) {

		// make sure to only calculate average player direction whenever they're bhopping
		if (previous->m_velocity.length() > 0.f && !on_ground) {

			auto current_direction = math::NormalizedAngle(math::rad_to_deg(atan2(record->m_velocity.y, record->m_velocity.x)));
			auto previous_direction = math::NormalizedAngle(math::rad_to_deg(atan2(previous->m_velocity.y, previous->m_velocity.x)));

			auto average_direction = current_direction - previous_direction;
			average_direction = math::deg_to_rad(math::NormalizedAngle(current_direction + average_direction * 0.5f));

			// modify velocity accordingly
			record->m_velocity.x = cos(average_direction) * record->m_velocity.length_2d();
			record->m_velocity.y = sin(average_direction) * record->m_velocity.length_2d();
		}
	}

	if (!on_ground)
	{
		// correct z velocity.
		// https://github.com/perilouswithadollarsign/cstrike15_src/blob/f82112a2388b841d72cb62ca48ab1846dfcc11c8/game/shared/gamemovement.cpp#L1950
		if (!(record->m_flags & FL_ONGROUND))
			record->m_velocity.z -= g_csgo.sv_gravity->GetFloat() * record->m_lag_time * 0.5f;
		else
			record->m_velocity.z = 0.f;
	}
	else
		record->m_velocity.z = 0.f;
}
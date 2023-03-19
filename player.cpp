#include "includes.h"

void Hooks::DoExtraBoneProcessing(int a2, int a3, int a4, int a5, int a6, int a7) {
	return; // dont fucking call anything
}

void Hooks::BuildTransformations(int a2, int a3, int a4, int a5, int a6, int a7) {
	// cast thisptr to player ptr.
	Player* player = (Player*)this;

	// get bone jiggle.
	int bone_jiggle = *reinterpret_cast<int*>(uintptr_t(player) + 0x291C);

	// null bone jiggle to prevent attachments from jiggling around.
	*reinterpret_cast<int*>(uintptr_t(player) + 0x291C) = 0;

	// call og.
	g_hooks.m_BuildTransformations(this, a2, a3, a4, a5, a6, a7);

	// restore bone jiggle.
	*reinterpret_cast<int*>(uintptr_t(player) + 0x291C) = bone_jiggle;
}

void Hooks::UpdateClientSideAnimation() {



	CCSGOPlayerAnimState* state{ g_cl.m_local->m_PlayerAnimState() };

	if (!state || !g_cl.m_processing)
		return g_hooks.m_UpdateClientSideAnimation(this);

	float poses[24];
	C_AnimationLayer layers[13];


	if (state->m_last_update_frame >= g_csgo.m_globals->m_frame)
		state->m_last_update_frame = g_csgo.m_globals->m_frame - 1;


	g_cl.m_local->GetAnimLayers(layers);
	g_cl.m_local->GetPoseParameters(poses);

	g_cl.m_local->SetAnimLayers(g_cl.m_layers);
	g_cl.m_local->SetPoseParameters(g_cl.m_poses);

	g_hooks.m_UpdateClientSideAnimation(this);

	g_cl.m_local->SetAnimLayers(layers);
	g_cl.m_local->SetPoseParameters(poses);
}

void Hooks::CalcView(vec3_t& eye_origin, vec3_t& eye_angles, float& z_near, float& z_far, float& fov)
{
	// cast thisptr to player ptr.
	Player* player = (Player*)this;

	if (!player || !player->m_bIsLocalPlayer())
		return g_hooks.m_1CalcView(this, eye_origin, eye_angles, z_near, z_far, fov);

	const auto old_use_new_animation_state = player->get<bool>(0x39E1);

	// prevent calls to ModifyEyePosition
	player->set<int>(0x39E1, false);

	// call og.
	g_hooks.m_1CalcView(this, eye_origin, eye_angles, z_near, z_far, fov);

	player->set<int>(0x39E1, old_use_new_animation_state);
}

Weapon* Hooks::GetActiveWeapon() {
	Stack stack;

	static Address ret_1 = pattern::find(g_csgo.m_client_dll, XOR("85 C0 74 1D 8B 88 ? ? ? ? 85 C9"));

	// note - dex; stop call to CIronSightController::RenderScopeEffect inside CViewRender::RenderView.
	if (g_menu.main.visuals.noscope.get()) {
		if (stack.ReturnAddress() == ret_1)
			return nullptr;
	}

	return g_hooks.m_GetActiveWeapon(this);
}

bool Hooks::EntityShouldInterpolate() {
	// cast thisptr to player ptr.
	Player* player = (Player*)this;

	// let us ensure we aren't disabling interpolation on our local player model.
	if (player == g_cl.m_local)
		return true;

	// lets disable interpolation on these niggas.
	if (player->IsPlayer()) {
		return false;
	}

	// call og.
	g_hooks.m_EntityShouldInterpolate(this);

	return false;
}

void Hooks::StandardBlendingRules(int a2, int a3, int a4, int a5, int a6)
{
	// cast thisptr to player ptr.
	Player* player = (Player*)this;

	if (!player
		|| !player->IsPlayer()
		|| !player->alive())
		return g_hooks.m_StandardBlendingRules(this, a2, a3, a4, a5, a6);

	if (player->index() == g_cl.m_local->index())
		a6 |= BONE_USED_BY_BONE_MERGE;
	else // fix arms. @ruka: DONT DO THIS ON LOCAL
		a6 = BONE_USED_BY_SERVER;

	if (player->index() != g_cl.m_local->index())
		player->m_fEffects() |= EF_NOINTERP;

	g_hooks.m_StandardBlendingRules(player, a2, a3, a4, a5, a6);
	player->m_fEffects() &= ~EF_NOINTERP;
}

void CustomEntityListener::OnEntityCreated(Entity* ent) {
	if (ent) {
		// player created.
		if (ent->IsPlayer()) {
			Player* player = ent->as< Player* >();
			
			// access out player data stucture and reset player data.
			AimPlayer* data = &g_aimbot.m_players[player->index() - 1];
			if (data)
				data->reset();

			// get ptr to vmt instance and reset tables.
			VMT* vmt = &g_hooks.m_player[player->index() - 1];
			if (vmt) {
				// init vtable with new ptr.
				vmt->reset();
				vmt->init(player);

				// hook this on every player.
				g_hooks.m_DoExtraBoneProcessing = vmt->add< Hooks::DoExtraBoneProcessing_t >(Player::DOEXTRABONEPROCESSING, util::force_cast(&Hooks::DoExtraBoneProcessing));
				g_hooks.m_EntityShouldInterpolate = vmt->add< Hooks::EntityShouldInterpolate_t >(Player::ENTITYSHOULDINTERPOLATE, util::force_cast(&Hooks::EntityShouldInterpolate));
				g_hooks.m_StandardBlendingRules = vmt->add< Hooks::StandardBlendingRules_t >(Player::STANDARDBLENDINGRULES, util::force_cast(&Hooks::StandardBlendingRules));

				// local gets special treatment.
				if (player->index() == g_csgo.m_engine->GetLocalPlayer()) {
					g_hooks.m_UpdateClientSideAnimation = vmt->add< Hooks::UpdateClientSideAnimation_t >(Player::UPDATECLIENTSIDEANIMATION, util::force_cast(&Hooks::UpdateClientSideAnimation));
				 	g_hooks.m_1CalcView = vmt->add< Hooks::CalcView_t >(270, util::force_cast(&Hooks::CalcView));
					g_hooks.m_GetActiveWeapon = vmt->add< Hooks::GetActiveWeapon_t >(Player::GETACTIVEWEAPON, util::force_cast(&Hooks::GetActiveWeapon));
					g_hooks.m_BuildTransformations = vmt->add< Hooks::BuildTransformations_t >(Player::BUILDTRANSFORMATIONS, util::force_cast(&Hooks::BuildTransformations));
				}
			}
		}
	}
}

void CustomEntityListener::OnEntityDeleted(Entity* ent) {
	// note; IsPlayer doesn't work here because the ent class is CBaseEntity.
	if (ent && ent->index() >= 1 && ent->index() <= 64) {
		Player* player = ent->as< Player* >();

		// access out player data stucture and reset player data.
		AimPlayer* data = &g_aimbot.m_players[player->index() - 1];
		if (data)
			data->reset();

		// get ptr to vmt instance and reset tables.
		VMT* vmt = &g_hooks.m_player[player->index() - 1];
		if (vmt)
			vmt->reset();
	}
}
#include "includes.h"

Chams g_chams{ };;

Chams::model_type_t Chams::GetModelType(const ModelRenderInfo_t& info) {
	// model name.
	//const char* mdl = info.m_model->m_name;

	std::string mdl{ info.m_model->m_name };

	//static auto int_from_chars = [ mdl ]( size_t index ) {
	//	return *( int* )( mdl + index );
	//};

	// little endian.
	//if( int_from_chars( 7 ) == 'paew' ) { // weap
	//	if( int_from_chars( 15 ) == 'om_v' && int_from_chars( 19 ) == 'sled' )
	//		return model_type_t::arms;
	//
	//	if( mdl[ 15 ] == 'v' )
	//		return model_type_t::view_weapon;
	//}

	//else if( int_from_chars( 7 ) == 'yalp' ) // play
	//	return model_type_t::player;

	if (mdl.find(XOR("player")) != std::string::npos && info.m_index >= 1 && info.m_index <= 64)
		return model_type_t::player;

	return model_type_t::invalid;
}

bool Chams::IsInViewPlane(const vec3_t& world) {
	float w;

	const VMatrix& matrix = g_csgo.m_engine->WorldToScreenMatrix();

	w = matrix[3][0] * world.x + matrix[3][1] * world.y + matrix[3][2] * world.z + matrix[3][3];

	return w > 0.001f;
}

void Chams::SetColor(Color col, IMaterial* mat) {
	if (mat)
		mat->ColorModulate(col);

	else
		g_csgo.m_render_view->SetColorModulation(col);
}

void Chams::SetAlpha(float alpha, IMaterial* mat) {
	if (mat)
		mat->AlphaModulate(alpha);

	else
		g_csgo.m_render_view->SetBlend(alpha);
}

void Chams::SetupMaterial(IMaterial* mat, Color col, bool z_flag) {
	SetColor(col);

	// mat->SetFlag( MATERIAL_VAR_HALFLAMBERT, flags );
	mat->SetFlag(MATERIAL_VAR_ZNEARER, z_flag);
	mat->SetFlag(MATERIAL_VAR_NOFOG, z_flag);
	mat->SetFlag(MATERIAL_VAR_IGNOREZ, z_flag);

	g_csgo.m_studio_render->ForcedMaterialOverride(mat);
}

void Chams::init() {
	// create custom materials
	std::ofstream("csgo\\materials\\simple_ignorez_reflective.vmt") << R"#("VertexLitGeneric"{
           "$basetexture" "vgui/white_additive"
            "$ignorez"      "1"
            "$envmap"       "env_cubemap"
            "$normalmapalphaenvmapmask"  "1"
            "$envmapcontrast"             "1"
            "$nofog"        "1"
            "$model"        "1"
            "$nocull"       "0"
            "$selfillum"    "1"
            "$halflambert"  "1"
            "$znearer"      "0"
            "$flat"         "1"
    })#";

	std::ofstream("csgo\\materials\\simple_regular_reflective.vmt") << R"#("VertexLitGeneric" {
            "$basetexture" "vgui/white_additive"
            "$ignorez"      "0"
            "$envmap"       "env_cubemap"
            "$normalmapalphaenvmapmask"  "1"
            "$envmapcontrast"             "1"
            "$nofog"        "1"
            "$model"        "1"
            "$nocull"       "0"
            "$selfillum"    "1"
            "$halflambert"  "1"
            "$znearer"      "0"
            "$flat"         "1"
    })#";

	std::ofstream("csgo/materials/ghost.vmt") << R"#("VertexLitGeneric" {
		    "$basetexture" "models/effects/cube_white"
		    "$additive"                    "1"
		    "$envmap"                    "models/effects/cube_white"
		    "$envmaptint"                "[1.0 1.0. 1.0]"
		    "$envmapfresnel"            "1.0"
		    "$envmapfresnelminmaxexp"    "[0.0 1.0 2.0]"
		    "$alpha"                    "0.99"
	})#";


	// find stupid materials.
	debugambientcube = g_csgo.m_material_system->FindMaterial(XOR("debug/debugambientcube"), XOR("Model textures"));
	debugambientcube->IncrementReferenceCount();

	debugdrawflat = g_csgo.m_material_system->FindMaterial(XOR("debug/debugdrawflat"), XOR("Model textures"));
	debugdrawflat->IncrementReferenceCount();

	glow_armsrace = g_csgo.m_material_system->FindMaterial(XOR("dev/armsrace_glow"), XOR("Model textures"));
	glow_armsrace->IncrementReferenceCount();

	materialMetall = g_csgo.m_material_system->FindMaterial("simple_ignorez_reflective", "Model textures");
	materialMetall->IncrementReferenceCount();

	materialMetallnZ = g_csgo.m_material_system->FindMaterial("simple_regular_reflective", "Model textures");
	materialMetallnZ->IncrementReferenceCount();

	ghost = g_csgo.m_material_system->FindMaterial(XOR("ghost"), XOR("Model textures"));
	ghost->IncrementReferenceCount();
}

bool Chams::OverridePlayer(int index) {
	Player* player = g_csgo.m_entlist->GetClientEntity< Player* >(index);
	if (!player)
		return false;

	// always skip the local player in DrawModelExecute.
	// this is because if we want to make the local player have less alpha
	// the static props are drawn after the players and it looks like aids.
	// therefore always process the local player in scene end.
	if (player->m_bIsLocalPlayer())
		return true;

	// see if this player is an enemy to us.
	bool enemy = g_cl.m_local && player->enemy(g_cl.m_local);

	// we have chams on enemies.
	if (enemy && g_menu.main.players.chams_enemy.get(0))
		return true;

	// we have chams on friendly.
	else if (!enemy && g_menu.main.players.chams_friendly.get(0))
		return true;

	return false;
}

bool Chams::GenerateLerpedMatrix(int ent_index, BoneArray* out) {
	if (!g_csgo.m_engine->IsConnected() || !g_csgo.m_engine->IsInGame() || !g_cl.m_local || !g_cl.m_local->alive())
		return false;

	Player* ent = g_csgo.m_entlist->GetClientEntity< Player* >(ent_index);
	if (!ent)
		return false;

	if (!g_aimbot.IsValidTarget(ent))
		return false;

	AimPlayer* data = &g_aimbot.m_players[ent_index - 1];
	if (!data || data->m_records.empty())
		return false;

	if (data->m_records.front().get()->m_broke_lc)
		return false;

	if (data->m_records.size() < 3)
		return false;

	auto* channel_info = g_csgo.m_engine->GetNetChannelInfo();
	if (!channel_info)
		return false;

	float max_unlag = g_csgo.m_cvar->FindVar(HASH("sv_maxunlag"))->GetFloat();

	// start from begin
	for (auto it = data->m_records.begin(); it != data->m_records.end(); ++it) {

		LagRecord* last_first{ nullptr };
		LagRecord* last_second{ nullptr };

		if (it->get()->valid() && it + 1 != data->m_records.end() && !(it + 1)->get()->valid() && !(it + 1)->get()->dormant()) {
			last_first = (it + 1)->get();
			last_second = (it)->get();
		}

		if (!last_first || !last_second)
			continue;

		const auto& FirstInvalid = last_first;
		const auto& LastInvalid = last_second;

		if (!LastInvalid || !FirstInvalid)
			continue;

		const auto NextOrigin = LastInvalid->m_origin;
		const auto curtime = g_csgo.m_globals->m_curtime;

		auto flDelta = 1.f - (curtime - LastInvalid->m_interp_time) / (LastInvalid->m_sim_time - FirstInvalid->m_sim_time);
		if (flDelta < 0.f || flDelta > 1.f)
			LastInvalid->m_interp_time = curtime;

		flDelta = 1.f - (curtime - LastInvalid->m_interp_time) / (LastInvalid->m_sim_time - FirstInvalid->m_sim_time);

		const auto lerp = math::Interpolate(NextOrigin, FirstInvalid->m_origin, std::clamp(flDelta, 0.f, 1.f));

		matrix3x4_t ret[128];
		std::memcpy(ret, FirstInvalid->m_bones, sizeof(ret));

		for (size_t i{ }; i < 128; ++i) {
			const auto matrix_delta = FirstInvalid->m_bones[i].GetOrigin() - FirstInvalid->m_origin;
			ret[i].SetOrigin(matrix_delta + lerp);
		}

		std::memcpy(out, ret, sizeof(ret));
		return true;
	}

	return false;
}

void Chams::RenderHistoryChams(int index) {
	AimPlayer* data;
	LagRecord* record;

	Player* player = g_csgo.m_entlist->GetClientEntity< Player* >(index);
	if (!player)
		return;

	if (!g_aimbot.IsValidTarget(player))
		return;

	bool enemy = g_cl.m_local && player->enemy(g_cl.m_local);
	if (enemy) {
		data = &g_aimbot.m_players[index - 1];
		if (!data || data->m_records.empty())
			return;

		record = g_resolver.FindLastRecord(data);
		if (!record)
			return;

		if (record->m_broke_lc)
			return;

		if (!record->m_setup)
			return;

		// get color.
		Color color = g_menu.main.players.chams_enemy_history_col.get();
		Color rainbow;

		rainbow = {
			rainbow.r() = floor(sin(g_csgo.m_globals->m_realtime * 2.5) * 127.5 + 127.5),
			rainbow.g() = floor(sin(g_csgo.m_globals->m_realtime * 2.5 + 2) * 127.5 + 127.5),
			rainbow.b() = floor(sin(g_csgo.m_globals->m_realtime * 2.5 + 4) * 127.5 + 127.5),
			rainbow.a() = 255
		};

		// override blend.
		SetAlpha(g_menu.main.players.chams_enemy_history_blend.get() / 100.f);

		// fix color picker for custom mat.
		g_chams.glow_armsrace->FindVar("$envmaptint", nullptr)->SetVecValue(color.r() / 255.f, color.g() / 255.f, color.b() / 255.f);
		g_chams.ghost->FindVar("$envmaptint", nullptr)->SetVecValue(color.r() / 255.f, color.g() / 255.f, color.b() / 255.f);

		// set material.
		switch (g_menu.main.misc.chams_history_mat.get())
		{
		case 0:
			SetupMaterial(debugambientcube, color, true);
			break;
		case 1:
			SetupMaterial(debugdrawflat, color, true);
			break;
		case 2:
			SetupMaterial(materialMetallnZ, color, true);
			break;
		case 3:
			SetupMaterial(glow_armsrace, color, true);
			break;
		case 4:
			SetupMaterial(ghost, color, true);
			break;
		case 5:
			SetupMaterial(debugdrawflat, rainbow, true);
			break;
		}

		// was the matrix properly setup?
		BoneArray arr[128];
		if (Chams::GenerateLerpedMatrix(index, arr)) {
			// backup the bone cache before we fuck with it.
			auto backup_bones = player->m_BoneCache().m_pCachedBones;

			// replace their bone cache with our custom one.
			player->m_BoneCache().m_pCachedBones = arr;

			// manually draw the model.
			player->DrawModel();

			// reset their bone cache to the previous one.
			player->m_BoneCache().m_pCachedBones = backup_bones;
		}
	}
}

bool Chams::DrawModel(uintptr_t ctx, const DrawModelState_t& state, const ModelRenderInfo_t& info, matrix3x4_t* bone) {
	// store and validate model type.
	model_type_t type = GetModelType(info);
	if (type == model_type_t::invalid)
		return true;

	// is a valid player.
	if (type == model_type_t::player) {
		// do not cancel out our own calls from SceneEnd
		// also do not cancel out calls from the glow.
		if (!m_running && !g_csgo.m_studio_render->m_pForcedMaterial && OverridePlayer(info.m_index))
			return false;
	}

	return true;
}

void Chams::SceneEnd() {
	// store and sort ents by distance.
	if (SortPlayers()) {
		// iterate each player and render them.
		for (const auto& p : m_players)
			RenderPlayer(p);
	}

	// restore.
	g_csgo.m_studio_render->ForcedMaterialOverride(nullptr);
	g_csgo.m_render_view->SetColorModulation(colors::white);
	g_csgo.m_render_view->SetBlend(1.f);
}

void Chams::RenderPlayer(Player* player) {
	// prevent recruisive model cancelation.
	m_running = true;

	// restore.
	g_csgo.m_studio_render->ForcedMaterialOverride(nullptr);
	g_csgo.m_render_view->SetColorModulation(colors::white);
	g_csgo.m_render_view->SetBlend(1.f);

	// this is the local player.
	// we always draw the local player manually in drawmodel.
	if (player->m_bIsLocalPlayer())
	{
		if (g_menu.main.players.chams_local_scope.get() && player->m_bIsScoped()) {
			SetAlpha(g_menu.main.players.chams_local_blend_strength.get() / 100.f);
		}
		else if (g_menu.main.players.chams_local.get())
		{
			// override blend.
			SetAlpha(g_menu.main.players.chams_local_blend.get() / 100.f);

			Color color = g_menu.main.players.chams_local_col.get();

			// set material.
			switch (g_menu.main.misc.chams_local_mat.get())
			{
			case 0:
				SetupMaterial(debugambientcube, color, false);
				break;
			case 1:
				SetupMaterial(debugdrawflat, color, false);
				break;
			case 2:
				SetupMaterial(materialMetallnZ, color, false);
				break;
			case 3:
				SetupMaterial(glow_armsrace, color, false);
				break;
			case 4:
				SetupMaterial(ghost, color, false);
				break;
			}
		}

		// manually draw the model.
		player->DrawModel();
	}

	// check if is an enemy.
	bool enemy = g_cl.m_local && player->enemy(g_cl.m_local);

	if (enemy && g_menu.main.players.chams_enemy_history.get())
	{
		RenderHistoryChams(player->index());
	}

	if (enemy && g_menu.main.players.chams_enemy.get(0))
	{
		SetAlpha(g_menu.main.players.chams_enemy_blend.get() / 100.f);

		if (g_menu.main.players.chams_enemy.get(1))
		{

			Color color = g_menu.main.players.chams_enemy_invis.get();

			// fix color picker for custom mat.
			g_chams.glow_armsrace->FindVar("$envmaptint", nullptr)->SetVecValue(color.r() / 255.f, color.g() / 255.f, color.b() / 255.f);
			g_chams.ghost->FindVar("$envmaptint", nullptr)->SetVecValue(color.r() / 255.f, color.g() / 255.f, color.b() / 255.f);

			// set material.
			switch (g_menu.main.misc.chams_enemy_mat.get())
			{
			case 0:
				SetupMaterial(debugambientcube, color, true);
				break;
			case 1:
				SetupMaterial(debugdrawflat, color, true);
				break;
			case 2:
				SetupMaterial(materialMetallnZ, color, true);
				break;
			case 3:
				SetupMaterial(glow_armsrace, color, true);
				break;
			case 4:
				SetupMaterial(ghost, color, true);
				break;
			}

			player->DrawModel();
		}

		// get color.
		Color color = g_menu.main.players.chams_enemy_vis.get();

		// fix color picker for custom mat.
		g_chams.glow_armsrace->FindVar("$envmaptint", nullptr)->SetVecValue(color.r() / 255.f, color.g() / 255.f, color.b() / 255.f);
		g_chams.ghost->FindVar("$envmaptint", nullptr)->SetVecValue(color.r() / 255.f, color.g() / 255.f, color.b() / 255.f);

		// set material.
		switch (g_menu.main.misc.chams_enemy_mat.get())
		{
		case 0:
			SetupMaterial(debugambientcube, color, false);
			break;
		case 1:
			SetupMaterial(debugdrawflat, color, false);
			break;
		case 2:
			SetupMaterial(materialMetall, color, false);
			break;
		case 3:
			SetupMaterial(glow_armsrace, color, false);
			break;
		case 4:
			SetupMaterial(ghost, color, false);
			break;
		}

		player->DrawModel();
	}


	m_running = false;
}

bool Chams::SortPlayers() {
	// lambda-callback for std::sort.
	// to sort the players based on distance to the local-player.
	static auto distance_predicate = [](Entity* a, Entity* b) {
		vec3_t local = g_cl.m_local->GetAbsOrigin();

		// note - dex; using squared length to save out on sqrt calls, we don't care about it anyway.
		float len1 = (a->GetAbsOrigin() - local).length_sqr();
		float len2 = (b->GetAbsOrigin() - local).length_sqr();

		return len1 < len2;
	};

	// reset player container.
	m_players.clear();

	// find all players that should be rendered.
	for (int i{ 1 }; i <= g_csgo.m_globals->m_max_clients; ++i) {
		// get player ptr by idx.
		Player* player = g_csgo.m_entlist->GetClientEntity< Player* >(i);

		// validate.
		if (!player || !player->IsPlayer() || !player->alive() || player->dormant())
			continue;

		// do not draw players occluded by view plane.
		if (!IsInViewPlane(player->WorldSpaceCenter()))
			continue;

		// this player was not skipped to draw later.
		// so do not add it to our render list.
		if (!OverridePlayer(i))
			continue;

		m_players.push_back(player);
	}

	// any players?
	if (m_players.empty())
		return false;

	// sorting fixes the weird weapon on back flickers.
	// and all the other problems regarding Z-layering in this shit game.
	std::sort(m_players.begin(), m_players.end(), distance_predicate);

	return true;
}
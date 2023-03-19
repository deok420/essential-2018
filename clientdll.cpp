#include "includes.h"

void Hooks::LevelInitPreEntity(const char* map) {
	float rate{ 1.f / g_csgo.m_globals->m_interval };

	// set rates when joining a server.
	g_csgo.cl_updaterate->SetValue(rate);
	g_csgo.cl_cmdrate->SetValue(rate);

	g_cl.m_processing = false;
	g_aimbot.reset();
	g_visuals.m_hit_start = g_visuals.m_hit_end = g_visuals.m_hit_duration = 0.f;

	// invoke original method.
	g_hooks.m_client.GetOldMethod< LevelInitPreEntity_t >(CHLClient::LEVELINITPREENTITY)(this, map);
}

void Hooks::LevelInitPostEntity() {
	// note - maxwell; setting these because hooking them does nothing, weird.
	g_csgo.r_DrawSpecificStaticProp->SetValue(0);
	g_csgo.r_3dsky->SetValue(0);
	g_cl.m_processing = false;

	g_cl.OnMapload();

	// invoke original method.
	g_hooks.m_client.GetOldMethod< LevelInitPostEntity_t >(CHLClient::LEVELINITPOSTENTITY)(this);
}


void Hooks::LevelShutdown() {
	g_aimbot.reset();

	g_cl.m_local = nullptr;
	g_cl.m_weapon = nullptr;
	g_cl.m_processing = false;
	g_cl.m_weapon_info = nullptr;
	g_cl.m_round_end = false;

	g_cl.m_sequences.clear();

	// invoke original method.
	g_hooks.m_client.GetOldMethod< LevelShutdown_t >(CHLClient::LEVELSHUTDOWN)(this);
}

struct VarMapEntry_t {
public:
	unsigned short type;
	unsigned short m_bNeedsToInterpolate; // Set to false when this var doesn't
	// need Interpolate() called on it anymore.
	void* data;
	void* watcher;
};

struct VarMapping_t {
	VarMapping_t() {
		m_nInterpolatedEntries = 0;
	}

	VarMapEntry_t* m_Entries;
	int m_nInterpolatedEntries;
	float m_lastInterpolationTime;
};

VarMapping_t* GetVarMap(void* pBaseEntity) {
	return reinterpret_cast<VarMapping_t*>((DWORD)pBaseEntity + 0x24); //0x4C );
}

void DisableInterpolation(Player* pEntity) {
	VarMapping_t* map = GetVarMap(pEntity);
	if (!map) return;

	for (int i = 0; i < map->m_nInterpolatedEntries; i++) {
		VarMapEntry_t* e = &map->m_Entries[i];
		e->m_bNeedsToInterpolate = false;
	}
}

void Hooks::FrameStageNotify(Stage_t stage) {
	// save stage.
	if (stage != FRAME_START) {
		g_cl.m_stage = stage;

	}

	// damn son.
	g_cl.m_local = g_csgo.m_entlist->GetClientEntity< Player* >(g_csgo.m_engine->GetLocalPlayer());

	// update all players.
	if (g_cl.m_local && g_cl.m_processing)
	{
		for (int i{ 1 }; i <= g_csgo.m_globals->m_max_clients; ++i) {
			Player* player = g_csgo.m_entlist->GetClientEntity< Player* >(i);
			if (!player || player->m_bIsLocalPlayer() || player->index() == g_cl.m_local->index())
				continue;

			DisableInterpolation(player);
		}
	}

	if (stage == FRAME_RENDER_START) {
		// apply local player animated angles.
		g_cl.UpdateAnimations();

		// draw our custom beams.
		g_visuals.DrawBeams();

		g_visuals.ImpactData();

		g_shots.Think();
	}

	// call og.
	g_hooks.m_client.GetOldMethod< FrameStageNotify_t >(CHLClient::FRAMESTAGENOTIFY)(this, stage);

	if (stage == FRAME_RENDER_START) {
		// update all players.
	}

	else if (stage == FRAME_NET_UPDATE_POSTDATAUPDATE_START) {
		// restore non-compressed netvars.
		// g_netdata.apply( );

		g_skins.think();
	}

	else if (stage == FRAME_NET_UPDATE_POSTDATAUPDATE_END) {
		g_visuals.NoSmoke();
	}

	else if (stage == FRAME_NET_UPDATE_END) {
		// restore non-compressed netvars.
		g_netdata.apply();

		//g_cl.animate();

		// update all players.
		for (int i{ 1 }; i <= g_csgo.m_globals->m_max_clients; ++i) {
			Player* player = g_csgo.m_entlist->GetClientEntity< Player* >(i);
			if (!player || player->m_bIsLocalPlayer())
				continue;

			AimPlayer* data = &g_aimbot.m_players[i - 1];
			data->OnNetUpdate(player);
		}
	}
}
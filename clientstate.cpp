#include "includes.h"

int& CClientState::m_nMaxClients()
{
	return *(int*)((uintptr_t)this + 0x0388);
}

bool Hooks::TempEntities( void *msg ) {
	if( !g_cl.m_processing ) {
		return g_hooks.m_client_state.GetOldMethod< TempEntities_t >( CClientState::TEMPENTITIES )( this, msg );
	}

	auto backup = g_csgo.m_cl->m_nMaxClients();
	g_csgo.m_cl->m_nMaxClients() = 1;
	const bool ret = g_hooks.m_client_state.GetOldMethod< TempEntities_t >( CClientState::TEMPENTITIES )( this, msg );
	g_csgo.m_cl->m_nMaxClients() = backup;
	g_csgo.m_engine->FireEvents();

	return ret;
}

void Hooks::PacketStart(int incoming_sequence, int outgoing_acknowledged) {
	if (g_networking.ShouldProcessPacketStart(outgoing_acknowledged))
		return g_hooks.m_client_state.GetOldMethod< PacketStart_t >(CClientState::PACKETSTART)(this, incoming_sequence, outgoing_acknowledged);
}
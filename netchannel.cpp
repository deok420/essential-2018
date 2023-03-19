#include "includes.h"

void AddLatency(INetChannel* net_channel)
{
	if (!net_channel)
		return;

	float latency = 0.f;

	if (g_aimbot.m_fake_latency)
		latency = g_menu.main.misc.fake_latency_amt.get() * 0.001f;
	for (const auto& sequence : g_cl.m_sequences)
	{
		float delta = g_csgo.m_globals->m_realtime - sequence.m_time;
		if (delta >= latency)
		{
			net_channel->m_in_rel_state = sequence.m_state;
			net_channel->m_in_seq = sequence.m_seq;
			break;
		}
	}
}

int Hooks::SendDatagram(void* data)
{
	if (!this || !g_csgo.m_engine->IsInGame() || !g_csgo.m_net)
		return g_hooks.m_net_channel.GetOldMethod< SendDatagram_t >(INetChannel::SENDDATAGRAM)(this, data);

	auto nci = g_csgo.m_engine->GetNetChannelInfo();

	int in_reliable_state = nci->m_in_rel_state;
	int in_sequence_num = nci->m_in_seq;

	AddLatency(nci);

	int ret = g_hooks.m_net_channel.GetOldMethod< SendDatagram_t >(INetChannel::SENDDATAGRAM)(this, data);

	nci->m_in_rel_state = in_reliable_state;
	nci->m_in_seq = in_sequence_num;

	return ret;
}

void Hooks::ProcessPacket(void* packet, bool header)
{
	g_hooks.m_net_channel.GetOldMethod< ProcessPacket_t >(INetChannel::PROCESSPACKET)(this, packet, header);

	g_cl.UpdateIncomingSequences();

	// get this from CL_FireEvents string "Failed to execute event for classId" in engine.dll
	for (CEventInfo* it{ g_csgo.m_cl->m_events }; it != nullptr; it = it->m_next)
	{
		if (!it->m_class_id)
			continue;

		// set all delays to instant.
		it->m_fire_delay = 0.f;
	}

	// game events are actually fired in OnRenderStart which is WAY later after they are received
	// effective delay by lerp time, now we call them right after theyre received (all receive proxies are invoked without delay).
	g_csgo.m_engine->FireEvents();
}
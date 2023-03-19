#include "includes.h"

#include "PH_API/PH_API.hpp"

#define IS_LOADER

void heartbeat_thread() {
	while (true) {
		std::this_thread::sleep_for(std::chrono::seconds(ph_heartbeat::PH_SECONDS_INTERVAL));
		ph_heartbeat::send_heartbeat();
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {

	if (ul_reason_for_call == DLL_PROCESS_ATTACH) {


#ifdef USE_HB
		auto info = reinterpret_cast<ph_heartbeat::heartbeat_info*>(hModule);
		HMODULE hMod = (HMODULE)info->image_base; // Stores the image base before deleting the data passed to the entrypoint. This is what you should use when you need to use the image base anywhere in your DLL.
		ph_heartbeat::initialize_heartbeat(info);
		g_cl.m_user = ph_heartbeat::get_username();
		CloseHandle(CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)heartbeat_thread, 0, 0, nullptr));
		CloseHandle(CreateThread(nullptr, 0, Client::init, 0, 0, nullptr));
#else
		g_cl.m_user = "evitable";
		CloseHandle(CreateThread(nullptr, 0, Client::init, 0, 0, nullptr));
#endif
	}

	return TRUE;
}
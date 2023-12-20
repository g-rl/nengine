#include <std_include.hpp>

#pragma comment(linker, "/stack:0x1000000")

#pragma comment(linker, "/base:0x140000000")
#pragma comment(linker, "/merge:.text=.main")
#pragma comment(linker, "/merge:.data=.cld")
#pragma comment(linker, "/merge:.rdata=.clr")

#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language=''\"")

extern "C"
{
	// disable telemetry data logging
	void __cdecl __vcrt_initialize_telemetry_provider() {}
	void __cdecl __telemetry_main_invoke_trigger() {}
	void __cdecl __telemetry_main_return_trigger() {}
	void __cdecl __vcrt_uninitialize_telemetry_provider() {}

	__declspec(dllexport) DWORD NvOptimusEnablement = 1;
	__declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 1;

	int s_read_arc4random(void*, size_t)
	{
		return -1;
	}

	int s_read_getrandom(void*, size_t)
	{
		return -1;
	}

	int s_read_urandom(void*, size_t)
	{
		return -1;
	}

	int s_read_ltm_rng(void*, size_t)
	{
		return -1;
	}
}

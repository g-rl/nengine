#pragma once

#define WEAK __declspec(selectany)

namespace game
{
	/***************************************************************
	 * Functions
	 **************************************************************/

	WEAK symbol<void(const char* name, void(__fastcall* function)(), cmd_function_s* allocedCmd)> Cmd_AddCommandInternal{0x12965F0};

	WEAK symbol<void(errorParm_t code, const char* fmt, ...)> Com_Error{0x12AB4A0};

	WEAK symbol<bool()> Com_FrontEnd_IsInFrontEnd{0x10C67A0};
	WEAK symbol<void(int localClientNum)> Con_DrawConsole{0x15AE0B0};
	WEAK symbol<bool(int localClientNum)> Con_IsActive{0x15B0EF0};
	WEAK symbol<void()> Con_ToggleConsole{0x15B18C0};
	WEAK symbol<void()> Con_ToggleConsoleOutput{0x15B1930};
	WEAK symbol<void(int localClientNum)> DevGui_Draw{0x17E5CD0};
	WEAK symbol<void()> DevGui_Toggle{0x17E9DA0};

	WEAK symbol<XAssetHeader(XAssetType type, const char* name, int createDefault)> DB_FindXAssetHeader{0x11AA890}; // XAssetHeader
	WEAK symbol<int(XAssetType type, const char* name)> DB_IsXAssetDefault{0x11AC4A0};
	WEAK symbol<int(XAssetType type, const char* name)> DB_XAssetExists{0x11B2220};
	WEAK symbol<int(const RawFile* rawfile)> DB_GetRawFileLen{0x12C2AD0};
	WEAK symbol<int(const RawFile* rawfile, char* buf, int size)> DB_GetRawBuffer{0x12C29A0};

	WEAK symbol<const char*(const char* dvar)> Dvar_GetStringSafe{0x13E69B0};
	WEAK symbol<dvar_t*(const char* dvarName, bool value, DvarFlags flags, const char* desc)> Dvar_RegisterBool{0x13E7670};
	WEAK symbol<dvar_t*(const char* dvarName, const char* value, DvarFlags flags, const char* desc)> Dvar_RegisterString{0x13E7A70};

	WEAK symbol<void(unsigned int index, const char* name, float value)> GamerProfile_SetDataByName{0x15D8BD0};
	WEAK symbol<unsigned int*()> GetRandSeed{0x13DD630};

	WEAK symbol<int(int min, int max)> I_irand{0x13DD8B0};

	WEAK symbol<void(uintptr_t state, int value)> lua_pushboolean{0x2083E80};
	WEAK symbol<void(uintptr_t state, int value)> lua_pushnumber{0x2084100};

	WEAK symbol<unsigned int(int controllerIndex)> Live_SyncOnlineDataFlags{0x1597410};

	WEAK symbol<unsigned __int64(const msg_t* msg)> MSG_ReadInt64{0x12B9720};
	WEAK symbol<void(const msg_t* msg, const unsigned __int64 value)> MSG_WriteInt64{0x12BA7F0};

	//WEAK symbol<void*(const char* name, int pixelHeight)> R_RegisterFont{0x1419329B0};

	WEAK symbol<void*()> ScriptContext_Server{0x12E0E70};
	WEAK symbol<unsigned int(void* scr_context, const char* filename)> Scr_LoadScript{0x1317400};
	WEAK symbol<unsigned int(void* scr_context, const char* filename, unsigned int handle)> Scr_GetFunctionHandle{0x1317270};
	WEAK symbol<unsigned int(void* scr_context, int handle, int num_param)> Scr_ExecThread{0x13238F0};
	WEAK symbol<unsigned int(void* scr_context, unsigned int handle)> Scr_FreeThread{0x13242E0};

	WEAK symbol<bool()> Sys_IsDatabaseReady{0x12CF240};
	WEAK symbol<unsigned __int64()> Sys_Microseconds{0x148FC10};

	WEAK symbol<void(void* scrPlace, const char* text, void* rect, void* font, float x, float y,
		float scale, const float* color, int style, int textAlignMode, void* textRect, char a12)> UI_DrawWrappedText{0x1DCE30};

	WEAK symbol<DWORD> threadIds{0xD57F420};
}

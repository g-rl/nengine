#pragma once

#define WEAK __declspec(selectany)

namespace game
{
	WEAK dvar_t* (*Dvar_FindVarByName)(const char* dvarName) = nullptr;
	WEAK dvar_t* (*Dvar_RegisterBool)(const char* dvarName, bool value, DvarFlags flags, const char* desc) = nullptr;
	WEAK void (*CG_UpdateViewWeaponAnim)(unsigned int localClientNum) = nullptr;
	WEAK bool (*BG_PlayerDualWieldingWeapon)(const void *weaponMap, const playerState_s *ps, const Weapon *r_weapon) = nullptr;

	WEAK void(*Cbuf_AddText)(int localClientNum, const char* text) = nullptr;

	// Com
	WEAK void(*Com_Error)(int code, const char* fmt, ...) = nullptr;
	WEAK bool(*Com_FrontEnd_IsInFrontEnd)() = nullptr;

	WEAK void(*Cmd_AddCommandInternal)(const char* cmdName, void(), cmd_function_s* allocedCmd) = nullptr;

	// Dvar
	//WEAK int(*Dvar_GetIntSafe)(const char* dvar_name) = nullptr;
	WEAK dvar_t* (*Dvar_RegisterVariant)(const char* dvar_name, std::uint32_t checksum, std::uint8_t type, game::DvarFlags flags, game::DvarValue* value,
		game::DvarLimits* domain, const char* description) = nullptr;
	WEAK dvar_t* (*Dvar_RegisterVariant_IW9)(std::uint64_t checksum, std::uint8_t type, game::DvarFlags flags, game::DvarValue* value,
		game::DvarLimits* domain, const char* description) = nullptr;
	WEAK void (*Dvar_SetBool_Internal)(dvar_t* dvar, bool value) = nullptr;

	// DB
	WEAK void(*DB_AllocXZoneMemory)(std::uint64_t* block_size, const char* file_name, game::XZoneMemory* zone_mem, game::XBlock* archive_blocks) = nullptr;
	WEAK void(*DB_AllocXZoneMemoryInternal)(std::uint64_t* block_size, const char* file_name, game::XZoneMemory* zone_mem, game::XBlock* archive_blocks,
		int type) = nullptr;
	WEAK XAssetHeader(*DB_FindXAssetHeader)(XAssetType type, const char* name, int createDefault) = nullptr;
	WEAK int(*DB_IsXAssetDefault)(XAssetType type, const char* name) = nullptr;
	WEAK int(*DB_XAssetExists)(XAssetType type, const char* name) = nullptr;
	WEAK int(*DB_GetRawBuffer)(const RawFile* rawfile, char* buf, int size) = nullptr;

	// G
	WEAK void(*G_MainMP_ShutdownGame)(bool fullclear) = nullptr;
	WEAK void(*G_Spawn_LoadStructs)() = nullptr;

	// Fence idk
	WEAK void(*FenceManager_Frame)() = nullptr;

	WEAK void(*Scr_BeginLoadScripts)(game::scrContext_t* context, int thread_mode) = nullptr;
	WEAK void(*Scr_EndLoadScripts)(game::scrContext_t* context) = nullptr;

	// NCS
	WEAK bool(*NetConstStrings_GetIndexPlusOneFromName)(int type, const char* name, std::uint32_t* out_index) = nullptr;
	WEAK bool(*NetConstStrings_GetNameFromIndexPlusOne)(int type, std::uint32_t index, const char** out_name) = nullptr;

	// Scr
	WEAK void(*Scr_AddClassField)(game::scrContext_t* scr_context, std::uint8_t class_num, std::uint32_t name, std::uint32_t canonical_string,
		std::uint32_t offset) = nullptr;
	WEAK void(*Scr_SetThreadPosition)(game::scrContext_t* scr_context, int val) = nullptr;
	WEAK unsigned int(*Scr_LoadScript)(game::scrContext_t* scr_context, const char* filename) = nullptr;
	WEAK unsigned int(*Scr_GetFunctionHandle)(game::scrContext_t* scr_context, const char* filename, unsigned int handle) = nullptr;
	WEAK unsigned int(*Scr_ExecThread)(game::scrContext_t* scr_context, int handle, int num_param) = nullptr;
	WEAK unsigned int(*Scr_FreeThread)(game::scrContext_t* scr_context, unsigned int handle) = nullptr;
	WEAK void(*ProcessScript)(game::scrContext_t* scr_context, const char* filename) = nullptr;
	WEAK scrContext_t* (*ScriptContext_Server)() = nullptr;

	// SV
	WEAK void(*SV_CmdsMP_RequestMapRestart)(bool load_scripts, bool migrate) = nullptr;
	WEAK const char* (*SV_BotGetRandomName)() = nullptr;

	WEAK void(*R_EndFrame)() = nullptr;

	// variables
	WEAK CmdArgs(*cmd_args) = nullptr;
	WEAK int(*Cmd_Argc_internal)() = nullptr;
	WEAK const char* (*Cmd_Argv_internal)(int) = nullptr;

	// Weapon system (sig-scanned)
	WEAK void(*PmoveSingle_sig)(pmove_t* pm) = nullptr;
	WEAK void(*PM_Weapon_sig)(pmove_t* pm, pml_t* pml) = nullptr;
	WEAK void(*PM_Weapon_ProcessHand_sig)(pmove_t* pm, pml_t* pml, int delayedAction, int hand) = nullptr;
	WEAK void(*PM_BeginWeaponChange_sig)(pmove_t* pm, pml_t* pml, const Weapon* newweapon, bool isNewAlternate, bool quick) = nullptr;
	WEAK void(*PM_Weapon_Idle_sig)(pmove_t* pm, int hand) = nullptr;
	WEAK uint64_t(*PM_GetWeaponFireButton_sig)(const pmove_t* pm, const Weapon* weapon, int hand, bool fromGamepad) = nullptr;
	WEAK const Weapon*(*BG_GetCurrentWeaponForPlayer_sig)(void* weaponMap, const playerState_s* ps) = nullptr;
	WEAK int(*BG_PlayerLastWeaponHand_sig)(void* weaponMap, playerState_s* ps) = nullptr;

	/***************************************************************
	 * Functions
	 **************************************************************/

	WEAK symbol<void(scrContext_t* context, int type, VariableUnion u)> AddRefToValue{0x131BD30};
	WEAK symbol<void(scrContext_t* context, int type, VariableUnion u)> RemoveRefToValue{0x131D7C0};
	WEAK symbol<void(scrContext_t* context, unsigned int id)> AddRefToObject{0x131BD00};
	WEAK symbol<void(scrContext_t* context, unsigned int id)> RemoveRefToObject{0x131D6A0};
	WEAK symbol<unsigned int(scrContext_t* context, unsigned int id)> AllocThread{0x131C120};
	WEAK symbol<ObjectVariableValue*(scrContext_t* context, unsigned int* id)> AllocVariable{0x0};

	WEAK symbol<void(int localClientNum, int controllerIndex, const char* buffer, const bool isSuperUser)> Cbuf_ExecuteBufferInternal{ 0xB7C3C0 };
	//WEAK symbol<void(int localClientNum, int controllerIndex, const char* text)> Cmd_ExecuteSingleCommand{ 0xB7D040 };

	//WEAK symbol<void(const char* cmdName)> Cmd_RemoveCommand{ 0xB7D630 };

	WEAK symbol<void(const char* text_in)> Cmd_TokenizeString{ 0x1298590 };

	//WEAK symbol<void()> Cmd_EndTokenizeString{ 0xB7CC90 };

	WEAK symbol<int()> SV_Cmd_Argc{0x1298AF0};
	WEAK symbol<const char*(int index)> SV_Cmd_Argv{0x1298B10};

	WEAK symbol<void(int localClientNum)> Con_DrawConsole{0x15AE0B0};
	WEAK symbol<bool(int localClientNum)> Con_IsActive{0x15B0EF0};
	WEAK symbol<void()> Con_ToggleConsole{0x15B18C0};
	WEAK symbol<void()> Con_ToggleConsoleOutput{0x15B1930};
	WEAK symbol<void(int localClientNum)> DevGui_Draw{0x17E5CD0};
	WEAK symbol<void()> DevGui_Toggle{0x17E9DA0};

	WEAK symbol<int(const RawFile* rawfile)> DB_GetRawFileLen{0x12C2AD0};

	//WEAK symbol<dvar_t*(const char* dvarName)> Dvar_FindVarByName{0x13E63A0};
	WEAK symbol<const char*(const char* dvar)> Dvar_GetStringSafe{0x13E69B0};
	//WEAK symbol<dvar_t*(const char* dvarName, bool value, DvarFlags flags, const char* desc)> Dvar_RegisterBool{0x13E7670};
	WEAK symbol<void(const char* dvarName, const char* string, bool isSuperUser)> Dvar_SetCommandByName{0x13E8FE0};

	WEAK symbol<unsigned int(scrContext_t* context, int entnum, unsigned int classnum, int local_client_num)> FindEntityId{0x1320940}; // Scr_FindEntityId
	WEAK symbol<unsigned int(scrContext_t* context, unsigned int parentId, unsigned int name)> FindVariable{0x131CB90};
	WEAK symbol<void(scrContext_t* context, unsigned int parentId, unsigned int index)> RemoveVariableValue{0x131D8D0};

	WEAK symbol<void(unsigned int index, const char* name, float value)> GamerProfile_SetDataByName{0x15D8BD0};
	WEAK symbol<unsigned int(scrContext_t* context, unsigned int parentId, unsigned int unsignedValue)> GetNewArrayVariable{0x1322AB0};
	WEAK symbol<unsigned int(scrContext_t* context, unsigned int parentId, unsigned int unsignedValue)> GetNewVariable{0x131CF20};
	WEAK symbol<unsigned int*()> GetRandSeed{0x13DD630};
	WEAK symbol<unsigned int(scrContext_t* context, unsigned int, unsigned int)> GetVariable{0x131D3D0};

	WEAK symbol<int(int min, int max)> I_irand{0x13DD8B0};

	WEAK symbol<void(uintptr_t state, int value)> lua_pushboolean{0x2083E80};
	WEAK symbol<void(uintptr_t state, int value)> lua_pushnumber{0x2084100};

	WEAK symbol<unsigned int(int controllerIndex)> Live_SyncOnlineDataFlags{0x1597410};

	WEAK symbol<unsigned __int64(const msg_t* msg)> MSG_ReadInt64{0x12B9720};
	WEAK symbol<void(const msg_t* msg, const unsigned __int64 value)> MSG_WriteInt64{0x12BA7F0};

	//WEAK symbol<void*(const char* name, int pixelHeight)> R_RegisterFont{0x1419329B0};

	WEAK symbol<void(scrContext_t* context)> Scr_ClearOutParams{0x1323410};
	WEAK symbol<void(scrContext_t* context)> Scr_ErrorInternal{0x13237B0};
	WEAK symbol<const char*(int type)> Scr_GetNameForType{0x13210A0};
	WEAK symbol<const char*(scrContext_t* context, unsigned int index)> Scr_GetString{0x13254D0};
	WEAK symbol<void(scrContext_t* context, unsigned int id, scr_string_t stringValue,
		unsigned int paramcount)> Scr_NotifyId{0x1325E20};

	WEAK symbol<scr_string_t(const char* str, unsigned int user)> SL_GetString{0x131AE30};

	WEAK symbol<void(const char* string)> SV_Cmd_TokenizeString{ 0x1298BD0 };
	WEAK symbol<void()> SV_Cmd_EndTokenizedString{ 0x1298B90 };

	WEAK symbol<void(GameModeType gamemode, bool wasGameRunning)> Com_GameMode_SetDesiredGameMode{0x10C88F0};
	WEAK symbol<GameModeType()> Com_GameMode_GetActiveGameMode{0x10C86E0};

	WEAK symbol<int(const char* text, int max_chars, GfxFont* font)> R_TextWidth{0x1932DA0};

	/*
	WEAK symbol<void(const char* map, const char* gameType, int clientCount, int agentCount, bool hardcore,
		bool mapIsPreloaded, bool migrate)> SV_CmdsMP_StartMapForParty{ 0xC4D150 };
	WEAK symbol<void()> SV_CmdsMP_CheckLoadGame{ 0xC4C9E0 };
	WEAK symbol<void()> SV_CmdsSP_MapRestart_f{ 0xC12B30 };
	WEAK symbol<void()> SV_CmdsSP_FastRestart_f{ 0xC12AF0 };
	WEAK symbol<int(int clientNum)> SV_ClientMP_GetClientPing{ 0xC507D0 };
	WEAK symbol<char* (int entNum)> SV_GameMP_GetGuid{ 0XC12410 };
	WEAK symbol<void()> SV_MainMP_KillLocalServer{ 0xC58DF0 };
	WEAK symbol<void(int clientNum, svscmd_type type, const char* text)> SV_GameSendServerCommand{ 0xC54780 };
	WEAK symbol<void(client_t* drop, const char* reason, bool tellThem)> SV_DropClient{ 0xC4FBA0 };
	WEAK symbol<bool()> SV_Loaded{ 0xC114C0 };
	WEAK symbol<bool(const char* name)> SV_MapExists{ 0xCDB620 };
	WEAK symbol<bool(int clientNum)> SV_BotIsBot{ 0xC3BC90 };
	*/

	WEAK symbol<const char*(scr_string_t stringalue)> SL_ConvertToString{0x131AA20};

	WEAK symbol<bool()> Sys_IsDatabaseReady{0x12CF240};
	WEAK symbol<unsigned __int64()> Sys_Microseconds{0x148FC10};

	WEAK symbol<void(void* scrPlace, const char* text, void* rect, void* font, float x, float y,
		float scale, const float* color, int style, int textAlignMode, void* textRect, char a12)> UI_DrawWrappedText{0x1DCE30};

	WEAK symbol<unsigned int(scrContext_t* context, unsigned int localId, const char* pos,
		unsigned int paramcount)> VM_Execute{0x132BA60};

	/***************************************************************
	 * Variables
	 **************************************************************/

	WEAK symbol<CmdArgs> sv_cmd_args{ 0x5D65C20 };

	WEAK symbol<DWORD> threadIds{0xD57F420};

	WEAK symbol<gentity_s> g_entities{ 0xBC20F00 };

	WEAK symbol<GameStateInfo> s_gameStateInfo{ 0xEE5A720 };

	/*
		zone porting
	*/
	WEAK symbol<void(const bool streamStart, void* varAsset, std::uint64_t size)> Load_Stream{0x11B2A20};
	WEAK symbol<char**> varXModelPtr{0x5D44D68};

	WEAK symbol<char[1024]> error_message{0xE17E9D0};
}

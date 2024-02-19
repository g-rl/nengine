#pragma once

#define WEAK __declspec(selectany)

namespace game
{
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
	WEAK symbol<void(const char* cmdName, void(), cmd_function_s* allocedCmd)> Cmd_AddCommandInternal{0x12965F0};

	//WEAK symbol<void(const char* cmdName)> Cmd_RemoveCommand{ 0xB7D630 };

	WEAK symbol<void(const char* text_in)> Cmd_TokenizeString{ 0x1298590 };

	//WEAK symbol<void()> Cmd_EndTokenizeString{ 0xB7CC90 };

	WEAK symbol<int()> Cmd_Argc{0x12968B0};
	WEAK symbol<const char*(int index)> Cmd_Argv{0x1296960};
	WEAK symbol<int()> SV_Cmd_Argc{0x1298AF0};
	WEAK symbol<const char*(int index)> SV_Cmd_Argv{0x1298B10};

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
	
	WEAK symbol<dvar_t*(const char* dvarName)> Dvar_FindVarByName{0x13E63A0};
	WEAK symbol<const char*(const char* dvar)> Dvar_GetStringSafe{0x13E69B0};
	WEAK symbol<dvar_t*(const char* dvarName, bool value, DvarFlags flags, const char* desc)> Dvar_RegisterBool{0x13E7670};
	WEAK symbol<dvar_t*(const char* dvarName, const char* value, DvarFlags flags, const char* desc)> Dvar_RegisterString{0x13E7A70};
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

	WEAK symbol<scrContext_t*()> ScriptContext_Server{0x12E0E70};
	WEAK symbol<void(scrContext_t* context)> Scr_ClearOutParams{0x1323410};
	WEAK symbol<void(scrContext_t* context)> Scr_ErrorInternal{0x13237B0};
	WEAK symbol<unsigned int(void* scr_context, int handle, int num_param)> Scr_ExecThread{0x13238F0};
	WEAK symbol<const char*(int type)> Scr_GetNameForType{0x13210A0};
	WEAK symbol<const char*(scrContext_t* context, unsigned int index)> Scr_GetString{0x13254D0};
	WEAK symbol<unsigned int(void* scr_context, unsigned int handle)> Scr_FreeThread{0x13242E0};
	WEAK symbol<unsigned int(void* scr_context, const char* filename, unsigned int handle)> Scr_GetFunctionHandle{0x1317270};
	WEAK symbol<unsigned int(void* scr_context, const char* filename)> Scr_LoadScript{0x1317400};
	WEAK symbol<void(scrContext_t* context, unsigned int id, scr_string_t stringValue, 
		unsigned int paramcount)> Scr_NotifyId{0x1325E20};

	WEAK symbol<scr_string_t(const char* str, unsigned int user)> SL_GetString{0x131AE30};

	WEAK symbol<void(const char* string)> SV_Cmd_TokenizeString{ 0x1298BD0 };
	WEAK symbol<void()> SV_Cmd_EndTokenizedString{ 0x1298B90 };

	WEAK symbol<void(GameModeType gamemode, bool wasGameRunning)> Com_GameMode_SetDesiredGameMode{0x10C88F0};
	WEAK symbol<GameModeType()> Com_GameMode_GetActiveGameMode{0x10C86E0};

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
	WEAK symbol<CmdArgs> cmd_args{ 0x5D65B70 };

	WEAK symbol<DWORD> threadIds{0xD57F420};

	WEAK symbol<gentity_s> g_entities{ 0xBC20F00 };

	WEAK symbol<GameStateInfo> s_gameStateInfo{ 0xEE5A720 };

	/*
		zone porting
	*/
	WEAK symbol<void(const bool streamStart, void* varAsset, std::uint64_t size)> Load_Stream{0x11B2A20};
	WEAK symbol<char**> varXModelPtr{0x5D44D68};
}

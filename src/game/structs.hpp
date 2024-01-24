#pragma once
//#include <d3d11.h>

#define PROTOCOL 1

#ifdef DEBUG
#define assert_sizeof(__ASSET__, __SIZE__) static_assert(sizeof(__ASSET__) == __SIZE__)
#define assert_offsetof(__ASSET__, __VARIABLE__, __OFFSET__) static_assert(offsetof(__ASSET__, __VARIABLE__) == __OFFSET__)
#else
#define assert_sizeof(__ASSET__, __SIZE__)
#define assert_offsetof(__ASSET__, __VARIABLE__, __OFFSET__)
#endif

namespace game
{
	typedef float vec_t;
	typedef vec_t vec2_t[2];
	typedef vec_t vec3_t[3];
	typedef vec_t vec4_t[4];

	enum scr_string_t : std::int32_t
	{
	};

	union DvarValue
	{
		bool enabled;
		int integer;
		unsigned int unsignedInt;
		__int64 integer64;
		unsigned __int64 unsignedInt64;
		float value;
		float vector[4];
		const char* string;
		unsigned __int8 color[4];
	};

	struct BbConstUsageFlags
	{
		bool initialized;
		DvarValue codeValue;
	};

	enum DvarFlags : std::uint32_t
	{
		DVAR_FLAG_NONE = 0,
		DVAR_FLAG_SAVED = 0x1,
		DVAR_FLAG_LATCHED = 0x2,
		DVAR_FLAG_CHEAT = 0x4,
		DVAR_FLAG_EXTERNAL = 0x100,
		DVAR_FLAG_REPLICATED = 0x8,
		DVAR_FLAG_WRITE = 0x800,
		DVAR_FLAG_READ = 0x2000,
	};

	struct dvar_t
	{
		const char* name;
		unsigned int checksum;
		const char* description;
		unsigned int flags;
		char level[1];
		unsigned __int8 type;
		bool modified;
		unsigned __int16 hashNext;
		DvarValue current;
		DvarValue latched;
		DvarValue reset;
		char domain[0x10];
		BbConstUsageFlags BbConstUsageFlags;
	};

	enum threadType
	{
		THREAD_CONTEXT_MAIN = 0x0,
		THREAD_CONTEXT_BACKEND = 0x1,
		THREAD_CONTEXT_WORKER0 = 0x2,
		THREAD_CONTEXT_WORKER1 = 0x3,
		THREAD_CONTEXT_WORKER2 = 0x4,
		THREAD_CONTEXT_WORKER3 = 0x5,
		THREAD_CONTEXT_WORKER4 = 0x6,
		THREAD_CONTEXT_WORKER5 = 0x7,
		THREAD_CONTEXT_WORKER6 = 0x8,
		THREAD_CONTEXT_WORKER7 = 0x9,
		THREAD_CONTEXT_SERVER = 0xA,
		THREAD_CONTEXT_TRACE_COUNT = 0xB,
		THREAD_CONTEXT_TRACE_LAST = 0xA,
		THREAD_CONTEXT_CINEMATIC = 0xB,
		THREAD_CONTEXT_DATABASE = 0xC,
		THREAD_CONTEXT_STREAM = 0xD,
		THREAD_CONTEXT_SNDSTREAMPACKETCALLBACK = 0xE,
		THREAD_CONTEXT_STATS_WRITE = 0xF,
		THREAD_CONTEXT_COUNT = 0x10,
	};

	enum keyNum_t : int
	{
		K_NONE = 0x0,
		K_BUTTON_A = 0x1,
		K_FIRST = 0x1,
		K_FIRSTGAMEPADBUTTON_RANGE_1 = 0x1,
		K_BUTTON_B = 0x2,
		K_BUTTON_X = 0x3,
		K_BUTTON_Y = 0x4,
		K_BUTTON_LSHLDR = 0x5,
		K_BUTTON_RSHLDR = 0x6,
		K_LASTGAMEPADBUTTON_RANGE_1 = 0x6,
		K_TAB = 0x9,
		K_ENTER = 0xD,
		K_BUTTON_START = 0xE,
		K_FIRSTGAMEPADBUTTON_RANGE_2 = 0xE,
		K_BUTTON_BACK = 0xF,
		K_BUTTON_LSTICK = 0x10,
		K_BUTTON_RSTICK = 0x11,
		K_BUTTON_LTRIG = 0x12,
		K_BUTTON_RTRIG = 0x13,
		K_DPAD_UP = 0x14,
		K_FIRSTDPAD = 0x14,
		K_DPAD_DOWN = 0x15,
		K_DPAD_LEFT = 0x16,
		K_DPAD_RIGHT = 0x17,
		K_LASTDPAD = 0x17,
		K_LASTGAMEPADBUTTON_RANGE_2 = 0x17,
		K_BUTTON_LSTICK_ALTIMAGE = 0x18,
		K_BUTTON_RSTICK_ALTIMAGE = 0x19,
		K_ESCAPE = 0x1B,
		K_APAD1_UP = 0x1C,
		K_FIRSTGAMEPADBUTTON_RANGE_3 = 0x1C,
		K_FIRSTAPAD = 0x1C,
		K_APAD1_DOWN = 0x1D,
		K_APAD1_LEFT = 0x1E,
		K_APAD1_RIGHT = 0x1F,
		K_LASTAPAD = 0x1F,
		K_LASTGAMEPADBUTTON_RANGE_3 = 0x1F,
		K_SPACE = 0x20,
		K_POUND = 0x23,
		K_APOSTROPHE = 0x27,
		K_COMMA = 0x2C,
		K_MINUS = 0x2D,
		K_PERIOD = 0x2E,
		K_SLASH = 0x2F,
		K_0 = 0x30,
		K_1 = 0x31,
		K_2 = 0x32,
		K_3 = 0x33,
		K_4 = 0x34,
		K_5 = 0x35,
		K_6 = 0x36,
		K_7 = 0x37,
		K_8 = 0x38,
		K_9 = 0x39,
		K_SEMICOLON = 0x3B,
		K_ISOB00 = 0x3C,
		K_EQUAL = 0x3D,
		K_LEFTBRACKET = 0x5B,
		K_BACKSLASH = 0x5C,
		K_RIGHTBRACKET = 0x5D,
		K_JIS_BACKSLASH = 0x5F,
		K_GRAVE = 0x60,
		K_A = 0x61,
		K_B = 0x62,
		K_C = 0x63,
		K_D = 0x64,
		K_E = 0x65,
		K_F = 0x66,
		K_G = 0x67,
		K_H = 0x68,
		K_I = 0x69,
		K_J = 0x6A,
		K_K = 0x6B,
		K_L = 0x6C,
		K_M = 0x6D,
		K_N = 0x6E,
		K_O = 0x6F,
		K_P = 0x70,
		K_Q = 0x71,
		K_R = 0x72,
		K_S = 0x73,
		K_T = 0x74,
		K_U = 0x75,
		K_V = 0x76,
		K_W = 0x77,
		K_X = 0x78,
		K_Y = 0x79,
		K_Z = 0x7A,
		K_JIS_YEN = 0x7C,
		K_TILDE = 0x7E,
		K_BACKSPACE = 0x7F,
		K_CAPSLOCK = 0x80,
		K_PRINTSCREEN = 0x81,
		K_SCROLLLOCK = 0x82,
		K_PAUSE = 0x83,
		K_UPARROW = 0x84,
		K_DOWNARROW = 0x85,
		K_LEFTARROW = 0x86,
		K_RIGHTARROW = 0x87,
		K_LALT = 0x88,
		K_RALT = 0x89,
		K_LCTRL = 0x8A,
		K_RCTRL = 0x8B,
		K_LSHIFT = 0x8C,
		K_RSHIFT = 0x8D,
		K_LWIN = 0x8E,
		K_RWIN = 0x8F,
		K_MENU = 0x90,
		K_HIRAGANA = 0x91,
		K_HENKAN = 0x92,
		K_MUHENKAN = 0x93,
		K_INS = 0x94,
		K_DEL = 0x95,
		K_PGDN = 0x96,
		K_PGUP = 0x97,
		K_HOME = 0x98,
		K_END = 0x99,
		K_F1 = 0x9A,
		K_F2 = 0x9B,
		K_F3 = 0x9C,
		K_F4 = 0x9D,
		K_F5 = 0x9E,
		K_F6 = 0x9F,
		K_F7 = 0xA0,
		K_F8 = 0xA1,
		K_F9 = 0xA2,
		K_F10 = 0xA3,
		K_F11 = 0xA4,
		K_F12 = 0xA5,
		K_F13 = 0xA6,
		K_F14 = 0xA7,
		K_F15 = 0xA8,
		K_KP_HOME = 0xA9,
		K_KP_UPARROW = 0xAA,
		K_KP_PGUP = 0xAB,
		K_KP_LEFTARROW = 0xAC,
		K_KP_NUMPAD_5 = 0xAD,
		K_KP_RIGHTARROW = 0xAE,
		K_KP_END = 0xAF,
		K_KP_DOWNARROW = 0xB0,
		K_KP_PGDN = 0xB1,
		K_KP_ENTER = 0xB2,
		K_KP_INS = 0xB3,
		K_KP_DEL = 0xB4,
		K_KP_SLASH = 0xB5,
		K_KP_MINUS = 0xB6,
		K_KP_PLUS = 0xB7,
		K_KP_NUMLOCK = 0xB8,
		K_KP_STAR = 0xB9,
		K_KP_EQUALS = 0xBA,
		K_MOUSE1 = 0xBB,
		K_START_MOUSEBUTTON = 0xBB,
		K_START_MOUSE = 0xBB,
		K_MOUSE2 = 0xBC,
		K_MOUSE3 = 0xBD,
		K_MOUSE4 = 0xBE,
		K_MOUSE5 = 0xBF,
		K_LAST_MOUSEBUTTON = 0xBF,
		K_MWHEELDOWN = 0xC0,
		K_START_MOUSEWHEEL = 0xC0,
		K_MWHEELUP = 0xC1,
		K_MWHEELLEFT = 0xC2,
		K_MWHEELRIGHT = 0xC3,
		K_LAST_MOUSEWHEEL = 0xC3,
		K_LAST_MOUSE = 0xC3,
		K_BUTTON_BACK_LTRIG = 0xC4,
		K_BUTTON_BACK_LSHLDR = 0xC5,
		K_BUTTON_BACK_RTRIG = 0xC6,
		K_BUTTON_BACK_RSHLDR = 0xC7,
		K_BUTTON_BACK_A = 0xC8,
		K_BUTTON_BACK_B = 0xC9,
		K_BUTTON_BACK_X = 0xCA,
		K_BUTTON_BACK_Y = 0xCB,
		K_BUTTON_BACK_LSTICK = 0xCC,
		K_BUTTON_BACK_RSTICK = 0xCD,
		K_BUTTON_BACK_UP = 0xCE,
		K_BUTTON_BACK_DOWN = 0xCF,
		K_BUTTON_BACK_LEFT = 0xD0,
		K_BUTTON_BACK_RIGHT = 0xD1,
		K_BUTTON_VITA_L1 = 0xD2,
		K_BUTTON_VITA_R1 = 0xD3,
		K_BUTTON_VITA_L2 = 0xD4,
		K_BUTTON_VITA_R2 = 0xD5,
		K_BUTTON_VITA_L3 = 0xD6,
		K_BUTTON_VITA_R3 = 0xD7,
		K_BUTTON_VITA_TOUCHPAD = 0xD8,
		K_BUTTON_OPTIONS = 0xD9,
		K_APAD2_UP = 0xDA,
		K_FIRSTGAMEPADBUTTON_RANGE_4 = 0xDA,
		K_FIRSTBPAD = 0xDA,
		K_APAD2_DOWN = 0xDB,
		K_APAD2_LEFT = 0xDC,
		K_APAD2_RIGHT = 0xDD,
		K_LASTBPAD = 0xDD,
		K_LASTGAMEPADBUTTON_RANGE_4 = 0xDD,
		K_INHERIT = 0xDE,
		K_LAST_KEY = 0xDE,
	};

	struct msg_t
	{
		int overflowed;
		int readOnly;
		unsigned __int8* data;
		unsigned __int8* splitData;
		int maxsize;
		int cursize;
		int splitSize;
		int readcount;
		int bit;
		int lastEntityRef;
		int targetLocalNetID;
		unsigned int compressionFlags;
	};

	struct XUID
	{
		unsigned __int64 m_id;

		void deserialize(const game::msg_t* msg);
		void serialize(const msg_t* msg);

		unsigned __int64 get_id();

		XUID* random_xuid();

		bool operator !=(const XUID* xuid);
		XUID* operator =(const XUID* xuid);
		bool operator ==(const XUID* xuid);
	};

	struct cmd_function_s
	{
		cmd_function_s* next;
		const char* name;
		const char** autoCompleteList;
		unsigned int autoCompleteListCount;
		void(__fastcall* function)();
	};

	struct ScriptFile
	{
		const char* name;
		int compressedLen;
		int len;
		int bytecodeLen;
		char* buffer;
		char* bytecode;
	};

	struct RawFile
	{
		const char* name;
		int compressedLen;
		int len;
		const char* buffer;
	};

	union XAssetHeader
	{
		RawFile* rawfile;
		ScriptFile* scriptfile;
	};

	/*
	enum XAssetType_pdb : std::uint32_t
	{
		ASSET_TYPE_PHYSICSLIBRARY = 0x0,
		ASSET_TYPE_PHYSICS_SFX_EVENT_ASSET = 0x1,
		ASSET_TYPE_PHYSICS_VFX_EVENT_ASSET = 0x2,
		ASSET_TYPE_PHYSICSASSET = 0x3,
		ASSET_TYPE_PHYSICS_FX_PIPELINE = 0x4,
		ASSET_TYPE_PHYSICS_FX_SHAPE = 0x5,
		ASSET_TYPE_PHYSICS_DEBUG_DATA = 0x6,
		ASSET_TYPE_XANIMPARTS = 0x7,
		ASSET_TYPE_XMODEL_SURFS = 0x8,
		ASSET_TYPE_XMODEL = 0x9,
		ASSET_TYPE_MAYHEM = 0xA,
		ASSET_TYPE_MATERIAL = 0xB,
		ASSET_TYPE_COMPUTESHADER = 0xC,
		ASSET_TYPE_SERIALIZEDSHADER = 0xD,
		ASSET_TYPE_TECHNIQUE_SET = 0xE,
		ASSET_TYPE_IMAGE = 0xF,
		ASSET_TYPE_SOUND_GLOBALS = 0x10,
		ASSET_TYPE_SOUND_BANK = 0x11,
		ASSET_TYPE_SOUND_BANK_TRANSIENT = 0x12,
		ASSET_TYPE_CLIPMAP = 0x13,
		ASSET_TYPE_COMWORLD = 0x14,
		ASSET_TYPE_GLASSWORLD = 0x15,
		ASSET_TYPE_PATHDATA = 0x16,
		ASSET_TYPE_NAVMESH = 0x17,
		ASSET_TYPE_TACGRAPH = 0x18,
		ASSET_TYPE_MAP_ENTS = 0x19,
		ASSET_TYPE_FXWORLD = 0x1A,
		ASSET_TYPE_GFXWORLD = 0x1B,
		ASSET_TYPE_GFXWORLD_TRANSIENT_ZONE = 0x1C,
		ASSET_TYPE_IESPROFILE = 0x1D,
		ASSET_TYPE_LIGHT_DEF = 0x1E,
		ASSET_TYPE_GRADING_CLUT = 0x1F,
		ASSET_TYPE_UI_MAP = 0x20,
		ASSET_TYPE_FOG_SPLINE = 0x21,
		ASSET_TYPE_ANIMCLASS = 0x22,
		ASSET_TYPE_PLAYERANIM = 0x23,
		ASSET_TYPE_GESTURE = 0x24,
		ASSET_TYPE_LOCALIZE_ENTRY = 0x25,
		ASSET_TYPE_ATTACHMENT = 0x26,
		ASSET_TYPE_WEAPON = 0x27,
		ASSET_TYPE_VFX = 0x28,
		ASSET_TYPE_IMPACT_FX = 0x29,
		ASSET_TYPE_SURFACE_FX = 0x2A,
		ASSET_TYPE_AITYPE = 0x2B,
		ASSET_TYPE_MPTYPE = 0x2C,
		ASSET_TYPE_CHARACTER = 0x2D,
		ASSET_TYPE_XMODELALIAS = 0x2E,
		ASSET_TYPE_RAWFILE = 0x2F,
		ASSET_TYPE_SCRIPTFILE = 0x30,
		ASSET_TYPE_SCRIPT_DEBUG_DATA = 0x31,
		ASSET_TYPE_STRINGTABLE = 0x32,
		ASSET_TYPE_LEADERBOARD = 0x33,
		ASSET_TYPE_VIRTUAL_LEADERBOARD = 0x34,
		ASSET_TYPE_DDL = 0x35,
		ASSET_TYPE_TRACER = 0x36,
		ASSET_TYPE_VEHICLE = 0x37,
		ASSET_TYPE_ADDON_MAP_ENTS = 0x38,
		ASSET_TYPE_NET_CONST_STRINGS = 0x39,
		ASSET_TYPE_LUA_FILE = 0x3A,
		ASSET_TYPE_SCRIPTABLE = 0x3B,
		ASSET_TYPE_EQUIPMENT_SND_TABLE = 0x3C,
		ASSET_TYPE_VECTORFIELD = 0x3D,
		ASSET_TYPE_PARTICLE_SIM_ANIMATION = 0x3E,
		ASSET_TYPE_STREAMING_INFO = 0x3F,
		ASSET_TYPE_LASER = 0x40,
		ASSET_TYPE_TTF = 0x41,
		ASSET_TYPE_SUIT = 0x42,
		ASSET_TYPE_SUITANIMPACKAGE = 0x43,
		ASSET_TYPE_CAMERA = 0x44,
		ASSET_TYPE_HUDOUTLINE = 0x45,
		ASSET_TYPE_SPACESHIPTARGET = 0x46,
		ASSET_TYPE_RUMBLE = 0x47,
		ASSET_TYPE_RUMBLE_GRAPH = 0x48,
		ASSET_TYPE_ANIM_PACKAGE = 0x49,
		ASSET_TYPE_SFX_PACKAGE = 0x4A,
		ASSET_TYPE_VFX_PACKAGE = 0x4B,
		ASSET_TYPE_FOOTSTEP_VFX = 0x4C,
		ASSET_TYPE_BEHAVIOR_TREE = 0x4D,
		ASSET_TYPE_ANIMSET = 0x4E,
		ASSET_TYPE_ASM = 0x4F,
		ASSET_TYPE_XANIM_PROCEDURALBONES = 0x50,
		ASSET_TYPE_XANIM_DYNAMICBONES = 0x51,
		ASSET_TYPE_RETICLE = 0x52,
		ASSET_TYPE_XANIMCURVE = 0x53,
		ASSET_TYPE_COVERSELECTOR = 0x54,
		ASSET_TYPE_ENEMYSELECTOR = 0x55,
		ASSET_TYPE_CLIENTCHARACTER = 0x56,
		ASSET_TYPE_CLOTHASSET = 0x57,
		ASSET_TYPE_CINEMATICMOTION = 0x58,
		ASSET_TYPE_ACCESSORY = 0x59,
		ASSET_TYPE_LOCDMGTABLE = 0x5A,
		ASSET_TYPE_BULLETPENETRATION = 0x5B,
		ASSET_TYPE_SCRIPTBUNDLE = 0x5C,
		ASSET_TYPE_BLENDSPACE2D = 0x5D,
		ASSET_TYPE_XCAM = 0x5E,
		ASSET_TYPE_CAMO = 0x5F,
		ASSET_TYPE_XCOMPOSITEMODEL = 0x60,
		ASSET_TYPE_XMODEL_DETAIL_COLLISION = 0x61,
		ASSET_TYPE_STREAM_KEY = 0x62,
		ASSET_TYPE_STREAM_TREE_OVERRIDE = 0x63,
		ASSET_TYPE_KEYVALUEPAIRS = 0x64,
		ASSET_TYPE_SUPER_TERRAIN = 0x65,
		ASSET_TYPE_NATIVE_SCRIPT_PATCH = 0x66,
		ASSET_TYPE_COLLISION_TILE = 0x67,
		ASSET_TYPE_EXECUTION = 0x68,
		ASSET_TYPE_CARRYOBJECT = 0x69,
		ASSET_TYPE_SOUNDBANKLIST = 0x6A,
		ASSET_TYPE_DECAL_VOLUME_MATERIAL = 0x6B,
		ASSET_TYPE_DECAL_VOLUME_MASK = 0x6C,
		ASSET_TYPE_DYNENTITY_LIST = 0x6D,
		ASSET_TYPE_FXWORLD_TRANSIENT_ZONE = 0x6E,
		ASSET_TYPE_DLOG_SCHEMA = 0x6F,
		ASSET_TYPE_EDGE_LIST = 0x70,
		ASSET_TYPE_COUNT = 0x71,
		ASSET_TYPE_STRING = 0x71,
		ASSET_TYPE_ASSETLIST = 0x72,
	};
	*/

	enum XAssetType : __int32
	{
		ASSET_TYPE_XMODEL = 9,
		ASSET_TYPE_GFXWORLD = 31,
		ASSET_TYPE_WEAPON = 0x2B,		// 0x27 maybe? (weapon???)
		ASSET_TYPE_RAWFILE = 0x33,		// 0x2F
		ASSET_TYPE_SCRIPTFILE = 0x34	// 0x30 on PDB
	};

	enum DBMemoryType
	{
		DM_MEMORY_VIRTUAL = 0,
		DM_MEMORY_SCRIPT = 1,
		DM_MEMORY_TEMP = 2,
		DM_MEMORY_GPUTEMP = 3,
		DM_MEMORY_COUNT = 4,
	};

	enum XFileBlock
	{
		XFILE_BLOCK_SCRIPT = 6,
		MAX_XFILE_COUNT = 8,
	};

	struct XBlock
	{
		char* data;
		unsigned __int64 size;
	};

	enum Mem_PageID
	{
	};

	struct Mem_PageRange
	{
		Mem_PageID firstPageID;
		Mem_PageID lastPageID;
	};

	struct XZoneMemoryAllocation
	{
		Mem_PageRange pageRange;
		char* alloc;
		unsigned __int64 size;
	};

	struct XArchiveBlocks
	{
		XBlock blocks[MAX_XFILE_COUNT];
	};

	struct EncryptionHeader
	{
		unsigned int isEncrypted;
		char IV[16];
	};

	struct __declspec(align(8)) XFile
	{
		unsigned __int64 size;
		unsigned __int64 preloadWalkSize;
		unsigned __int64 blockSize[11];
		EncryptionHeader encryption;
	};

	struct DB_FFHeader
	{
		char magic[8]; // 0
		unsigned int headerVersion; // 8
		unsigned int xfileVersion; // 12
		bool dashCompressBuild; // 16
		bool dashEncryptBuild;
		BYTE transientFileType[1];
		unsigned int residentPartSize;
		unsigned int residentHash;
		unsigned int alwaysLoadedPartSize;
		XFile xfileHeader;
	};

	enum XAnimParameterType
	{
		INVALID = 0x0,
		BOOL_VALUE = 0x1,
		BYTE_VALUE = 0x2,
		BYTE_POINTER = 0x3,
		INT_VALUE = 0x4,
		FLOAT_VALUE = 0x5,
		FLOAT_POINTER = 0x6,
		VEC3_VALUE = 0x7,
		VEC3_POINTER = 0x8,
		STRUCT_POINTER = 0x9,
	};

	typedef XAnimParameterType FileStreamFileID;

	struct DBFileHandle
	{
		FileStreamFileID fileID;
		unsigned __int64 dcacheFileID;
	};

	struct EncryptionInfo
	{
		EncryptionHeader header;
		char privateKey[32];
	};

	struct DBFile
	{
		char name[64];
		DBFileHandle dbFileHandle;
		bool isSecured;
		EncryptionInfo encryption;
	};

	struct BDiffWindowSizes
	{
		unsigned __int64 destWindow;
		unsigned __int64 sourceWindow;
		unsigned __int64 diffWindow;
	};

	struct DB_FDHeader
	{
		char magic[8];
		unsigned int version;
		unsigned int diffVersion;
		BDiffWindowSizes residentWindowSizes;
		unsigned __int64 residentDiffCompSize;
		unsigned __int64 residentDiffUncompSize;
		DB_FFHeader baseHeader;
		DB_FFHeader newHeader;
	};

	struct DB_FFDiffData
	{
		DBFile file;
		DB_FDHeader header;
	};

	struct __declspec(align(8)) DB_FFOpenData
	{
		DBFile baseFastfile;
		DB_FFHeader baseHeader;
		DB_FFHeader topHeader;
		DB_FFDiffData diff[2];
		unsigned int diffCount;
	};

	struct XZoneTemporaryLoadData
	{
		DB_FFOpenData openData;
		XArchiveBlocks archiveBlocks;
	};

	struct XZoneMemory
	{
		XZoneMemoryAllocation alloc[DM_MEMORY_COUNT];
		XZoneTemporaryLoadData* tempData;
		// there's more data after this, be warned
	};

	enum errorParm_t
	{
		ERR_FATAL = 0x0,
		ERR_DROP = 0x1,
		ERR_SERVERDISCONNECT = 0x2,
		ERR_DISCONNECT = 0x3,
		ERR_SCRIPT = 0x4,
		ERR_SCRIPT_DROP = 0x5,
		ERR_LOCALIZATION = 0x6,
		ERR_COUNT = 0x7,
	};

	/*
	struct CmdArgs
	{
		int nesting;
		int localClientNum[8];
		int controllerIndex[8];
		int argc[8];
		const char** argv[8];
	};
	*/

	struct CmdArgs
	{
		int nesting;
		int localClientNum[8];
		int controllerIndex[8];
		int argc[8];
		const char** argv[8];
		char textPool[16384]; // new all below and here
		const char* argvPool[512];
		int usedTextPool[8];
		int totalUsedArgvPool;
		int totalUsedTextPool;
	};

	struct SvCommandInfo
	{
		const char* name;
		void(__fastcall* function)();
		cmd_function_s svvar;
	};

	struct gentity_s
	{
		__int16 s_number; // 0
		char __pad0[0x8E]; // 2
		__int16 client_num; // 144
		//char __pad1[0x8E];// 146
	};

	union $19C82CA8BD5CE28553D0D27D79F0E3F3
	{
		const char* m_scriptPos;
		unsigned __int64 m_genericPos;
	};

	struct scr_entref_t
	{
		unsigned short entnum;
		unsigned short classnum;
	};

	struct scrContext_t;

	using builtin_function = void(*)(scrContext_t*);
	using builtin_method = void(*)(scrContext_t*, scr_entref_t);

	struct ScriptCodePos
	{
		$19C82CA8BD5CE28553D0D27D79F0E3F3 ___u0;
	};

	struct VariableStackBuffer
	{
		const char* pos;
		unsigned __int16 size;
		unsigned __int16 bufLen;
		unsigned __int16 localId; // type unsigned int?
		char time;
		char buf[1];
	};

	union VariableUnion
	{
		int intValue;
		unsigned int uintValue;
		float floatValue;
		unsigned int stringValue;
		const float* vectorValue;
		const char* codePosValue;
		unsigned __int64 scriptCodePosValue;
		unsigned int pointerValue;
		VariableStackBuffer* stackValue;
		unsigned int entityOffset;
	};

	struct VariableValue
	{
		VariableUnion u; // 0
		int type; // 8
	};

	enum VariableType
	{
		VAR_UNDEFINED = 0x0,
		VAR_BEGIN_REF = 0x1,
		VAR_POINTER = 0x1,
		VAR_STRING = 0x2,
		VAR_ISTRING = 0x3,
		VAR_VECTOR = 0x4,
		VAR_END_REF = 0x5,
		VAR_FLOAT = 0x5,
		VAR_INTEGER = 0x6,
		VAR_CODEPOS = 0x7,
		VAR_PRECODEPOS = 0x8,
		VAR_FUNCTION = 0x9,
		VAR_BUILTIN_FUNCTION = 0xA,
		VAR_BUILTIN_METHOD = 0xB,
		VAR_STACK = 0xC,
		VAR_ANIMATION = 0xD,
		//VAR_DEVELOPER_CODEPOS = 0xE,
		VAR_PRE_ANIMATION = 0xE,
		VAR_ANIM_TREE = 0xF,
		VAR_THREAD = 0x10,
		VAR_NOTIFY_THREAD = 0x11,
		VAR_TIME_THREAD = 0x12,
		VAR_CHILD_THREAD = 0x13,
		VAR_OBJECT = 0x14,
		VAR_DEAD_ENTITY = 0x15,
		VAR_ENTITY = 0x16,
		VAR_ARRAY = 0x17,
		VAR_DEAD_THREAD = 0x18,
		VAR_COUNT = 0x19,
		VAR_FREE = 0x19,
		VAR_THREAD_LIST = 0x1A,
		VAR_ENDON_LIST = 0x1B,
		VAR_TOTAL_COUNT = 0x1C,

		// idk what these are, but i'll keep them
		VAR_FIRST_OBJECT = 0x11,
		VAR_FIRST_CLEARABLE_OBJECT = 0x15,
		VAR_LAST_NONENTITY_OBJECT = 0x15,
		VAR_FIRST_ENTITY_OBJECT = 0x17,
		VAR_FIRST_NONFIELD_OBJECT = 0x18,
		VAR_FIRST_DEAD_OBJECT = 0x19,
	};

	struct ObjectVariableChildren
	{
		unsigned __int16 firstChild;
		unsigned __int16 lastChild;
	};

	struct ObjectVariableValue_u_f
	{
		unsigned __int16 prev;
		unsigned __int16 next;
	};

	union ObjectVariableValue_u_o_u
	{
		unsigned __int16 size;
		unsigned __int16 entnum;
		unsigned __int16 nextEntId;
		unsigned __int16 self;
	};

	struct	ObjectVariableValue_u_o
	{
		unsigned __int16 refCount;
		ObjectVariableValue_u_o_u u;
	};

	union ObjectVariableValue_w
	{
		unsigned int type;
		unsigned int classnum;
		unsigned int notifyName;
		unsigned int waitTime;
		unsigned int parentLocalId;
	};

	union ObjectVariableValue_u
	{
		ObjectVariableValue_u_f f;
		ObjectVariableValue_u_o o;
	};

	struct ObjectVariableValue
	{
		ObjectVariableValue_u u;
		ObjectVariableValue_w w;
	};

	/* 21481 */
	struct ChildVariableValue_FreeListOrVariableUnion_f
	{
		unsigned int prev;
		unsigned int next;
	};

	/* 21482 */
	union ChildVariableValue_FreeListOrVariableUnion
	{
		ChildVariableValue_FreeListOrVariableUnion_f f;
		VariableUnion u;
	};

	struct ChildBucketMatchKeys_keys
	{
		unsigned __int16 name_hi;
		unsigned __int16 parentId;
	};

	/* 21485 */
	union ChildBucketMatchKeys
	{
		ChildBucketMatchKeys_keys keys;
		unsigned int match;
	};

	struct ChildVariableValue
	{
		ChildVariableValue_FreeListOrVariableUnion u;
		unsigned int next;
		char type;
		char name_lo;
		ChildBucketMatchKeys k;
		unsigned int nextSibling;
		unsigned int prevSibling;
	};

	struct function_stack_t
	{
		ScriptCodePos pos;
		unsigned int localId;
		unsigned int localVarCount;
		VariableValue* top;
		VariableValue* startTop;
	};

	struct function_frame_t
	{
		function_stack_t fs;
		int topType;
	};

	struct scrContext_t
	{
		char __pad0[3184]; // 0
		bool script_loading; // 3184
		char __pad1[3]; // 3185
		unsigned int m_funcBegin; // 3188
		unsigned int m_funcEnd; // 3192
		unsigned int m_funcCount; // 3196
		unsigned int m_methBegin; // 3200
		unsigned int m_methEnd; // 3204
		unsigned int m_methCount; // 3208
		char __pad2[4]; // 3212
		builtin_function* m_pFuncTable; // 3216
		builtin_method* m_pMethTable; // 3224
		char __pad3[10248]; // 3232

		ObjectVariableValue* objectVariableValue; // 13480
		ObjectVariableChildren* objectVariableChildren; // 13488
		void* unk_0; // 13496
		ChildVariableValue* childVariableValue; // 13504

		char __pad3_1[34016]; // 13512

		unsigned int* localVars; // 47528
		VariableValue* maxstack; // 47536 (scrVmPub_t->maxstack)
		int function_count; // 47544
		int __pad4; // 47548
		function_frame_t* function_frame; // 47552
		VariableValue* top; // 47560
		int __pad5; // 47568

		// confirmed all below
		unsigned int inparamcount; // 47572
		unsigned int outparamcount; // 47576
		function_frame_t function_frame_start[64]; // 47580
		char __pad6[35376]; // 47644

		ScriptCodePos pos; // 82920 // ScriptCodePos::GetScriptPos(scrContext + 82920);
	};

	/*
		
		zone research
	
	*/
	struct WeaponAnimPackage
	{
		const char* name; // 0
		char __pad0[56]; // 8
	};
	static_assert(sizeof(WeaponAnimPackage) == 64);

	struct WeaponDef
	{
		const char* szOverlayName; // 0
		char __pad0[112]; // 8

		// new to 1.20
		void* playerShadowModel; // 120
		void* playerShadowModelLeftHand; // 128
		void* playerShadowModelRightHand; // 136

		WeaponAnimPackage* szXAnims; // 144
		WeaponAnimPackage* szXAnimsRightHanded; // 152
		WeaponAnimPackage* szXAnimsLeftHanded; // 160

		char __pad1[5128]; // 168
	};
	static_assert(sizeof(WeaponDef) == 5296);

	/*
	struct GfxShaderBufferView
	{
		void* resource; // 0
		unsigned int view; // 8
		// 12
	};
	*/

	struct GfxWrappedBuffer
	{
		void* buffer; // 0
		char view[48]; // 8
		void* data; // 56
	};
	static_assert(sizeof(GfxWrappedBuffer) == 64);

	struct GfxFrustumLights
	{
		char __pad0[64]; // 0
		GfxWrappedBuffer indexBuffer;		// 64
		GfxWrappedBuffer vertexBuffer;		// 128
	};
	static_assert(sizeof(GfxFrustumLights) == 192);

	struct GfxWorld
	{
		const char* name; // 0
		const char* baseName; // 8
		int bspVersion; // 16

		char __pad0[14460]; // 20
		char dynamicLightset[928]; // 14480
		char mayhemSelfVis[112]; // 15408

		GfxFrustumLights frustumLights; // 15520 (160 on 1.19, 32 byte difference)

		// this can all be stored into the pad but im using it for debugging
		void* lightViewFrustums; // 15712
		void* primaryLights; // 15720
		__int64 voxelTreeCount; // 15728
		void* voxelTree; // 15736

		char __pad1[2064]; // 15744
	};
	static_assert(sizeof(GfxWorld) == 17808);

	namespace iw8_1_19
	{
		struct WeaponDef
		{
			const char* szOverlayName; // 0
			char __pad0[112]; // 8

			WeaponAnimPackage* szXAnims; // 120
			WeaponAnimPackage* szXAnimsRightHanded; // 128
			WeaponAnimPackage* szXAnimsLeftHanded; // 136

			char __pad1[5128]; // 144
		};
		static_assert(sizeof(iw8_1_19::WeaponDef) == 5272);

		struct GfxWrappedBuffer
		{
			void* buffer; // 0
			char __pad0[24]; // 8
		};

		struct GfxFrustumLights
		{
			char __pad0[64]; // 0
			iw8_1_19::GfxWrappedBuffer indexBuffer; // 64
			game::GfxWrappedBuffer vertexBuffer; // 96
		};
		static_assert(sizeof(iw8_1_19::GfxFrustumLights) == 160);

		struct GfxWorld
		{
			const char* name; // 0
			const char* baseName; // 8
			int bspVersion; // 16

			char __pad0[14460]; // 20
			char dynamicLightset[928]; // 14480

			// this can all be stored into the pad but im using it for debugging
			char mayhemSelfVis[112]; // 15408
			iw8_1_19::GfxFrustumLights frustumLights; // 15520
			void* lightViewFrustums; // 15680
			void* primaryLights; // 15688
			__int64 voxelTreeCount; // 15696
			void* voxelTree; // 15704

			char __pad1[2064]; // 15712
		};
		static_assert(sizeof(iw8_1_19::GfxWorld) == 17776);
	}
}

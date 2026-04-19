#pragma once

#include "structs.hpp"

namespace game
{
	extern uint64_t base_address;
	void load_base_address();

	DvarValue* get_current(dvar_t* dvar);
	bool dvar_is_enabled_safe(dvar_t*);

	void Dvar_SetBool_Internal(game::dvar_t* dvar, bool value);

	int Cmd_Argc();
	const char* Cmd_Argv(int argIndex);

	constexpr std::uint64_t hash_64a(const std::string& str, std::uint64_t start, std::uint64_t iv) 
	{
		std::uint64_t hash = start;

		for (const char* data = str.data(); *data; data++) 
		{
			if (*data >= 'A' && *data <= 'Z') {
				hash = hash ^ (std::uint8_t)('a' + (*data - (std::uint8_t)'A'));
			}
			else if (*data == '\\') {
				hash = hash ^ '/';
			}
			else {
				hash = hash ^ *data;
			}

			hash *= iv;
		}

		return hash;
	}

	constexpr std::uint64_t hash_scr_dvar(const std::string& str) 
	{
		switch (str.at(0)) {
		case 'a': case 'A':
			return hash_64a(str, 0xFC9B930CF7416D64, 0x10000000233);
		case 'b': case 'B':
			return hash_64a(str, 0x9A88BEE10E2FB71C, 0x10000000233);
		case 'c': case 'C':
			return hash_64a(str, 0x193B2D4166F1E474, 0x10000000233);
		case 'd': case 'D':
			return hash_64a(str, 0x90BBD97D817C3BB4, 0x10000000233);
		case 'e': case 'E':
			return hash_64a(str, 0x4BF68A19BDA2F7BC, 0x10000000233);
		case 'f': case 'F':
			return hash_64a(str, 0x220E8473FBFE04E4, 0x10000000233);
		case 'g': case 'G':
			return hash_64a(str, 0x85B013A4E70EDCAC, 0x10000000233);
		case 'h': case 'H':
			return hash_64a(str, 0x35359AD030F0CECC, 0x10000000233);
		case 'i': case 'I':
			return hash_64a(str, 0xB742F8B10AE9E1F4, 0x10000000233);
		case 'j': case 'J':
			return hash_64a(str, 0x629C7AB30DDD511C, 0x10000000233);
		case 'k': case 'K':
			return hash_64a(str, 0x7B50CE4016A19064, 0x10000000233);
		case 'l': case 'L':
			return hash_64a(str, 0x827CCF83792891C4, 0x10000000233);
		case 'm': case 'M':
			return hash_64a(str, 0xBDDB2ACB28358A7C, 0x10000000233);
		case 'n': case 'N':
			return hash_64a(str, 0x772E559AE9E22494, 0x10000000233);
		case 'o': case 'O':
			return hash_64a(str, 0x0F579F12D8AF33CC, 0x10000000233);
		case 'p': case 'P':
			return hash_64a(str, 0x4EFF8082D8DFCB2C, 0x10000000233);
		case 'r': case 'R':
			return hash_64a(str, 0x68E7F378CBD4275C, 0x10000000233);
		case 's': case 'S':
			return hash_64a(str, 0x9EACF1B19DED9334, 0x10000000233);
		case 't': case 'T':
			return hash_64a(str, 0xBD0C7686C08A04F4, 0x10000000233);
		case 'u': case 'U':
			return hash_64a(str, 0x5219561CA516433C, 0x10000000233);
		case 'v': case 'V':
			return hash_64a(str, 0xBC6AD2D970767A44, 0x10000000233);
		case 'w': case 'W':
			return hash_64a(str, 0x8355B5EA1EC0C1CC, 0x10000000233);
		case 'x': case 'X':
			return hash_64a(str, 0xF2C1E13850ECCEAC, 0x10000000233);
		default:
			return 0;
		}
	}

	template <typename T>
	class symbol
	{
	public:
		symbol(const size_t address)
			: address_(reinterpret_cast<T*>(address))
		{
		}

		T* get() const
		{
			return reinterpret_cast<T*>((uint64_t)address_ + base_address);
		}

		operator T* () const
		{
			return this->get();
		}

		T* operator->() const
		{
			return this->get();
		}

	private:
		T* address_;
	};

	dvar_t* Dvar_RegisterString(const char* name, const char* str, game::DvarFlags flags, const char* desc);
}

size_t operator"" _b(const size_t ptr);
size_t reverse_b(const size_t ptr);
size_t reverse_b(const void* ptr);

#include "symbols.hpp"

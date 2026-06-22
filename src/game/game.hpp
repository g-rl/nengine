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

	constexpr std::uint32_t START_DEFAULT_32 = 0x811C9DC5;
	constexpr std::uint32_t IV_DEFAULT_32 = 0x1000193;
	constexpr std::uint32_t SIGN_32 = 0x7FFFFFFF;

	constexpr std::uint64_t START_DEFAULT_64 = 0xCBF29CE484222325;
	constexpr std::uint64_t IV_DEFAULT_64 = 0x100000001B3;
	constexpr std::uint64_t IV_TYPE2_64 = 0x10000000233;
	constexpr std::uint64_t SIGN_64 = 0x7FFFFFFFFFFFFFFF;

	constexpr char validate_char(char c) {
		if (c >= 'A' && c <= 'Z') {
			return 'a' + (c - 'A');
		}
		if (c == '\\') {
			return '/';
		}
		return c;
	}

	constexpr std::uint64_t hash_64a(const std::string& str, std::uint64_t start = START_DEFAULT_64, std::uint64_t iv = IV_DEFAULT_64) {
		std::uint64_t hash = start;
		for (const char* data = str.data(); *data; data++) {
			hash = (hash ^ validate_char(*data)) * iv;
		}
		return hash;
	}

	constexpr std::uint64_t hash_secure_internal(const std::string& str, std::uint64_t start, std::uint64_t iv) {
		if (!start) {
			return 0;
		}
		return hash_64a(str.substr(1), start, iv);
	}

	constexpr std::uint64_t hash_secure(const std::string& str, std::uint64_t start, const std::string& pattern, std::uint64_t iv) {
		if (str.empty()) {
			return 0;
		}
		return hash_secure_internal(str, hash_64a(pattern, (start ^ str.at(0)) * iv, iv), iv);
	}

	constexpr auto hash_scr_dvar(const std::string& str) {
		std::uint64_t fixed_start;
		switch (validate_char(str.at(0))) {
		case 'a':	fixed_start = 0xCB916A83C4E2C1FF; break;
		case 'b':	fixed_start = 0x0A7346F232F08A1A; break;
		case 'c':	fixed_start = 0x6F089DD565F89E95; break;
		case 'd':	fixed_start = 0xC95B1F03C4378A70; break;
		case 'e':	fixed_start = 0xB2298F9C0D67123B; break;
		case 'f':	fixed_start = 0xE3F1CD132FA3E9E6; break;
		case 'g':	fixed_start = 0x1117FEA825AF9271; break;
		case 'h':	fixed_start = 0xF5B21BDBA19672AC; break;
		case 'i':	fixed_start = 0xF22A8A5B005B2C47; break;
		case 'j':	fixed_start = 0xBB734DC77DBA2682; break;
		case 'k':	fixed_start = 0xD445A5F1C54FD0FD; break;
		case 'l':	fixed_start = 0x210E062374385478; break;
		case 'm':	fixed_start = 0xBE892DC96DBEA363; break;
		case 'n':	fixed_start = 0xFD0B3DB05C5751CE; break;
		case 'o':	fixed_start = 0x6CE67972894E8F79; break;
		case 'p':	fixed_start = 0x9BB2FBC2F42C3B54; break;
		case 'r':	fixed_start = 0x8A43A0A843922A2A; break;
		case 's':	fixed_start = 0xE3EAD09E517AE525; break;
		case 't':	fixed_start = 0x4C6D2A596F87E580; break;
		case 'u':	fixed_start = 0xA3FBA9FF0FF5F98B; break;
		case 'v':	fixed_start = 0xD567E632548EBBF6; break;
		case 'w':	fixed_start = 0x9640CCE1A1EA0E41; break;
		case 'x':	fixed_start = 0xCD3122D9F8CADC3C; break;
		default:	fixed_start = 0; break;
		}
		return hash_secure_internal(str, fixed_start, IV_TYPE2_64);
	}

	constexpr std::uint64_t hash_scr_dvar_iw9(const std::string& str)
	{
		return hash_secure(str, 0xD86A3B09566EBAAC, "q6n-+7=tyytg94_*", IV_TYPE2_64);
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

	dvar_t* Dvar_RegisterString(const char* name, const char* str, DvarFlags flags, const char* desc);
	dvar_t* Dvar_RegisterBool(const char* dvarName, bool value, DvarFlags flags, const char* desc);
}

size_t operator"" _b(const size_t ptr);
size_t reverse_b(const size_t ptr);
size_t reverse_b(const void* ptr);

#include "symbols.hpp"

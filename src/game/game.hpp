#pragma once

#include "structs.hpp"

namespace game
{
	extern uint64_t base_address;
	void load_base_address();

	DvarValue* get_current(dvar_t* dvar);
	bool dvar_is_enabled_safe(dvar_t*);

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

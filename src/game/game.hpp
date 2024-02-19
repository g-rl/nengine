#pragma once

#include "structs.hpp"

#define SERVER_CD_KEY "iw8mod-CD-Key"

namespace game
{
	extern uint64_t base_address;
	void load_base_address();

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

	namespace environment
	{
		bool is_dedi();
	}
}

size_t operator"" _b(const size_t ptr);
size_t reverse_b(const size_t ptr);
size_t reverse_b(const void* ptr);

#include "symbols.hpp"

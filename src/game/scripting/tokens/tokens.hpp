#pragma once

namespace tokens
{
	using u8 = std::uint8_t;
	using u16 = std::uint16_t;
	using u32 = std::uint32_t;
	using u64 = std::uint64_t;

	namespace s4
	{
		extern std::pair<u32, char const*> const token_list[];
		extern std::size_t const token_list_count;

		extern std::pair<u16, char const*> const func_list[];
		extern std::size_t const func_list_count;

		extern std::pair<u16, char const*> const meth_list[];
		extern std::size_t const meth_list_count;
	}

	namespace iw9
	{
		extern std::pair<u64, char const*> const hash_list[];
		extern std::size_t const hash_list_count;

		extern std::pair<u64, char const*> const path_list[];
		extern std::size_t const path_list_count;

		extern std::pair<u64, char const*> const func_list[];
		extern std::size_t const func_list_count;

		extern std::pair<u64, char const*> const meth_list[];
		extern std::size_t const meth_list_count;
	}
}

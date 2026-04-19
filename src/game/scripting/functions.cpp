#include <std_include.hpp>
#include "functions.hpp"

#include <utils/string.hpp>

#include "component/gsc/script_loading.hpp"
#include "component/gsc/script_extension.hpp"

#include "tokens/tokens.hpp"

#include <identification/game.hpp>

namespace scripting
{
	namespace
	{
		const std::unordered_map<std::uint32_t, std::string>& s4_token_map()
		{
			static const auto map = []
			{
				std::unordered_map<std::uint32_t, std::string> m;
				m.reserve(tokens::s4::token_list_count);
				for (std::size_t i = 0; i < tokens::s4::token_list_count; ++i)
				{
					const auto& [id, name] = tokens::s4::token_list[i];
					m.emplace(id, name);
				}
				return m;
			}();
			return map;
		}

		const std::unordered_map<std::uint64_t, std::string>& iw9_hash_map()
		{
			static const auto map = []
			{
				std::unordered_map<std::uint64_t, std::string> m;
				m.reserve(tokens::iw9::hash_list_count);
				for (std::size_t i = 0; i < tokens::iw9::hash_list_count; ++i)
				{
					const auto& [id, name] = tokens::iw9::hash_list[i];
					m.emplace(id, name);
				}
				return m;
			}();
			return map;
		}

		int find_function_index(const std::string& name, [[maybe_unused]] const bool prefer_global)
		{
			const auto target = utils::string::to_lower(name);
			auto const& first = gsc::gsc_ctx->func_map();
			auto const& second = gsc::gsc_ctx->meth_map();

			if (!prefer_global)
			{
				if (const auto itr = second.find(name); itr != second.end())
				{
					return static_cast<int>(itr->second);
				}

				if (const auto itr = first.find(name); itr != first.end())
				{
					return static_cast<int>(itr->second);
				}
			}

			if (const auto itr = first.find(name); itr != first.end())
			{
				return static_cast<int>(itr->second);
			}

			if (const auto itr = second.find(name); itr != second.end())
			{
				return static_cast<int>(itr->second);
			}

			return -1;
		}
	}

	inline std::string find_token(std::uint32_t id)
	{
		static const auto& game_ = identification::game::get_target_game().client_name;

		if (game_ == "s4-mod"s)
		{
			const auto& map = s4_token_map();
			if (const auto it = map.find(id); it != map.end())
			{
				return it->second;
			}
			return std::to_string(id);
		}

		if (game_ == "iw9-mod")
		{
			const auto& map = iw9_hash_map();
			if (const auto it = map.find(static_cast<std::uint64_t>(id)); it != map.end())
			{
				return it->second;
			}
			return std::to_string(id);
		}

		return gsc::gsc_ctx->token_name(id);
	}

	std::string find_token_single(std::uint32_t id)
	{
		return find_token(id);
	}

	script_function get_function_by_index(const std::uint32_t index)
	{
		static const auto function_table = &gsc::func_table;
		static const auto method_table = &gsc::meth_table;

		if (index < 0x1000)
		{
			return reinterpret_cast<script_function*>(function_table)[index - 1];
		}

		return reinterpret_cast<script_function*>(method_table)[index - 0x8000];
	}

	script_function find_function(const std::string& name, const bool prefer_global)
	{
		const auto index = find_function_index(name, prefer_global);
		if (index < 0) return nullptr;

		return get_function_by_index(index);
	}
}

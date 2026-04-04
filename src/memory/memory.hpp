#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <utils/nt.hpp>

#include "scanned_result.hpp"

namespace memory
{
	struct masked_signature
	{
		std::vector<std::uint8_t> data_{};
		std::vector<std::uint8_t> mask_{};
	};

	inline masked_signature masked_signature_from_string(const std::string& pattern)
	{
		const auto astroul = [](char c) -> std::uint8_t
		{
			if (c >= '0' && c <= '9') return static_cast<std::uint8_t>(c - '0');
			if (c >= 'A' && c <= 'F') return static_cast<std::uint8_t>(10 + (c - 'A'));
			if (c >= 'a' && c <= 'f') return static_cast<std::uint8_t>(10 + (c - 'a'));
			return 0;
		};

		masked_signature sig{};
		for (std::size_t ix = 0, len = pattern.length(); ix < len; ix++)
		{
			if (pattern[ix] == ' ') continue;

			const char h1 = pattern[ix];
			const char h2 = (ix + 1 < len && pattern[ix + 1] != ' ') ? pattern[ix + 1] : '?';

			if (h1 == '?' && h2 == '?')
			{
				sig.data_.push_back(0x00);
				sig.mask_.push_back(0x00);
			}
			else if (h1 != '?' && h2 == '?')
			{
				sig.data_.push_back(static_cast<std::uint8_t>(astroul(h1) << 4));
				sig.mask_.push_back(0xF0);
			}
			else if (h1 == '?' && h2 != '?')
			{
				sig.data_.push_back(astroul(h2));
				sig.mask_.push_back(0x0F);
			}
			else
			{
				sig.data_.push_back(static_cast<std::uint8_t>((astroul(h1) << 4) | astroul(h2)));
				sig.mask_.push_back(0xFF);
			}

			ix++;
		}
		return sig;
	}

	inline std::pair<std::uintptr_t, std::size_t> get_module_range(const utils::nt::library& library)
	{
		const auto base = reinterpret_cast<std::uintptr_t>(library.get_ptr());
		const auto headers = library.get_nt_headers();
		const auto size = headers ? static_cast<std::size_t>(headers->OptionalHeader.SizeOfImage) : 0;
		return { base, size };
	}

	inline std::vector<scanned_result<void>> masked_vectored_sig_scan(const utils::nt::library& library,
		const masked_signature& sig, std::size_t limit)
	{
		std::vector<scanned_result<void>> results{};
		const auto [mod_base, mod_len] = get_module_range(library);
		if (!mod_base || !mod_len || sig.mask_.empty()) return results;

		const std::size_t pat_len = sig.mask_.size();

		MEMORY_BASIC_INFORMATION page_info{};
		for (auto current_page = mod_base; current_page < mod_base + mod_len;
			current_page = reinterpret_cast<std::uintptr_t>(page_info.BaseAddress) + page_info.RegionSize)
		{
			if (!VirtualQuery(reinterpret_cast<LPCVOID>(current_page), &page_info, sizeof(page_info)))
			{
				break;
			}
			if (page_info.Protect == PAGE_NOACCESS || page_info.State != MEM_COMMIT || (page_info.Protect & PAGE_GUARD))
			{
				continue;
			}

			const auto page_base = reinterpret_cast<std::uintptr_t>(page_info.BaseAddress);
			const auto page_end = page_base + page_info.RegionSize;
			if (page_end < pat_len + 0x8) continue;

			for (auto current_addr = page_base; current_addr < page_end - 0x8 - pat_len; current_addr++)
			{
				if (current_addr >= mod_base + mod_len - pat_len) continue;

				bool found = true;
				for (std::size_t jx = 0; jx < pat_len; jx++)
				{
					const auto byte = *reinterpret_cast<std::uint8_t*>(current_addr + jx);
					if ((byte & sig.mask_[jx]) != sig.data_[jx])
					{
						found = false;
						break;
					}
				}

				if (found)
				{
					results.emplace_back(current_addr);
					if (results.size() >= limit) return results;
				}
			}
		}

		return results;
	}

	inline scanned_result<void> masked_sig_scan(const utils::nt::library& library, const masked_signature& sig)
	{
		auto r = masked_vectored_sig_scan(library, sig, 1);
		if (!r.empty()) return r.front();
		return scanned_result<void>(nullptr);
	}

	inline std::vector<scanned_result<void>> vectored_sig_scan(const utils::nt::library& library,
		const std::string& pattern, std::size_t limit)
	{
		return masked_vectored_sig_scan(library, masked_signature_from_string(pattern), limit);
	}

	inline scanned_result<void> sig_scan(const utils::nt::library& library, const std::string& pattern)
	{
		return masked_sig_scan(library, masked_signature_from_string(pattern));
	}
}

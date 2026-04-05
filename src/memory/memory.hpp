#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <Psapi.h>
#pragma comment(lib, "Psapi.lib")

#include <utils/nt.hpp>

#include "scanned_result.hpp"

// Scanner ported verbatim from ZeroProxy (common/memory/memory.hpp).
// Keep the logic identical — it is known-good against Arxan'd MW builds.

#define VT_GET(ptr, idx) (*(void***)(ptr))[idx]
#define PTR_AS(type, ptr) reinterpret_cast<type>((ptr))
#define VAL_AS(type, val) static_cast<type>((val))
#define DEREF_PTR_AS(type, ptr) *PTR_AS(type*, ptr)
#define ENUM_UNDER(val) static_cast<std::underlying_type_t<decltype(val)>>(val)
#define CLASS_ASSERT_SZ(cls, sz) static_assert(sizeof(cls) == sz, #cls " is not " #sz " bytes in size.")
#define CLASS_VALUE_AT_PTR(cls, off, t) PTR_AS(t*, PTR_AS(std::uintptr_t, cls) + off)
#define CLASS_VALUE_AT(cls, off, t) DEREF_PTR_AS(t, PTR_AS(std::uintptr_t, cls) + off)
#define RSC_VAL(arg) PTR_AS(std::uintptr_t, arg)

namespace memory
{
	struct masked_signature {
		std::vector<std::uint8_t> data_{};
		std::vector<std::uint8_t> mask_{};
	};

	inline masked_signature masked_signature_from_string(const std::string& pattern) {
		const auto astroul = [](char c) -> std::uint8_t {
			if (c >= '0' && c <= '9') {
				return c - '0';
			}

			if (c >= 'A' && c <= 'F') {
				return 10 + (c - 'A');
			}

			if (c >= 'a' && c <= 'f') {
				return 10 + (c - 'a');
			}

			return 0;
		};

		masked_signature sig{};
		for (std::size_t ix = 0, len = pattern.length(); ix < len; ix++) {
			if (pattern[ix] == ' ') {
				continue;
			}

			char h1 = pattern[ix];
			char h2 = (ix + 1 < len && pattern[ix + 1] != ' ') ? pattern[ix + 1] : '?';

			if (h1 == '?' && h2 == '?') {
				sig.data_.push_back(0x00);
				sig.mask_.push_back(0x00); // full wildcard
			}
			else if (h1 != '?' && h2 == '?') {
				sig.data_.push_back(astroul(h1) << 4);
				sig.mask_.push_back(0xF0); // high nibble exact, low wildcard
			}
			else if (h1 == '?' && h2 != '?') {
				sig.data_.push_back(astroul(h2));
				sig.mask_.push_back(0x0F); // low nibble exact, high wildcard
			}
			else if (h1 != '?' && h2 != '?') {
				sig.data_.push_back((astroul(h1) << 4) | astroul(h2));
				sig.mask_.push_back(0xFF); // exact match
			}

			ix++; // do the funny
		}
		return sig;
	}

	template <typename T = void*>
	std::enable_if_t<std::is_pointer_v<T>, std::vector<scanned_result<std::remove_pointer_t<T>>>> masked_vectored_sig_scan(utils::nt::library library,
		masked_signature sig, std::size_t limit)
	{
		std::size_t pat_len = sig.mask_.size();
		MODULEINFO mod_info = library.get_info();
		std::uintptr_t mod_base = PTR_AS(std::uintptr_t, mod_info.lpBaseOfDll);
		std::uintptr_t mod_len = mod_info.SizeOfImage;
		std::vector<scanned_result<std::remove_pointer_t<T>>> results{};

#		if defined(_WIN64)
			MEMORY_BASIC_INFORMATION64 page_info = {};
#			define PAGE_BASE_ADDR(p) p.BaseAddress
#		else
			MEMORY_BASIC_INFORMATION page_info = {};
#			define PAGE_BASE_ADDR(p) PTR_AS(std::uintptr_t, p.BaseAddress)
#		endif
		for (auto current_page = mod_base; current_page < mod_base + mod_len; current_page = PAGE_BASE_ADDR(page_info) + page_info.RegionSize) {
			VirtualQuery(PTR_AS(LPCVOID, current_page), PTR_AS(PMEMORY_BASIC_INFORMATION, &page_info), sizeof(MEMORY_BASIC_INFORMATION));
			if (page_info.Protect == PAGE_NOACCESS || page_info.State != MEM_COMMIT || page_info.Protect & PAGE_GUARD) {
				continue;
			}

			for (auto current_addr = PAGE_BASE_ADDR(page_info); current_addr < PAGE_BASE_ADDR(page_info) + page_info.RegionSize - 0x8 - pat_len;
				current_addr++)
			{
				if (current_addr >= mod_base + mod_len - pat_len) {
					continue;
				}

				bool found = true;

				for (std::size_t jx = 0; jx < pat_len; jx++) {
                    std::uint8_t byte = DEREF_PTR_AS(std::uint8_t, current_addr + jx);
                    if ((byte & sig.mask_[jx]) != sig.data_[jx]) {
                        found = false;
                        break;
                    }
                }

				if (found) {
					results.push_back(scanned_result<std::remove_pointer_t<T>>(current_addr));
					if (results.size() >= limit) {
						return results;
					}
				}
			}
		}

		return results;
	}

	template <typename T = void*>
	std::enable_if_t<std::is_pointer_v<T>, scanned_result<std::remove_pointer_t<T>>> masked_sig_scan(utils::nt::library library, masked_signature sig) {
		std::vector<scanned_result<std::remove_pointer_t<T>>> result = masked_vectored_sig_scan<T>(library, sig, 1);
		if (result.size() > 0) {
			return result.at(0);
		}
		return scanned_result<std::remove_pointer_t<T>>(nullptr);
	}

	template <typename T = void*>
	std::enable_if_t<std::is_pointer_v<T>, std::vector<scanned_result<std::remove_pointer_t<T>>>> vectored_sig_scan(utils::nt::library library,
		std::string pattern, std::size_t limit, std::string name = "", bool print_fail = true)
	{
		std::vector<scanned_result<std::remove_pointer_t<T>>> res = masked_vectored_sig_scan<T>(library, masked_signature_from_string(pattern), limit);
		if (!name.empty()) {
			if (res.size() > 0) {
				printf("%s\n", std::format("Found '{}' {} + 0x{:X}[{} total]", name, library.get_name(), res.at(0).template as<std::uintptr_t>()
					- PTR_AS(std::uintptr_t, library.get_info().lpBaseOfDll), res.size()).data());
			}
			else if (print_fail) {
				printf("%s\n", std::format("Failed to find '{}' in{} ({})", name, library.get_name(), pattern).data());
			}
		}
		return res;
	}

	template <typename T = void*>
	std::enable_if_t<std::is_pointer_v<T>, scanned_result<std::remove_pointer_t<T>>> sig_scan(utils::nt::library library, std::string pattern,
		std::string name = "", bool print_fail = true)
	{
		scanned_result<std::remove_pointer_t<T>> res = masked_sig_scan<T>(library, masked_signature_from_string(pattern));
		if (!name.empty()) {
			if (res) {
				printf("%s\n", std::format("Found '{}' {}+0x{:X}", name, library.get_name(), res.template as<std::uintptr_t>()
					- PTR_AS(std::uintptr_t, library.get_info().lpBaseOfDll)).data());
			}
			else if (print_fail) {
				printf("%s\n", std::format("Failed to find '{}' in {}", name, library.get_name()).data());
			}
		}
		return res;
	}

	template <typename T = void*>
	std::enable_if_t<std::is_pointer_v<T>, scanned_result<std::remove_pointer_t<T>>> get_export(const std::string& module_name, const std::string& export_name) {
		utils::nt::library library(module_name);
		if (!library.is_valid()) {
			printf("%s\n", std::format("Failed to find module '{}' while trying to find '{}'", export_name, module_name).data());
			return scanned_result<std::remove_pointer_t<T>>(nullptr);
		}

		scanned_result<std::remove_pointer_t<T>> res = scanned_result<std::remove_pointer_t<T>>(library.get_proc<void*>(export_name));
		if (res) {
			printf("%s\n", std::format("Found '{}' {}+0x{:X}", export_name, module_name, res.template as<std::uintptr_t>()
				- PTR_AS(std::uintptr_t, library.get_info().lpBaseOfDll)).data());
		}
		else {
			printf("%s\n", std::format("Failed to find '{}' in {}", export_name, module_name).data());
		}
		return res;
	}
}

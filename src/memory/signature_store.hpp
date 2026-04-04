#pragma once

#include <functional>
#include <string>
#include <vector>

#include <utils/nt.hpp>

#include "memory.hpp"
#include "scanned_result.hpp"

namespace memory
{
	// Minimal signature store: collect (name, pattern, pointer, mod) entries,
	// then scan_all() resolves each and writes the result into *pointer.
	// No caching, no CI, no async — add if/when needed.
	class signature_store
	{
	public:
		using res = scanned_result<void>;
		using mod_fn = std::function<res(res)>;

		signature_store() : library_() {}
		explicit signature_store(const utils::nt::library& library) : library_(library) {}

		void add(const std::string& name, void** pointer, const std::string& pattern, mod_fn mod)
		{
			signatures_.push_back({ name, pattern, pointer, std::move(mod) });
		}

		void add(const std::string& name, void** pointer, const std::string& pattern)
		{
			signatures_.push_back({ name, pattern, pointer, [](res r) { return r; } });
		}

		struct scan_stats
		{
			std::uint32_t found = 0;
			std::uint32_t total = 0;
		};

		scan_stats scan_all()
		{
			scan_stats stats{};
			for (auto& sig : signatures_)
			{
				stats.total++;
				if (*sig.pointer != nullptr)
				{
					stats.found++;
					continue;
				}

				auto r = sig_scan(library_, sig.pattern);
				if (r)
				{
					*sig.pointer = sig.mod(r).as<void*>();
					stats.found++;
				}
				else
				{
					printf(("[sig] MISS: " + sig.name + " (" + sig.pattern + ")\n").c_str());
				}
			}
			return stats;
		}

	private:
		struct entry
		{
			std::string name;
			std::string pattern;
			void** pointer;
			mod_fn mod;
		};

		utils::nt::library library_;
		std::vector<entry> signatures_;
	};
}

#define SETUP_POINTER(name) #name, reinterpret_cast<void**>(&name)
#define SETUP_MOD(chain) [](memory::scanned_result<void> r) { return r.chain; }
#define GRAB_CALL SETUP_MOD(add(1).rip())

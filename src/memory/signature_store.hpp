#pragma once

#include <functional>
#include <string>
#include <vector>

#include <utils/nt.hpp>

#include "memory.hpp"
#include "scanned_result.hpp"

#include <game/game.hpp>

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

		using offset_mod_fn = std::function<std::uint32_t(res)>;
		void add(const std::string& name, std::uint32_t* out, const std::string& pattern, offset_mod_fn mod)
		{
			offset_signatures_.push_back({ name, pattern, out, std::move(mod) });
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
					printf(("[sig] found: " + sig.name + " at 0x%llx\n").c_str(),
						reinterpret_cast<uint64_t>(*sig.pointer) - game::base_address);
				}
				else
				{
					printf(("[sig] MISS: " + sig.name + " (" + sig.pattern + ")\n").c_str());
				}
			}

			for (auto& sig : offset_signatures_)
			{
				stats.total++;
				auto r = sig_scan(library_, sig.pattern);
				if (r)
				{
					*sig.out = sig.mod(r);
					stats.found++;
					printf(("[sig] offset: " + sig.name + " = 0x%x\n").c_str(), *sig.out);
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

		struct offset_entry
		{
			std::string name;
			std::string pattern;
			std::uint32_t* out;
			offset_mod_fn mod;
		};

		utils::nt::library library_;
		std::vector<entry> signatures_;
		std::vector<offset_entry> offset_signatures_;
	};
}

#define SETUP_POINTER(name) #name, reinterpret_cast<void**>(&name)
#define SETUP_MOD(chain) [](memory::scanned_result<void> r) { return r.chain; }
#define GRAB_CALL SETUP_MOD(add(1).rip())

#define SETUP_OFFSET(name) #name, &(name)
#define SETUP_OFFSET_MOD(chain) [](memory::scanned_result<void> r) -> std::uint32_t { return r.chain; }

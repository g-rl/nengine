#pragma once

#include <cstdio>
#include <cstdlib>
#include <functional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <utils/io.hpp>
#include <utils/nt.hpp>

#include "memory.hpp"
#include "scanned_result.hpp"

#include <game/game.hpp>
#include <identification/game.hpp>

namespace memory
{
	// Signature store: collect (name, pattern, pointer, mod) entries, then
	// scan_all() resolves each and writes the result into *pointer.
	// Results are cached on disk at neura/sig_cache.txt keyed by the game
	// binary's xxh32 and the (name, pattern) pair, so re-launches skip scanning.
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
			load_cache();

			scan_stats stats{};
			bool cache_dirty = false;

			for (auto& sig : signatures_)
			{
				stats.total++;
				if (*sig.pointer != nullptr)
				{
					stats.found++;
					continue;
				}

				const auto key = make_key('P', sig.name, sig.pattern);
				if (const auto it = cache_.find(key); it != cache_.end())
				{
					*sig.pointer = reinterpret_cast<void*>(game::base_address + it->second);
					stats.found++;
					continue;
				}

				auto r = sig_scan(library_, sig.pattern);
				if (r)
				{
					*sig.pointer = sig.mod(r).as<void*>();
					stats.found++;
					const auto offset = reinterpret_cast<std::uint64_t>(*sig.pointer) - game::base_address;
					cache_[key] = offset;
					cache_dirty = true;
					printf(("[sig] found: " + sig.name + " at 0x%llx\n").c_str(), offset);
				}
				else
				{
					printf(("[sig] MISS: " + sig.name + " (" + sig.pattern + ")\n").c_str());
				}
			}

			for (auto& sig : offset_signatures_)
			{
				stats.total++;

				const auto key = make_key('O', sig.name, sig.pattern);
				if (const auto it = cache_.find(key); it != cache_.end())
				{
					*sig.out = static_cast<std::uint32_t>(it->second);
					stats.found++;
					continue;
				}

				auto r = sig_scan(library_, sig.pattern);
				if (r)
				{
					*sig.out = sig.mod(r);
					stats.found++;
					cache_[key] = *sig.out;
					cache_dirty = true;
					printf(("[sig] offset: " + sig.name + " = 0x%x\n").c_str(), *sig.out);
				}
				else
				{
					printf(("[sig] MISS: " + sig.name + " (" + sig.pattern + ")\n").c_str());
				}
			}

			if (cache_dirty)
			{
				save_cache();
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

		static std::string make_key(char kind, const std::string& name, const std::string& pattern)
		{
			std::string k;
			k.reserve(name.size() + pattern.size() + 3);
			k.push_back(kind);
			k.push_back('|');
			k.append(name);
			k.push_back('|');
			k.append(pattern);
			return k;
		}

		static constexpr const char* cache_path_ = "neura/sig_cache.txt";

		void load_cache()
		{
			if (cache_loaded_) return;
			cache_loaded_ = true;
			cache_game_hash_ = identification::game::get_game_xxh32_checksum();

			std::string data;
			if (!utils::io::read_file(cache_path_, &data)) return;

			std::istringstream ss(data);
			std::string line;
			if (!std::getline(ss, line)) return;

			std::uint32_t stored_hash = 0;
			if (sscanf_s(line.c_str(), "v1 %x", &stored_hash) != 1) return;
			// Binary changed — drop the old cache; scan_all will rebuild it. a
			if (stored_hash != cache_game_hash_) return;

			while (std::getline(ss, line))
			{
				if (line.empty()) continue;
				const auto last = line.rfind('|');
				if (last == std::string::npos || last + 1 >= line.size()) continue;
				const std::string key = line.substr(0, last);
				const std::uint64_t value = std::strtoull(line.c_str() + last + 1, nullptr, 16);
				cache_[key] = value;
			}
		}

		void save_cache() const
		{
			std::string out;
			char header[64];
			_snprintf_s(header, _TRUNCATE, "v1 %08x\n", cache_game_hash_);
			out += header;
			for (const auto& [k, v] : cache_)
			{
				char tail[32];
				_snprintf_s(tail, _TRUNCATE, "|%llx\n", static_cast<unsigned long long>(v));
				out += k;
				out += tail;
			}
			utils::io::write_file(cache_path_, out);
		}

		utils::nt::library library_;
		std::vector<entry> signatures_;
		std::vector<offset_entry> offset_signatures_;

		bool cache_loaded_ = false;
		std::uint32_t cache_game_hash_ = 0;
		std::unordered_map<std::string, std::uint64_t> cache_;
	};
}

#define SETUP_POINTER(name) #name, reinterpret_cast<void**>(&name)
#define SETUP_MOD(chain) [](memory::scanned_result<void> r) { return r.chain; }
#define GRAB_CALL SETUP_MOD(add(1).rip())

#define SETUP_OFFSET(name) #name, &(name)
#define SETUP_OFFSET_MOD(chain) [](memory::scanned_result<void> r) -> std::uint32_t { return r.chain; }

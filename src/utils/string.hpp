#pragma once
#include "memory.hpp"
#include <cstdint>

#ifndef ARRAYSIZE
template <class Type, size_t n>
size_t ARRAYSIZE(Type (&)[n]) { return n; }
#endif

namespace utils::string
{
	template <size_t Buffers, size_t MinBufferSize>
	class va_provider final
	{
	public:
		static_assert(Buffers != 0 && MinBufferSize != 0, "Buffers and MinBufferSize mustn't be 0");

		va_provider() : current_buffer_(0)
		{
		}

		char* get(const char* format, const va_list ap)
		{
			++this->current_buffer_ %= ARRAYSIZE(this->string_pool_);
			auto entry = &this->string_pool_[this->current_buffer_];

			if (!entry->size || !entry->buffer)
			{
				throw std::runtime_error("String pool not initialized");
			}

			while (true)
			{
				const int res = vsnprintf_s(entry->buffer, entry->size, _TRUNCATE, format, ap);
				if (res > 0) break; // Success
				if (res == 0) return nullptr; // Error

				entry->double_size();
			}

			return entry->buffer;
		}

	private:
		class entry final
		{
		public:
			explicit entry(const size_t _size = MinBufferSize) : size(_size), buffer(nullptr)
			{
				if (this->size < MinBufferSize) this->size = MinBufferSize;
				this->allocate();
			}

			~entry()
			{
				if (this->buffer) memory::get_allocator()->free(this->buffer);
				this->size = 0;
				this->buffer = nullptr;
			}

			void allocate()
			{
				if (this->buffer) memory::get_allocator()->free(this->buffer);
				this->buffer = memory::get_allocator()->allocate_array<char>(this->size + 1);
			}

			void double_size()
			{
				this->size *= 2;
				this->allocate();
			}

			size_t size;
			char* buffer;
		};

		size_t current_buffer_;
		entry string_pool_[Buffers];
	};

	const char* va(const char* fmt, ...);

	std::vector<std::string> split(const std::string& s, char delim);

	std::string to_lower(std::string text);
	std::string to_upper(std::string text);
	bool starts_with(const std::string& text, const std::string& substring);
	bool ends_with(const std::string& text, const std::string& substring);

	std::string dump_hex(const std::string& data, const std::string& separator = " ");

	std::string get_clipboard_data();

	void strip(const char* in, char* out, int max);

	std::string convert(const std::wstring& wstr);
	std::wstring convert(const std::string& str);

	std::string replace(std::string str, const std::string& from, const std::string& to);

	bool match_compare(const std::string& input, const std::string& text, const bool exact);

	bool is_numeric(const std::string& text);

	constexpr inline std::uint32_t dvar_checksum(const char* str, std::size_t length)
	{
		if (length < 1 || length > 1024)
		{
			return 0xFFFFFFFF;
		}

		std::size_t total_len = length + 1;

		std::uint8_t buffer[1024] = { 0 };
		for (std::size_t i = 0; i < total_len; ++i)
		{
			char ch_ = str[i];
			if (ch_ >= 'A' && ch_ <= 'Z')
			{
				ch_ |= 0x20;
			}
			buffer[i] = static_cast<std::uint8_t>(ch_);
		}

		std::uint32_t hash = 0xDEADDEAD;
		for (std::size_t i = 0; buffer[i]; ++i)
		{
			hash = buffer[i] ^ (hash * 0x1000193);
		}

		return hash;
	}

	constexpr inline std::uint32_t dvar_checksum(std::string_view dvar_name)
	{
		return dvar_checksum(dvar_name.data(), dvar_name.length());
	}

	constexpr inline std::uint32_t operator""_dc(const char* s, std::size_t n)
	{
		return dvar_checksum(s, n);
	}
}

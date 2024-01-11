#pragma once

namespace command
{
	void add_raw(const char* name, void (*callback)());
	void add(const char* name, const std::function<void()>& callback);
}

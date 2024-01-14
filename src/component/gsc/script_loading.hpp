#pragma once
#include <xsk/gsc/engine/iw8.hpp>

namespace gsc
{
	extern std::unique_ptr<xsk::gsc::iw8::context> gsc_ctx;

	game::ScriptFile* find_script(game::XAssetType type, const char* name, int allow_create_default);

	void on_begin_scripts(const std::function<void()>& callback);
}

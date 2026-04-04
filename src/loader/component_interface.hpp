#pragma once

#include <memory/signature_store.hpp>

class component_interface
{
public:
	virtual ~component_interface()
	{
	}

	virtual void find_signatures(memory::signature_store& /*batch*/)
	{
	}

	virtual void post_start()
	{
	}

	virtual void post_load()
	{
	}

	virtual void pre_destroy()
	{
	}

	virtual void post_unpack()
	{
	}

	virtual void* load_import([[maybe_unused]] const std::string& library, [[maybe_unused]] const std::string& function)
	{
		return nullptr;
	}

	virtual bool is_supported()
	{
		return true;
	}
};

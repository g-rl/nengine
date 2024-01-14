#include <std_include.hpp>
#include "stack_isolation.hpp"

namespace scripting
{
	stack_isolation::stack_isolation()
	{
		const auto context = game::ScriptContext_Server();
		this->in_param_count_ = context->inparamcount;
		this->out_param_count_ = context->outparamcount;
		this->top_ = context->top;
		this->max_stack_ = context->maxstack;

		context->top = this->stack_;
		context->maxstack = &this->stack_[ARRAYSIZE(this->stack_) - 1];
		context->inparamcount = 0;
		context->outparamcount = 0;
	}

	stack_isolation::~stack_isolation()
	{
		const auto context = game::ScriptContext_Server();
		game::Scr_ClearOutParams(context);
		context->inparamcount = this->in_param_count_;
		context->outparamcount = this->out_param_count_;
		context->top = this->top_;
		context->maxstack = this->max_stack_;
	}
}

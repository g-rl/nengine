#include <std_include.hpp>

#include "call_spoofer.hpp"

#include "game/game.hpp"
#include <identification/game.hpp>
#include "loader/component_loader.hpp"

#include <utils/hook.hpp>

namespace call_spoofer {
	class component final : public component_interface
	{
		void post_load() override
		{
			auto* stub = utils::hook::assemble([](utils::hook::assembler& a) {
				auto fixup = a.newLabel();

				a.pop(r11);							// popping without setting up stack frame, r11 is the return address (the one in our code)
				a.add(rsp, 8);						// skipping caller reserved space
				a.mov(rax, qword_ptr(rsp, 0x18));	// dereference shell_param

				a.mov(r10, qword_ptr(rax));			// load shell_param.trampoline
				a.mov(qword_ptr(rsp), r10);			// store address of trampoline as return address

				a.mov(r10, qword_ptr(rax, 0x08));	// load shell_param.function
				a.mov(qword_ptr(rax, 0x08), r11);	// store the original return address in shell_param.function

				a.mov(qword_ptr(rax, 0x10), rdi);	// preserve rdi in shell_param.rdi
				a.lea(rdi, ptr(fixup));
				a.mov(qword_ptr(rax), rdi);			// store address of fixup label in shell_param.trampoline
				a.mov(rdi, rax);					// preserve address of shell_param in rdi

				a.jmp(r10);							// call shell_param.function

				a.bind(fixup);
				a.sub(rsp, 0x10);
				a.mov(rcx, rdi);					// restore address of shell_param
				a.mov(rdi, qword_ptr(rcx, 0x10));	// restore rdi from shell_param.rdi
				a.jmp(qword_ptr(rcx, 0x08));		// jmp to the original return address
			});

			spoofer_stub = stub;
		}

		void find_signatures(memory::signature_store& batch) override {
			batch.add(SETUP_POINTER(spoof_trampoline), "FF 27");
		}
	};
}

REGISTER_COMPONENT(call_spoofer::component)

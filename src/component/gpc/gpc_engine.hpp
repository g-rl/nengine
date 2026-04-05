#pragma once

#include "gpc_lexer.hpp"
#include "gpc_parser.hpp"
#include "gpc_vm.hpp"

#include <string>
#include <vector>
#include <memory>
#include <mutex>

// XInput struct/constant definitions (avoiding #include <Xinput.h> which
// declares dllimport functions that conflict with our dllexport proxies)
#ifndef XINPUT_GAMEPAD_DPAD_UP
#define XINPUT_GAMEPAD_DPAD_UP          0x0001
#define XINPUT_GAMEPAD_DPAD_DOWN        0x0002
#define XINPUT_GAMEPAD_DPAD_LEFT        0x0004
#define XINPUT_GAMEPAD_DPAD_RIGHT       0x0008
#define XINPUT_GAMEPAD_START            0x0010
#define XINPUT_GAMEPAD_BACK             0x0020
#define XINPUT_GAMEPAD_LEFT_THUMB       0x0040
#define XINPUT_GAMEPAD_RIGHT_THUMB      0x0080
#define XINPUT_GAMEPAD_LEFT_SHOULDER    0x0100
#define XINPUT_GAMEPAD_RIGHT_SHOULDER   0x0200
#define XINPUT_GAMEPAD_A                0x1000
#define XINPUT_GAMEPAD_B                0x2000
#define XINPUT_GAMEPAD_X                0x4000
#define XINPUT_GAMEPAD_Y                0x8000

struct XINPUT_GAMEPAD
{
	WORD  wButtons;
	BYTE  bLeftTrigger;
	BYTE  bRightTrigger;
	SHORT sThumbLX;
	SHORT sThumbLY;
	SHORT sThumbRX;
	SHORT sThumbRY;
};

struct XINPUT_STATE
{
	DWORD          dwPacketNumber;
	XINPUT_GAMEPAD Gamepad;
};

struct XINPUT_VIBRATION
{
	WORD wLeftMotorSpeed;
	WORD wRightMotorSpeed;
};

struct XINPUT_CAPABILITIES
{
	BYTE             Type;
	BYTE             SubType;
	WORD             Flags;
	XINPUT_GAMEPAD   Gamepad;
	XINPUT_VIBRATION Vibration;
};
#endif

namespace gpc
{
	struct loaded_script
	{
		std::string filename;
		std::string source;
		std::vector<token> tokens;
		std::unique_ptr<program_node> ast;
		std::unique_ptr<vm> runtime;
		bool active = false;
		std::filesystem::file_time_type last_modified{};
	};

	class engine
	{
	public:
		static engine& instance();

		void initialize(const std::string& scripts_dir);
		void shutdown();

		// called from XInputGetState hook
		void on_xinput_get_state(DWORD user_index, XINPUT_STATE* state);

		// reload all scripts
		void reload();

		// list loaded scripts
		std::vector<std::string> get_loaded_scripts() const;

	private:
		engine() = default;

		mutable std::mutex mutex_;
		std::string scripts_dir_;
		std::vector<std::unique_ptr<loaded_script>> scripts_;
		bool initialized_ = false;

		void load_scripts();
		void load_script_file(const std::string& path);
		void check_for_updates();

		// xinput value mapping
		static void xinput_state_to_vm(const XINPUT_STATE* state, vm& runtime);
		static void vm_to_xinput_state(const vm& runtime, XINPUT_STATE* state);
	};
}

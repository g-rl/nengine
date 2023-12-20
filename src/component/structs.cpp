#include <std_include.hpp>

#include "game/game.hpp"
#include "game/structs.hpp"

namespace game
{
	void XUID::Deserialize(const msg_t* msg) {
		this->m_id = game::MSG_ReadInt64(msg);
	}

	void XUID::Serialize(const msg_t* msg) {
		MSG_WriteInt64(msg, this->m_id);
	}

	XUID* XUID::FromHexString(const char* xuidString) {
		this->m_id = I_atoui64_hex(xuidString);
		return this;
	}

	XUID* XUID::FromMsg(const msg_t* msg) {
		this->m_id = MSG_ReadInt64(msg);
		return this;
	}

	XUID* XUID::FromString(const char* xuidString) {
		this->m_id = I_atoui64(xuidString);
		return this;
	}

	XUID* XUID::FromUInt64(unsigned __int64 id) {
		this->m_id = id;
		return this;
	}

	XUID* XUID::FromUniversalId(unsigned __int64 id) {
		this->m_id = id;
		return this;
	}

	unsigned __int64 XUID::GetUniversalId() {
		return this->m_id;
	}

	bool XUID::IsNull() {
		return this->m_id == 0;
	}

	bool XUID::IsValid() {
		return this->m_id != 0;
	}

	XUID* XUID::NullXUID() {
		this->m_id = 0;
		return this;
	}

	XUID* XUID::RandomXUID() {
		unsigned int* RandSeed;
		unsigned int BackupRandSeed;
		this->m_id = 0;
		RandSeed = GetRandSeed();
		BackupRandSeed = *RandSeed;
		*RandSeed = Sys_Microseconds();
		this->m_id = I_irand(1, 0x7FFFFFFF);
		*RandSeed = BackupRandSeed;
		return this;
	}

	const char* XUID::ToDevString() {
		return va("%zu", this->m_id);
	}

	const char* XUID::ToHexString() {
		return va("%zx", this->m_id);
	}

	const char* XUID::ToString() {
		return va("%zu", this->m_id);
	}

	bool XUID::operator !=(const XUID* xuid) {
		return this->m_id != xuid->m_id;
	}

	XUID* XUID::operator =(const XUID* xuid) {
		this->m_id = xuid->m_id;
		return this;
	}

	bool XUID::operator ==(const XUID* xuid) {
		return this->m_id == xuid->m_id;
	}
}
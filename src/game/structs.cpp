#include <std_include.hpp>

#include "game.hpp"

namespace game
{
	void XUID::deserialize(const msg_t* msg) {
		this->m_id = MSG_ReadInt64(msg);
	}

	void XUID::serialize(const msg_t* msg) {
		MSG_WriteInt64(msg, this->m_id);
	}

	unsigned __int64 XUID::get_id() {
		return this->m_id;
	}

	XUID* XUID::random_xuid() {
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
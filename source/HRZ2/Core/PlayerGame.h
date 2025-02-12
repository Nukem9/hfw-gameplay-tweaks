#pragma once

#include "Player.h"

namespace HRZ2
{
	class PlayerGame : public Player
	{
	public:
		char _pad0[0xC8];
		bool m_RestartOnSpawned; // 0x1F0
	};
	assert_offset(PlayerGame, m_RestartOnSpawned, 0x1F0);
}

#pragma once

namespace Wait
{
	enum class Phase : int { kIdle = 0, kActive = 1, kReleasing = 2 };

	enum class Block : int { kNone = 0, kMenu, kLoading, kDead, kCombat, kKillMove, kRecentlyHit };

	struct Status
	{
		Phase phase;
		Block block;
		float multiplier;     // 1.0 when idle
		float baseTimescale;  // the timescale to go back to
		float timescale;      // what the game is running at now
	};

	// call once the game data is loaded (kDataLoaded)
	void Init();

	// thread safe
	Status GetStatus();

	// back to the normal timescale at once; a_restore = false leaves the value alone (a new game / load replaces it anyway)
	void Cancel(bool a_restore);

	// co-save: a save written while time is sped up remembers the timescale to go back to
	void Save(SKSE::SerializationInterface* a_intfc);
	void Load(SKSE::SerializationInterface* a_intfc);
	void OnPostLoadGame();
	void OnRevert();

	// call after the settings changed; wakes the update loop if a binding is held
	void SettingsChanged();

	// the hotkey the player pressed most recently, for the settings page
	const char* BlockName(Block a_block);
}

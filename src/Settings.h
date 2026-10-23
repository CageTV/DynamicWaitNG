#pragma once

// Settings live in Data/SKSE/Plugins/DynamicWaitNG.ini. The first four keys are the ones of the original
// Dynamic Wait SKSE (DynamicWait.ini), which is read once when there is no DynamicWaitNG.ini yet.
struct Settings
{
	enum Mode : int { kHold = 0, kToggle = 1 };

	// Binding. Codes: keyboard = DirectInput scan code, mouse = 256 + button, gamepad = 266..281 (Skyrim's own numbering).
	std::uint32_t hotkey{ 40 };           // apostrophe
	std::uint32_t hotkey2{ 0 };           // second binding, e.g. a gamepad button (0 = none)
	std::uint32_t modifier{ 0 };          // key that has to be held as well (0 = none)
	int           mode{ kHold };

	// Speed
	float waitMultiplier{ 2.0f };         // how much faster time passes
	bool  rampUp{ true };                 // grow from waitMultiplier to clampMultiplier while held
	float clampMultiplier{ 10.0f };
	float rampSeconds{ 6.0f };            // real seconds from waitMultiplier to clampMultiplier
	float releaseSeconds{ 0.4f };         // real seconds to slow back down after letting go (0 = instantly)

	// Safety. Each of these stops the speed-up at once, without the slow-down.
	bool  blockInCombat{ true };
	bool  blockWhenHit{ true };
	float hitCooldownSeconds{ 3.0f };
	bool  blockInMenus{ true };           // any menu that pauses the game, dialogue, loading
	bool  blockWhenDead{ true };

	// HUD (needs the SKSE Menu Framework)
	bool  showHud{ true };
	float hudX{ 0.5f };                   // fraction of the screen width
	float hudY{ 0.06f };                  // fraction of the screen height
	float hudScale{ 1.4f };

	bool  debugLog{ false };

	static Settings& Get();
	static constexpr const char* kPath = "Data/SKSE/Plugins/DynamicWaitNG.ini";
	void Load();
	void Save() const;
	void Clamp();
};

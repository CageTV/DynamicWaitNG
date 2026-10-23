#include "Settings.h"

Settings& Settings::Get()
{
	static Settings instance;
	return instance;
}

void Settings::Clamp()
{
	waitMultiplier = std::clamp(waitMultiplier, 1.0f, 1000.0f);
	clampMultiplier = std::clamp(clampMultiplier, waitMultiplier, 1000.0f);
	rampSeconds = std::clamp(rampSeconds, 0.1f, 600.0f);
	releaseSeconds = std::clamp(releaseSeconds, 0.0f, 10.0f);
	hitCooldownSeconds = std::clamp(hitCooldownSeconds, 0.0f, 60.0f);
	hudX = std::clamp(hudX, 0.0f, 1.0f);
	hudY = std::clamp(hudY, 0.0f, 1.0f);
	hudScale = std::clamp(hudScale, 0.5f, 5.0f);
	mode = std::clamp(mode, 0, 1);
}

void Settings::Load()
{
	CSimpleIniA ini;
	ini.SetUnicode();

	bool legacy = false;
	if (ini.LoadFile(kPath) < 0) {
		// first run: take over the original Dynamic Wait SKSE's ini when it is there
		if (ini.LoadFile("Data/SKSE/Plugins/DynamicWait.ini") >= 0) {
			legacy = true;
		}
	}

	const auto getBool = [&](const char* s, const char* k, bool& v) { v = ini.GetBoolValue(s, k, v); };
	const auto getFloat = [&](const char* s, const char* k, float& v) { v = static_cast<float>(ini.GetDoubleValue(s, k, v)); };
	const auto getUInt = [&](const char* s, const char* k, std::uint32_t& v) { v = static_cast<std::uint32_t>(ini.GetLongValue(s, k, static_cast<long>(v))); };

	getUInt("General", "uWaitHotkey", hotkey);
	getFloat("General", "fWaitMultiplier", waitMultiplier);
	getBool("General", "bRampUpMultiplier", rampUp);
	getFloat("General", "fClampMultiplier", clampMultiplier);

	if (!legacy) {
		getUInt("General", "uWaitHotkey2", hotkey2);
		getUInt("General", "uModifierHotkey", modifier);
		mode = static_cast<int>(ini.GetLongValue("General", "iMode", mode));
		getFloat("General", "fRampSeconds", rampSeconds);
		getFloat("General", "fReleaseSeconds", releaseSeconds);

		getBool("Safety", "bBlockInCombat", blockInCombat);
		getBool("Safety", "bBlockWhenHit", blockWhenHit);
		getFloat("Safety", "fHitCooldownSeconds", hitCooldownSeconds);
		getBool("Safety", "bBlockInMenus", blockInMenus);
		getBool("Safety", "bBlockWhenDead", blockWhenDead);

		getBool("HUD", "bShowHud", showHud);
		getFloat("HUD", "fHudX", hudX);
		getFloat("HUD", "fHudY", hudY);
		getFloat("HUD", "fHudScale", hudScale);
	}
	getBool("Log", "Debug", debugLog);

	Clamp();
}

void Settings::Save() const
{
	CSimpleIniA ini;
	ini.SetUnicode();
	ini.LoadFile(kPath);  // keep whatever else is in there

	ini.SetValue("General", nullptr, nullptr, "; uWaitHotkey / uWaitHotkey2 / uModifierHotkey: DirectInput scan code (apostrophe = 40), mouse button = 256 + button, gamepad button = 266..281 (LB = 274, RB = 275, LT = 280, RT = 281)");
	ini.SetLongValue("General", "uWaitHotkey", hotkey);
	ini.SetLongValue("General", "uWaitHotkey2", hotkey2);
	ini.SetLongValue("General", "uModifierHotkey", modifier);
	ini.SetLongValue("General", "iMode", mode, "; 0 = hold the key, 1 = press once to start and again to stop");
	ini.SetDoubleValue("General", "fWaitMultiplier", waitMultiplier);
	ini.SetBoolValue("General", "bRampUpMultiplier", rampUp);
	ini.SetDoubleValue("General", "fClampMultiplier", clampMultiplier);
	ini.SetDoubleValue("General", "fRampSeconds", rampSeconds);
	ini.SetDoubleValue("General", "fReleaseSeconds", releaseSeconds);

	ini.SetBoolValue("Safety", "bBlockInCombat", blockInCombat);
	ini.SetBoolValue("Safety", "bBlockWhenHit", blockWhenHit);
	ini.SetDoubleValue("Safety", "fHitCooldownSeconds", hitCooldownSeconds);
	ini.SetBoolValue("Safety", "bBlockInMenus", blockInMenus);
	ini.SetBoolValue("Safety", "bBlockWhenDead", blockWhenDead);

	ini.SetBoolValue("HUD", "bShowHud", showHud);
	ini.SetDoubleValue("HUD", "fHudX", hudX);
	ini.SetDoubleValue("HUD", "fHudY", hudY);
	ini.SetDoubleValue("HUD", "fHudScale", hudScale);

	ini.SetBoolValue("Log", "Debug", debugLog);

	std::error_code ec;
	std::filesystem::create_directories(std::filesystem::path(kPath).parent_path(), ec);
	if (ini.SaveFile(kPath) < 0) {
		logger::error("Could not write {}", kPath);
	}
}

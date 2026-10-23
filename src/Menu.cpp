#include "Menu.h"

#include "Settings.h"
#include "Wait.h"

#pragma warning(push)
#pragma warning(disable : 4099 5054)
#include "SKSEMenuFramework.h"
#pragma warning(pop)

namespace Menu
{
	namespace
	{
		using namespace ImGuiMCP;

		bool dirty = false;

		// DirectInput scan code <-> ImGui key (the game's hotkeys are scan codes)
		struct KeyPair { std::uint32_t scan; ImGuiKey key; };
		const KeyPair keyTable[] = {
			{ 1, ImGuiKey_Escape }, { 2, ImGuiKey_1 }, { 3, ImGuiKey_2 }, { 4, ImGuiKey_3 }, { 5, ImGuiKey_4 },
			{ 6, ImGuiKey_5 }, { 7, ImGuiKey_6 }, { 8, ImGuiKey_7 }, { 9, ImGuiKey_8 }, { 10, ImGuiKey_9 },
			{ 11, ImGuiKey_0 }, { 12, ImGuiKey_Minus }, { 13, ImGuiKey_Equal }, { 14, ImGuiKey_Backspace },
			{ 15, ImGuiKey_Tab }, { 16, ImGuiKey_Q }, { 17, ImGuiKey_W }, { 18, ImGuiKey_E }, { 19, ImGuiKey_R },
			{ 20, ImGuiKey_T }, { 21, ImGuiKey_Y }, { 22, ImGuiKey_U }, { 23, ImGuiKey_I }, { 24, ImGuiKey_O },
			{ 25, ImGuiKey_P }, { 26, ImGuiKey_LeftBracket }, { 27, ImGuiKey_RightBracket }, { 28, ImGuiKey_Enter },
			{ 29, ImGuiKey_LeftCtrl }, { 30, ImGuiKey_A }, { 31, ImGuiKey_S }, { 32, ImGuiKey_D }, { 33, ImGuiKey_F },
			{ 34, ImGuiKey_G }, { 35, ImGuiKey_H }, { 36, ImGuiKey_J }, { 37, ImGuiKey_K }, { 38, ImGuiKey_L },
			{ 39, ImGuiKey_Semicolon }, { 40, ImGuiKey_Apostrophe }, { 41, ImGuiKey_GraveAccent },
			{ 42, ImGuiKey_LeftShift }, { 43, ImGuiKey_Backslash }, { 44, ImGuiKey_Z }, { 45, ImGuiKey_X },
			{ 46, ImGuiKey_C }, { 47, ImGuiKey_V }, { 48, ImGuiKey_B }, { 49, ImGuiKey_N }, { 50, ImGuiKey_M },
			{ 51, ImGuiKey_Comma }, { 52, ImGuiKey_Period }, { 53, ImGuiKey_Slash }, { 54, ImGuiKey_RightShift },
			{ 55, ImGuiKey_KeypadMultiply }, { 56, ImGuiKey_LeftAlt }, { 57, ImGuiKey_Space }, { 58, ImGuiKey_CapsLock },
			{ 59, ImGuiKey_F1 }, { 60, ImGuiKey_F2 }, { 61, ImGuiKey_F3 }, { 62, ImGuiKey_F4 }, { 63, ImGuiKey_F5 },
			{ 64, ImGuiKey_F6 }, { 65, ImGuiKey_F7 }, { 66, ImGuiKey_F8 }, { 67, ImGuiKey_F9 }, { 68, ImGuiKey_F10 },
			{ 69, ImGuiKey_NumLock }, { 70, ImGuiKey_ScrollLock }, { 71, ImGuiKey_Keypad7 }, { 72, ImGuiKey_Keypad8 },
			{ 73, ImGuiKey_Keypad9 }, { 74, ImGuiKey_KeypadSubtract }, { 75, ImGuiKey_Keypad4 }, { 76, ImGuiKey_Keypad5 },
			{ 77, ImGuiKey_Keypad6 }, { 78, ImGuiKey_KeypadAdd }, { 79, ImGuiKey_Keypad1 }, { 80, ImGuiKey_Keypad2 },
			{ 81, ImGuiKey_Keypad3 }, { 82, ImGuiKey_Keypad0 }, { 83, ImGuiKey_KeypadDecimal }, { 87, ImGuiKey_F11 },
			{ 88, ImGuiKey_F12 }, { 156, ImGuiKey_KeypadEnter }, { 157, ImGuiKey_RightCtrl }, { 181, ImGuiKey_KeypadDivide },
			{ 183, ImGuiKey_PrintScreen }, { 184, ImGuiKey_RightAlt }, { 197, ImGuiKey_Pause }, { 199, ImGuiKey_Home },
			{ 200, ImGuiKey_UpArrow }, { 201, ImGuiKey_PageUp }, { 203, ImGuiKey_LeftArrow }, { 205, ImGuiKey_RightArrow },
			{ 207, ImGuiKey_End }, { 208, ImGuiKey_DownArrow }, { 209, ImGuiKey_PageDown }, { 210, ImGuiKey_Insert },
			{ 211, ImGuiKey_Delete },
		};

		ImGuiKey ScanToKey(std::uint32_t a_scan)
		{
			for (const auto& p : keyTable) if (p.scan == a_scan) return p.key;
			return ImGuiKey_None;
		}

		std::uint32_t KeyToScan(ImGuiKey a_key)
		{
			for (const auto& p : keyTable) if (p.key == a_key) return p.scan;
			return 0;
		}

		// mouse and gamepad buttons, in the numbering the settings use
		struct ButtonPair { std::uint32_t code; const char* name; };
		const ButtonPair buttons[] = {
			{ 0, "None" },
			{ 258, "Mouse middle button" }, { 259, "Mouse button 4" }, { 260, "Mouse button 5" }, { 261, "Mouse button 6" },
			{ 266, "Gamepad D-pad up" }, { 267, "Gamepad D-pad down" }, { 268, "Gamepad D-pad left" }, { 269, "Gamepad D-pad right" },
			{ 270, "Gamepad Start" }, { 271, "Gamepad Back" }, { 272, "Gamepad left stick click" }, { 273, "Gamepad right stick click" },
			{ 274, "Gamepad LB" }, { 275, "Gamepad RB" }, { 276, "Gamepad A" }, { 277, "Gamepad B" }, { 278, "Gamepad X" },
			{ 279, "Gamepad Y" }, { 280, "Gamepad LT" }, { 281, "Gamepad RT" },
		};
		constexpr int buttonCount = sizeof(buttons) / sizeof(buttons[0]);

		bool KeyRow(const char* a_label, std::uint32_t& a_code)
		{
			ImGuiKey key = ScanToKey(a_code);
			if (ImGuiMCPComponents::KeyBindingPicker(a_label, &key)) {
				a_code = KeyToScan(key);
				return true;
			}
			if (key == ImGuiKey_None && a_code != 0) {
				SameLine();
				TextDisabled("(code %u)", a_code);
			}
			return false;
		}

		bool ButtonRow(const char* a_label, std::uint32_t& a_code)
		{
			int index = 0;
			for (int i = 0; i < buttonCount; i++) {
				if (buttons[i].code == a_code) { index = i; break; }
			}
			bool changed = false;
			if (BeginCombo(a_label, buttons[index].name)) {
				for (int i = 0; i < buttonCount; i++) {
					if (Selectable(buttons[i].name, i == index)) {
						a_code = buttons[i].code;
						changed = true;
					}
				}
				EndCombo();
			}
			return changed;
		}

		void Tip(const char* a_text)
		{
			if (IsItemHovered()) {
				SetTooltip("%s", a_text);
			}
		}

		void __stdcall OnMenuEvent(SKSEMenuFramework::Model::EventType a_type)
		{
			if (a_type == SKSEMenuFramework::Model::kCloseMenu && dirty) {
				Settings::Get().Clamp();
				Settings::Get().Save();
				Wait::SettingsChanged();
				dirty = false;
			}
		}

		void __stdcall RenderSettings()
		{
			auto& s = Settings::Get();
			bool  c = false;

			SeparatorText("Hotkey");
			c |= KeyRow("Keyboard key", s.hotkey);
			Tip("The default is the apostrophe key. Clear it to use only the alternative button.");
			c |= ButtonRow("Alternative button", s.hotkey2);
			Tip("A mouse or gamepad button that works the same as the keyboard key.");
			c |= KeyRow("Also hold (modifier)", s.modifier);
			Tip("Optional: the speed-up only starts while this key is held as well. Useful on a key that is used for other things.");
			c |= Combo("Mode", &s.mode, "Hold the key\0Press to start, press again to stop\0");

			SeparatorText("Speed");
			c |= SliderFloat("Speed-up", &s.waitMultiplier, 1.0f, 100.0f, "x%.1f", ImGuiSliderFlags_Logarithmic);
			Tip("How many times faster time passes. It is applied to your normal timescale, so a mod that changed the timescale is respected.");
			c |= Checkbox("Speed up gradually while held", &s.rampUp);
			if (s.rampUp) {
				c |= SliderFloat("Up to", &s.clampMultiplier, s.waitMultiplier, 200.0f, "x%.1f", ImGuiSliderFlags_Logarithmic);
				c |= SliderFloat("Seconds to get there", &s.rampSeconds, 0.5f, 60.0f, "%.1f s");
			}
			c |= SliderFloat("Slow down after letting go", &s.releaseSeconds, 0.0f, 3.0f, "%.2f s");
			Tip("0 = back to normal speed at once.");

			SeparatorText("Safety");
			c |= Checkbox("Stop in combat", &s.blockInCombat);
			c |= Checkbox("Stop when I get hit", &s.blockWhenHit);
			if (s.blockWhenHit) {
				c |= SliderFloat("Wait after a hit", &s.hitCooldownSeconds, 0.0f, 15.0f, "%.1f s");
			}
			c |= Checkbox("Stop in menus, dialogue and loading", &s.blockInMenus);
			c |= Checkbox("Stop when dead", &s.blockWhenDead);

			SeparatorText("On-screen indicator");
			c |= Checkbox("Show the current speed", &s.showHud);
			if (s.showHud) {
				c |= SliderFloat("Horizontal position", &s.hudX, 0.0f, 1.0f, "%.2f");
				c |= SliderFloat("Vertical position", &s.hudY, 0.0f, 1.0f, "%.2f");
				c |= SliderFloat("Size", &s.hudScale, 0.5f, 4.0f, "%.1f");
			}

			SeparatorText("Now");
			const auto st = Wait::GetStatus();
			Text("Timescale %.1f (normal %.1f)", st.timescale, st.baseTimescale);
			Text("Speed-up: %s", st.phase == Wait::Phase::kIdle ? "off" : (st.phase == Wait::Phase::kActive ? "on" : "slowing down"));
			if (st.block != Wait::Block::kNone) {
				TextDisabled("Blocked: %s", Wait::BlockName(st.block));
			}

			Spacing();
			if (Button("Save now")) {
				s.Clamp();
				s.Save();
				Wait::SettingsChanged();
				dirty = false;
			}
			SameLine();
			if (Button("Reload from disk")) {
				s.Load();
				Wait::SettingsChanged();
				dirty = false;
			}
			SameLine();
			if (Button("Reset to defaults")) {
				const bool debug = s.debugLog;
				s = Settings{};
				s.debugLog = debug;
				c = true;
			}
			TextDisabled(dirty ? "Unsaved changes. They are saved when this menu closes." : "Saved to Data/SKSE/Plugins/DynamicWaitNG.ini");
			dirty |= c;
		}

		void __stdcall RenderHud()
		{
			const auto& s = Settings::Get();
			if (!s.showHud) {
				return;
			}
			const auto st = Wait::GetStatus();
			if (st.phase == Wait::Phase::kIdle) {
				return;
			}
			const auto io = GetIO();
			if (!io) {
				return;
			}
			char text[48];
			std::snprintf(text, sizeof(text), ">> x%.1f", st.multiplier);

			const float fontSize = 18.0f * s.hudScale;
			const auto  draw = GetForegroundDrawList();
			const ImVec2 size = CalcTextSize(text);
			const float  scale = fontSize / 18.0f;
			const ImVec2 pos{ io->DisplaySize.x * s.hudX - size.x * scale * 0.5f, io->DisplaySize.y * s.hudY };
			// a dark outline keeps it readable over snow and sky
			const ImU32 shadow = IM_COL32(0, 0, 0, 200);
			const ImU32 fill = IM_COL32(255, 232, 170, 235);
			ImDrawListManager::AddText(draw, nullptr, fontSize, ImVec2{ pos.x + 1.5f, pos.y + 1.5f }, shadow, text);
			ImDrawListManager::AddText(draw, nullptr, fontSize, pos, fill, text);
		}
	}

	void Register()
	{
		if (!SKSEMenuFramework::IsInstalled()) {
			logger::info("SKSE Menu Framework is not installed, the settings stay in the ini");
			return;
		}
		SKSEMenuFramework::SetSection("Dynamic Wait");
		SKSEMenuFramework::AddSectionItem("Settings", RenderSettings);
		SKSEMenuFramework::AddEvent(OnMenuEvent, 0.0f);  // never freed on purpose, like the framework's own examples
		SKSEMenuFramework::AddHudElement(RenderHud);
		logger::info("Registered the settings page and the indicator with the SKSE Menu Framework");
	}
}

#include "Wait.h"

#include "Settings.h"

namespace Wait
{
	namespace
	{
		using Clock = std::chrono::steady_clock;

		// ---- state (main thread) ----
		Phase        phase{ Phase::kIdle };
		float        baseScale{ 20.0f };
		float        lastWritten{ 0.0f };
		float        currentMult{ 1.0f };
		float        releaseFrom{ 1.0f };
		Clock::time_point rampStart{};
		Clock::time_point releaseStart{};
		Clock::time_point hitUntil{};
		bool         toggled{ false };
		std::set<std::uint32_t> held;

		// restore after loading a save that was written while sped up
		bool         pendingRestore{ false };
		float        pendingBase{ 0.0f };

		// ---- shared with the update thread and the HUD ----
		std::atomic<bool>  wake{ false };
		std::atomic<bool>  tickQueued{ false };
		std::atomic<int>   sharedPhase{ 0 };
		std::atomic<int>   sharedBlock{ 0 };
		std::atomic<float> sharedMult{ 1.0f };
		std::atomic<float> sharedBase{ 20.0f };
		std::atomic<float> sharedScale{ 20.0f };

		constexpr std::uint32_t kRecordType = 'DWST';
		constexpr std::uint32_t kRecordVersion = 1;

		RE::TESGlobal* TimeScale()
		{
			const auto calendar = RE::Calendar::GetSingleton();
			return calendar ? calendar->timeScale : nullptr;
		}

		// Skyrim's own numbering: keyboard scan code, mouse 256 + button, gamepad 266 .. 281
		std::uint32_t GamepadCode(std::uint32_t a_mask)
		{
			using Key = RE::BSWin32GamepadDevice::Key;
			switch (static_cast<Key>(a_mask)) {
			case Key::kUp: return 266;
			case Key::kDown: return 267;
			case Key::kLeft: return 268;
			case Key::kRight: return 269;
			case Key::kStart: return 270;
			case Key::kBack: return 271;
			case Key::kLeftThumb: return 272;
			case Key::kRightThumb: return 273;
			case Key::kLeftShoulder: return 274;
			case Key::kRightShoulder: return 275;
			case Key::kA: return 276;
			case Key::kB: return 277;
			case Key::kX: return 278;
			case Key::kY: return 279;
			case Key::kLeftTrigger: return 280;
			case Key::kRightTrigger: return 281;
			default: return 0;
			}
		}

		std::uint32_t ButtonCode(const RE::ButtonEvent* a_event)
		{
			switch (a_event->GetDevice()) {
			case RE::INPUT_DEVICE::kKeyboard: return a_event->GetIDCode();
			case RE::INPUT_DEVICE::kMouse: return 256 + a_event->GetIDCode();
			case RE::INPUT_DEVICE::kGamepad: return GamepadCode(a_event->GetIDCode());
			default: return 0;
			}
		}

		bool BindingHeld()
		{
			const auto& s = Settings::Get();
			if (s.modifier != 0 && !held.contains(s.modifier)) {
				return false;
			}
			return (s.hotkey != 0 && held.contains(s.hotkey)) || (s.hotkey2 != 0 && held.contains(s.hotkey2));
		}

		bool WantActive()
		{
			return Settings::Get().mode == Settings::kToggle ? toggled : BindingHeld();
		}

		Block CurrentBlock(Clock::time_point a_now)
		{
			const auto& s = Settings::Get();
			const auto player = RE::PlayerCharacter::GetSingleton();
			if (!player || !player->Is3DLoaded()) {
				return Block::kLoading;
			}
			if (s.blockInMenus) {
				if (const auto ui = RE::UI::GetSingleton()) {
					if (ui->GameIsPaused() || ui->IsMenuOpen(RE::DialogueMenu::MENU_NAME) || ui->IsMenuOpen(RE::LoadingMenu::MENU_NAME) ||
						ui->IsMenuOpen(RE::SleepWaitMenu::MENU_NAME) || ui->IsMenuOpen(RE::FaderMenu::MENU_NAME)) {
						return Block::kMenu;
					}
				}
			}
			if (s.blockWhenDead && player->IsDead()) {
				return Block::kDead;
			}
			if (player->IsInKillMove()) {
				return Block::kKillMove;
			}
			if (s.blockInCombat && player->IsInCombat()) {
				return Block::kCombat;
			}
			if (s.blockWhenHit && a_now < hitUntil) {
				return Block::kRecentlyHit;
			}
			return Block::kNone;
		}

		float RampMultiplier(float a_seconds)
		{
			const auto& s = Settings::Get();
			if (!s.rampUp || s.clampMultiplier <= s.waitMultiplier) {
				return s.waitMultiplier;
			}
			const float t = std::clamp(a_seconds / s.rampSeconds, 0.0f, 1.0f);
			return s.waitMultiplier + (s.clampMultiplier - s.waitMultiplier) * t;
		}

		// seconds into the ramp at which it is at a_mult, so that pressing again during the slow-down carries on from there
		float RampSecondsAt(float a_mult)
		{
			const auto& s = Settings::Get();
			if (!s.rampUp || s.clampMultiplier <= s.waitMultiplier || a_mult <= s.waitMultiplier) {
				return 0.0f;
			}
			return std::clamp((a_mult - s.waitMultiplier) / (s.clampMultiplier - s.waitMultiplier), 0.0f, 1.0f) * s.rampSeconds;
		}

		void Publish(float a_scale)
		{
			sharedPhase.store(static_cast<int>(phase));
			sharedMult.store(currentMult);
			sharedBase.store(baseScale);
			sharedScale.store(a_scale);
		}

		void Restore(RE::TESGlobal* a_scale)
		{
			if (a_scale) {
				a_scale->value = baseScale;
			}
			lastWritten = baseScale;
			phase = Phase::kIdle;
			currentMult = 1.0f;
			Publish(baseScale);
		}

		void Tick()
		{
			tickQueued.store(false);

			const auto scale = TimeScale();
			if (!scale) {
				return;
			}
			const auto now = Clock::now();
			const auto& s = Settings::Get();

			// alt-tabbing away while the key is down loses the key-up, which would leave the speed-up on
			DWORD foregroundPid = 0;
			GetWindowThreadProcessId(GetForegroundWindow(), &foregroundPid);
			if (foregroundPid != GetCurrentProcessId() && (!held.empty() || toggled)) {
				held.clear();
				toggled = false;
			}

			// somebody else changed the timescale while we were holding it: let go, theirs wins
			if (phase != Phase::kIdle && std::fabs(scale->value - lastWritten) > 0.001f * std::max(1.0f, std::fabs(lastWritten))) {
				logger::info("Timescale changed by another mod ({} -> {}), dropping the speed-up", lastWritten, scale->value);
				phase = Phase::kIdle;
				currentMult = 1.0f;
				Publish(scale->value);
				return;
			}

			bool       want = WantActive();
			const auto block = CurrentBlock(now);
			sharedBlock.store(static_cast<int>(block));
			if (block != Block::kNone) {
				if (phase != Phase::kIdle) {
					logger::info("Speed-up stopped: {}", BlockName(block));
					Restore(scale);
				}
				if (s.mode == Settings::kToggle) {
					toggled = false;
				}
				want = false;
			}

			switch (phase) {
			case Phase::kIdle:
				if (want && scale->value > 0.0f) {
					baseScale = scale->value;
					phase = Phase::kActive;
					rampStart = now;
					currentMult = s.waitMultiplier;
					logger::info("Speed-up started from timescale {}", baseScale);
				}
				break;
			case Phase::kActive:
				if (!want) {
					if (s.releaseSeconds > 0.0f) {
						phase = Phase::kReleasing;
						releaseFrom = currentMult;
						releaseStart = now;
					} else {
						Restore(scale);
					}
				} else {
					currentMult = RampMultiplier(std::chrono::duration<float>(now - rampStart).count());
				}
				break;
			case Phase::kReleasing:
				if (want) {
					phase = Phase::kActive;
					rampStart = now - std::chrono::duration_cast<Clock::duration>(std::chrono::duration<float>(RampSecondsAt(currentMult)));
				} else {
					const float t = std::clamp(std::chrono::duration<float>(now - releaseStart).count() / s.releaseSeconds, 0.0f, 1.0f);
					if (t >= 1.0f) {
						Restore(scale);
					} else {
						const float eased = 1.0f - (1.0f - t) * (1.0f - t);
						currentMult = releaseFrom + (1.0f - releaseFrom) * eased;
					}
				}
				break;
			}

			if (phase != Phase::kIdle) {
				const float value = baseScale * currentMult;
				scale->value = value;
				lastWritten = value;
				Publish(value);
			} else {
				Publish(scale->value);
			}

			wake.store(phase != Phase::kIdle || BindingHeld() || toggled);
		}

		void QueueTick()
		{
			if (!tickQueued.exchange(true)) {
				SKSE::GetTaskInterface()->AddTask(Tick);
			}
		}

		class InputSink : public RE::BSTEventSink<RE::InputEvent*>
		{
		public:
			static InputSink* Get()
			{
				static InputSink instance;
				return std::addressof(instance);
			}

			RE::BSEventNotifyControl ProcessEvent(RE::InputEvent* const* a_event, RE::BSTEventSource<RE::InputEvent*>*) override
			{
				if (!a_event) {
					return RE::BSEventNotifyControl::kContinue;
				}
				const auto& s = Settings::Get();
				for (auto event = *a_event; event; event = event->next) {
					const auto button = event->AsButtonEvent();
					if (!button) {
						continue;
					}
					const auto code = ButtonCode(button);
					if (code == 0) {
						continue;
					}
					if (button->IsUp()) {
						held.erase(code);
					} else if (button->IsPressed()) {
						const bool wasHeld = held.contains(code);
						held.insert(code);
						if (!wasHeld && s.mode == Settings::kToggle && (code == s.hotkey || code == s.hotkey2) &&
							(s.modifier == 0 || held.contains(s.modifier))) {
							toggled = !toggled;
						}
					}
				}
				if (BindingHeld() || toggled || phase != Phase::kIdle) {
					wake.store(true);
					QueueTick();
				}
				return RE::BSEventNotifyControl::kContinue;
			}
		};

		class HitSink : public RE::BSTEventSink<RE::TESHitEvent>
		{
		public:
			static HitSink* Get()
			{
				static HitSink instance;
				return std::addressof(instance);
			}

			RE::BSEventNotifyControl ProcessEvent(const RE::TESHitEvent* a_event, RE::BSTEventSource<RE::TESHitEvent>*) override
			{
				if (a_event && a_event->target && a_event->target->IsPlayerRef()) {
					const auto& s = Settings::Get();
					if (s.blockWhenHit) {
						hitUntil = Clock::now() + std::chrono::duration_cast<Clock::duration>(std::chrono::duration<float>(s.hitCooldownSeconds));
						if (phase != Phase::kIdle) {
							QueueTick();
						}
					}
				}
				return RE::BSEventNotifyControl::kContinue;
			}
		};
	}

	const char* BlockName(Block a_block)
	{
		switch (a_block) {
		case Block::kMenu: return "a menu or dialogue is open";
		case Block::kLoading: return "the game is loading";
		case Block::kDead: return "the player is dead";
		case Block::kCombat: return "in combat";
		case Block::kKillMove: return "kill move";
		case Block::kRecentlyHit: return "just got hit";
		default: return "nothing";
		}
	}

	void Init()
	{
		if (const auto input = RE::BSInputDeviceManager::GetSingleton()) {
			input->AddEventSink(InputSink::Get());
		}
		if (const auto holder = RE::ScriptEventSourceHolder::GetSingleton()) {
			holder->AddEventSink<RE::TESHitEvent>(HitSink::Get());
		}

		// A small clock: input only arrives while a button changes, but the ramp and the safety checks have to keep running while it is held
		std::thread([]() {
			while (true) {
				std::this_thread::sleep_for(wake.load() ? 16ms : 100ms);
				if (wake.load()) {
					QueueTick();
				}
			}
		}).detach();

		logger::info("Dynamic Wait NG ready: hotkey {}, x{}", Settings::Get().hotkey, Settings::Get().waitMultiplier);
	}

	Status GetStatus()
	{
		return { static_cast<Phase>(sharedPhase.load()), static_cast<Block>(sharedBlock.load()), sharedMult.load(), sharedBase.load(), sharedScale.load() };
	}

	void Cancel(bool a_restore)
	{
		if (phase != Phase::kIdle) {
			if (a_restore) {
				Restore(TimeScale());
			} else {
				phase = Phase::kIdle;
				currentMult = 1.0f;
				Publish(sharedScale.load());
			}
		}
		toggled = false;
		held.clear();
	}

	void SettingsChanged()
	{
		if (phase != Phase::kIdle || BindingHeld()) {
			wake.store(true);
		}
	}

	void Save(SKSE::SerializationInterface* a_intfc)
	{
		if (phase == Phase::kIdle) {
			return;
		}
		struct Record { std::uint8_t active; float base; } record{ 1, baseScale };
		if (!a_intfc->WriteRecord(kRecordType, kRecordVersion, record)) {
			logger::error("Could not write the co-save record");
		}
	}

	void Load(SKSE::SerializationInterface* a_intfc)
	{
		std::uint32_t type, version, length;
		while (a_intfc->GetNextRecordInfo(type, version, length)) {
			if (type == kRecordType && version == kRecordVersion && length == 8) {
				struct Record { std::uint8_t active; float base; } record{};
				if (a_intfc->ReadRecordData(record) && record.active && record.base > 0.0f) {
					pendingRestore = true;
					pendingBase = record.base;
				}
			}
		}
	}

	void OnPostLoadGame()
	{
		// the save itself holds the sped-up value
		Cancel(false);
		if (pendingRestore) {
			if (const auto scale = TimeScale()) {
				logger::info("Save was written while time was sped up, timescale {} -> {}", scale->value, pendingBase);
				scale->value = pendingBase;
			}
			pendingRestore = false;
		}
		if (const auto scale = TimeScale()) {
			baseScale = scale->value;
			lastWritten = scale->value;
			Publish(scale->value);
		}
	}

	void OnRevert()
	{
		pendingRestore = false;
		Cancel(false);
	}
}

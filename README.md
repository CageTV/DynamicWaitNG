# Dynamic Wait NG

Hold a key or button to speed up time. A new take on [Dynamic Wait SKSE](https://www.nexusmods.com/skyrimspecialedition/mods/119008) by ThirdEyeSqueegee (open permissions), written from scratch. It reads the same four ini keys, so an existing `DynamicWait.ini` is picked up on the first run. Do not run both at once.

## What it does

- Hold the key (apostrophe by default) and the game's timescale is multiplied; let go and it is put back.
- It multiplies the timescale you already have, and gives it back exactly, so a mod or console command that changed the timescale is respected. If another mod changes the timescale while you hold the key, this one lets go and leaves their value alone.
- Optional gradual speed-up from `fWaitMultiplier` to `fClampMultiplier` over `fRampSeconds`, and a smooth slow-down after letting go (`fReleaseSeconds`).
- Hold mode or press-to-toggle, a second binding (mouse or gamepad button), an optional modifier key.
- Stops by itself in combat, when you are hit (for a few seconds), in menus, dialogue and loading screens, when dead, and when you alt-tab away with the key down.
- A save written while time is sped up remembers the normal timescale, so loading it does not leave you at 10x.
- On-screen speed indicator and a settings page in the [SKSE Menu Framework](https://www.nexusmods.com/skyrimspecialedition/mods/120352) when it is installed. Everything is also in `Data/SKSE/Plugins/DynamicWaitNG.ini`.
- No game hooks and no addresses: it uses the input event sink and the game's own timescale global, so it should keep working across game updates.

## Requirements

SKSE64, Address Library for SKSE Plugins. SE and AE (1.5.97 to 1.7.x). The SKSE Menu Framework is optional.

## Settings

| key | meaning |
|---|---|
| `uWaitHotkey`, `uWaitHotkey2` | keyboard scan code (apostrophe = 40), mouse button = 256 + n, gamepad button = 266..281 |
| `uModifierHotkey` | key that has to be held as well, 0 = none |
| `iMode` | 0 hold, 1 toggle |
| `fWaitMultiplier`, `bRampUpMultiplier`, `fClampMultiplier`, `fRampSeconds`, `fReleaseSeconds` | speed |
| `[Safety]` | `bBlockInCombat`, `bBlockWhenHit`, `fHitCooldownSeconds`, `bBlockInMenus`, `bBlockWhenDead` |
| `[HUD]` | `bShowHud`, `fHudX`, `fHudY`, `fHudScale` |

## Building

Built in an [alandtse CommonLibSSE-NG](https://github.com/alandtse/CommonLibVR/tree/ng) tree through `ng.cmake`. Needs `simpleini` and `spdlog` from vcpkg.

## License

GPL-3.0-or-later (it bundles the SKSE Menu Framework header, GPL-3.0).

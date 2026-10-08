# MW ActionNitro

Action-based nitrous recovery for **Need for Speed: Most Wanted (2005)**. Independent 32-bit ASI, version **0.1.0-alpha.7**.

[Download](https://github.com/Zakkey250/MW-ActionNitro/releases) · [日本語](README_JA.md)

Replaces the player's passive NOS recharge with driving rewards. It calls the game's native NOS charging function, so the stock gauge and CustomHUD read the same internal tank value. An independent text HUD shows each reward and the points actually credited.

## Rewards

| Action | Default reward |
| --- | --- |
| Near miss / oncoming near miss | 100 / 200 points |
| Oncoming lane | 1 point per metre |
| Slipstream | 2 points per metre |
| Jump / airtime | 100 points / 40 points per second |
| Drift | 50–100 points per second, weighted by speed and angle |
| Sustained drift | 100 extra points every 3 seconds |
| Near-miss chain | 50-point increments, capped at 200 extra points |
| Top speed | At least 75% of the car's estimated maximum speed: 4% of tank per simulation second |

100 points restore 0.3 seconds of NOS by default. Recovery stops while using NOS and is limited by tank capacity. Collision, pause, loading, invalid telemetry and action-specific speed/ground checks suppress rewards. Top-speed recovery uses the stock HP Remastered police-side rate as its reference; it is not a claim of identical gameplay balance across the two games.

Physical sliding works independently. A compatible **MW Arcade Drift** can optionally supply its confirmed drift state through a read-only API. It is not required, and neither its physics nor configuration is changed.

## Install / update

One ASI supports the verified **Redux 3.04 English 1.3 / 4 GB executable** and the supported **NFSPatcher English 1.3 executable**, with or without its 4 GB patch. Requires an existing ASI loader. See [executable compatibility](COMPATIBILITY.md) for exact identities. Other executables are rejected before hooking. This does not add support for different regional executable builds; game text resources can be multilingual.

1. Close the game and extract the release ZIP.
2. Run `Install.ps1 -GameDirectory "your game folder"` in PowerShell. It checks the executable, backs up this MOD, preserves existing INI values and adds missing settings. Write permission to the game directory is required.
3. Launch using your usual launcher. New installations use `ObserveOnly=0` (active). Existing observation mode is preserved; change it to `0` if you want rewards enabled.

For manual installation, copy `scripts/NFSMWActionNitro.asi` into the game's `scripts` folder. Copy the included INI only on a new install; merge settings when updating. Keep your current ASI loader. No game assets, saves or other MODs are included or replaced.

To uninstall, close the game and remove this MOD's ASI/INI. To roll back, restore its backed-up files from `ActionNitro-backups`.

## Languages

Both reward HUD and update notices cover all 15 text-language files found in the target game's language set, plus English UK, Simplified Chinese and Thai options, for **18 selectable variants**:

`en`, `en-GB`, `fr`, `de`, `it`, `es`, `es-MX`, `nl`, `sv`, `da`, `fi`, `pl`, `ru`, `ja`, `ko`, `zh-TW`, `zh-CN`, `th`.

Set `[HUD] Language=auto` (default) to follow Widescreen Fix's language override, then Widescreen Fix's local `Settings.ini` language when `WriteSettingsToFile` is enabled, then the game's 32-bit registry language, then the native language-table filename. This preserves Japanese bridge installations using an English executable. Unknown settings fall back to English. A language code explicitly overrides auto detection. Selection is read once at startup; restart after changing it.

Windows fonts render Unicode without modifying game fonts. Appropriate CJK/Thai fonts must be installed in Windows. Long labels shrink to fit. Translations are functional labels for this MOD, not extracted official game strings. HUD `Right`, `Bottom`, and `Width` can be adjusted to match the speedometer; automatic tracking of CustomHUD layouts is not implemented.

## Update notices

`[Updates] Enabled=1` checks this repository's releases once at startup, on the initialization worker. `Language=auto` follows the HUD language; an explicit code can override it. Use `Enabled=0` to disable the request.

The checker accepts newer version tags only when a matching uploaded binary ZIP exists. Stable installs do not advertise prereleases. Notices share the same per-process queue as other Zakkey250 MW MODs, time out automatically, and are suppressed/closed during gameplay. The GitHub button opens the fixed releases page only when clicked. Nothing is downloaded or installed automatically. Offline/HTTP errors leave gameplay unaffected.

## Performance and validation

The HUD retains cached GDI resources, updates text at most 10 times per second, and uploads only the visible texture rows. Diagnostic state changes are coalesced to at most four log records per second per category while retaining a bitmap/count of short transitions. No network or language lookup runs in the physics/render callback.

Alpha.6 validation passes reward calculations, 5,000 x86 hook iterations, unsupported-host rejection, all 18 language glyph/raster tests, 92 update-policy/dialog checks, and existing D3D9 pixel/state/Reset regression checks. The accepted driving predicates and reward rates are unchanged. New multilingual presentation and notification behavior still require acceptance in the actual game. Standalone rendering tests and earlier driving logs do not prove an in-game FPS improvement.

Known limits: slipstream can break at road-segment boundaries and does not raycast occluders; near misses target traffic vehicles; unrecognized roads/intersections are excluded from oncoming rewards. Hooks fail closed on conflicting patches. Test your particular MOD combination and HUD layout.

## Build

Visual Studio Build Tools 2022 (v143, C++ desktop tools and Windows SDK), Win32, C++17. Run `tools/Build.ps1` from a source checkout. It builds the ASI and isolated tests without launching the game. Optional Python verification tools need `pefile`, `capstone`, and `unicorn`, and your own supported game installation.

Third-party code: MinHook and nlohmann/json. Their licenses are preserved under `third_party` and in the binary ZIP. The project contains no game assets or game executable.

## License

Original project code and documentation are licensed under the [MIT License](LICENSE). MinHook and nlohmann/json retain their respective licenses. The MIT License grants no rights to *Need for Speed* or other third-party material.

## Alpha.7: shared Redux / Main binary

Adds the verified Redux 3.04 executable as an exact size/hash pair, retaining all live hook guards. No separate Redux ASI or loader replacement is needed. Local Widescreen Fix settings now take precedence over another installation's registry language. Gameplay predicates, rewards and rendering are unchanged from alpha.6.

Validated both installed executable files through the shipping SHA-256 helper, all 11 static hook/reader guards and 3,260 native-selected Redux road fixtures. Full isolated build/tests pass. Redux startup hook coexistence, in-game NOS/HUD and actual driving remain to be verified; static compatibility is not runtime acceptance.

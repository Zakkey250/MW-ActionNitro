# Executable compatibility

`NFSMWActionNitro.asi` is the same binary for all targets below. Target names describe verified files, not blanket approval of every executable bundled with similarly named mod packs.

| Target | Bytes | SHA-256 |
| --- | ---: | --- |
| NFSPatcher English 1.3 | 6,029,312 | `80774C2E5D619B4F120B48D4462896FD504C263399D203A238769CFFDE1D253C` |
| NFSPatcher English 1.3, 4 GB | 6,029,312 | `B248271BF8EAC8C9B283B8C95E3ADD672B713BF529B05F1780E58268493B9D06` |
| Redux 3.04 English 1.3, 4 GB | 5,926,912 | `0C5675A08CD71FD6D31CA87E992A915054BD8B80D268BFF0561D7ECC2067E342` |

The verified Redux executable's `.text`, `.rdata` and `.data` bytes, virtual addresses and virtual sizes match the Main target. Resource section differences account for the different file identity. The common engine adapter retains all 11 startup byte guards and runtime object/vtable validation; no guessed addresses or broad size-only acceptance were added. Both original target identities remain supported.

The installer and ASI require a matching size/hash pair. The Python static verifier checks that their policies agree. Unknown or modified executables are rejected. Conflicting runtime patches at guarded hook sites are also rejected rather than overwritten.

## Existing Redux components

- Keep Redux's `dinput8.dll` and existing Widescreen Fix, CustomHUD, camera, graphics and save setup.
- NOS recovery uses the native internal tank value, independent of the gauge skin.
- MW Arcade Drift integration remains optional and read-only. ActionNitro does not enable camera features or change drift settings.
- With Widescreen Fix `WriteSettingsToFile=1`, language detection reads only `Language` from the configured user-data directory's `NFS Most Wanted/Settings.ini`. Explicit ActionNitro/WSF language overrides still take precedence. This avoids using another installation's global registry language.
- HUD placement remains configurable through ActionNitro's `Right`, `Bottom`, and `Width`; gauge layouts are not repositioned automatically.

## Validation boundary

Alpha.7 verifies actual Main/Redux files with the shipping hash reader, known/unknown/crossed target identities, 11 static hook guards and 3,260 Redux native-selected road fixtures. Isolated reward, hook, localization, HUD and notification tests pass. The accepted action predicates and rewards are unchanged from alpha.6.

Redux startup with the complete installed MOD chain, visible HUD behavior and NOS recovery during driving still require in-game acceptance. No claim of an FPS improvement is made.

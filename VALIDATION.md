# v0.4.2 release validation — October 1, 2026

- UE 5.8 Editor and Windows Shipping compilation, cook, stage and archive passed.
- TEA: 83 checks, zero errors; sizes, original damage/radius/cooldown and 10 FPS lifetimes checked.
- HUD/block: 291 checks, zero errors.
- Iron Matriarch: 110 checks, zero errors with audio enabled. The no-sound run intentionally could not create the intro audio component.
- Atlas verification: zero errors.
- Enemy expansion: 35,982 checks, zero failures.
- Final packaged startup/audio smoke and inventory/loot smoke both exited 0 with zero errors.
- Inventory synthetic drag targets were moved inside destination cells to avoid physical-pixel rounding at exact borders; gameplay gesture behavior is unchanged.
- The Windows linker needed an explicit SDK tool environment for manifest embedding; final packaging completed successfully afterward.
- Both release ZIPs are integrity-tested and accompanied by SHA-256 files.

The approved TEA visuals were inspected in rendered gameplay. This is not an exhaustive manual playthrough of every floor. StreamPixel hosted-browser testing remains required after upload. Historical validation is retained in Docs/Archive.

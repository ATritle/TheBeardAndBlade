# StreamPixel — v0.4.2

Upload `Builds/TheBeardAndBlade-StreamPixel-v0.4.2.zip` to The Beard And Blade project. Select `Windows/TheBeardAndBlade.exe` if asked.

This is the complete Pixel Streaming-enabled Shipping runtime, not a source-project ZIP. No credentials or fixed signalling address are embedded.

After validation, activate the build and open the stream. Test audio, mouse capture, movement, inventory, map, blocking, MMB tea throw, skill 2 Tea Spirit and boss combat. Check that the expansion enemies render on the bunker floor. GitHub publication does not deploy to StreamPixel; replace the previous v0.4.2 upload with this refreshed ZIP.

Reproduce with `Tools/package_windows.ps1 -Destination "$PWD/Builds/v0.4.2"`, then `python Tools/zip_release.py --version v0.4.2 --streaming`. Use a fresh destination when rebuilding.

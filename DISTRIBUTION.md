# Distribution — v0.4.2

Build using UE 5.8 and the Windows C++ toolchain:
`Tools/package_windows.ps1 -Destination "$PWD/Builds/v0.4.2"`

Create both archives:
`python Tools/zip_release.py --version v0.4.2`
`python Tools/zip_release.py --version v0.4.2 --streaming`

The Windows ZIP uses TheBeardAndBlade/; StreamPixel uses Windows/. Both contain the same Shipping runtime with Pixel Streaming enabled. Distribute the whole folder, never the EXE alone. SHA-256 files accompany both archives.

Players do not need Unreal Editor. Visual C++ prerequisites are under Engine/Extras/Redist/en-us. The build is unsigned; do not disable Windows security globally to bypass reputation warnings. No campaign save or automatic updater. Hosting activation is separate.

Engine installations, caches and local packaged builds are excluded from source control. Public hosting does not grant an open-source license. Preserve third-party credits and Unreal Engine terms.

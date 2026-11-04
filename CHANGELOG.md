# Changelog

## v1.0 — 2026-11-04 — First release

- First public release of the PS Vita port (`REDEGEN01`), verified end-to-end
  on real PS Vita hardware.
- Native **960×544** landscape rendering via vitaGL at **60 FPS**, with the
  original `480×320` GLES1 framebuffer scaled to full screen.
- Custom Mach-O ARMv6 loader: 3 segments, 28 load commands, 8,011 symbols,
  external relocations, symbol pointers, ObjC metadata and C++ vtables.
- Minimal ObjC runtime with Foundation/UIKit/EAGL classes, `UIApplicationMain`,
  the `s3eAppDelegate` lifecycle and `MainWindow.nib` handling.
- Marmalade/S3E file I/O with `rom://`, `ram://`, `rst://` and `raw://` drive
  prefixes; `main.dz` (48 LZMA streams) is read by the original engine.
- Runtime-only `s3e.icf`/`app.icf` overrides (64 MB memory, 32 MB DZip bucket,
  landscape) without modifying the user's game files.
- Audio through `AudioQueueNewOutput` / `AudioQueueEnqueueBuffer` translated
  to `sceAudioOut` (16 kHz mono PCM, digital gain).
- Full physical controls plus front touch:
  - left/right stick and D-pad move Leon;
  - Cross interacts/fires/confirms; Circle runs/closes menus, Circle + Down
    quick-turns;
  - R aims; L enters knife mode; Square reloads; Triangle switches target;
  - Start/Select open the PDA and skip cutscenes; L1/R1 switch PDA tabs;
  - the front touch screen drives the HUD, menus and PDA.
- Saves in `ux0:data/redeg/game/Documents/` (`savedata2.i3d`).
- Loader-only VPK; game data must be provided by the user from a legally
  obtained, decrypted IPA.

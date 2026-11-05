# Troubleshooting

> The VPK does **not** contain the game. Check the layout in
> [`INSTALL.md`](INSTALL.md).

---

## 1. Game does not launch / returns to LiveArea

1. Check **kubridge** and **FdFix** in `ux0:tai/config.txt` under `*KERNEL`.
2. Check `ur0:data/libshacccg.suprx`.
3. Check `ux0:data/redeg/game/residentevil`. It must be the Mach-O file from
   `Payload/residentevil.app/` and must not be renamed.
4. Check `ux0:data/redeg/game/main.dz`, `s3e.icf`, `app.icf` and
   `MainWindow.nib`.
5. Read the last lines of `ux0:data/redeg/logs/redeg_NNN.log`.
6. Confirm that the IPA was decrypted before extraction (`cryptid = 0`).

Typical log messages:

```
Mach-O load failed
chdir(ux0:data/redeg/game) failed
```

## 2. "Invalid mode" or configuration error

- `s3e.icf` and `app.icf` must be the original files from the IPA.
- Do not edit them. The port applies its runtime overrides in memory only.
- Re-copy both files from `Payload/residentevil.app/`.

## 3. Black screen or wrong orientation

- Make sure no other display/overlay plugin is forcing a resolution or
  rotation for `REDEGEN01`.
- Delete `ux0:data/redeg/shader_cache/` once and reboot.
- Check that the game data is at `ux0:data/redeg/game/`, not
  `ux0:data/redeg/residentevil.app/` or `ux0:data/redeg/game/residentevil.app/`.

### Stretched triangles / exploding animated meshes

Use port version **1.0.1** or newer and check its version in the new run's
log. The loader in 1.0 could rebase a numeric sine-table entry as a pointer,
which broke skeletal animation. Reinstalling textures does not repair that
executable. See the [investigation](GRAPHICS-INVESTIGATION-2026-11-05.md).

## 4. No audio

1. Check the PS Vita volume and mute state.
2. Check the in-game audio options.
3. Look for `[audio]` lines in the log.
4. The port uses 16 kHz mono output through `sceAudioOut`; it does not change
   the system volume.

## 5. Saves not working

- Saves are stored in `ux0:data/redeg/game/Documents/`.
- The tested build creates `savedata2.i3d`.
- Make sure the folder is writable and has free space.
- Do not restore a save from a different game version; the engine validates
  its own data.

## 6. Input problems

- Remove pad-remapping, overlay or macro plugins for this title.
- The port already maps the left/right stick, D-pad, face buttons, triggers,
  Start/Select and the front touch screen.
- In walk mode, Circle runs forward; Circle + Down performs the quick turn.
- In menus, D-pad/left stick move the cursor, Cross confirms and Circle goes
  back.

## 7. Cutscenes and QTEs

- Start or Select skips a cutscene.
- Cross and the D-pad directions trigger the QTEs.
- If a cutscene is skipped unintentionally, check for a stuck Start/Select
  input or a plugin that injects button presses.

## 8. vpk installation

- Install `redeg.vpk` with VitaShell; the Title ID is `REDEGEN01`.
- If the LiveArea icon/art looks stale after reinstalling, reboot the console
  or refresh the LiveArea.
- Keep `SHA256SUMS.txt` and verify the VPK if the install fails.

## 9. Reporting a bug

Include:

- port version/commit;
- game version and whether the IPA is decrypted;
- PS Vita/PSTV model and CFW;
- plugins installed;
- the relevant part of `ux0:data/redeg/logs/redeg_NNN.log` (redact IP
  addresses, serial numbers and personal data).

**Do not attach or link** the IPA, `residentevil`, `main.dz`, `.icf` files,
`.nib`, videos, saves or any other copyrighted game data. Issues containing
that content will be closed. See [`../LEGAL.md`](../LEGAL.md).

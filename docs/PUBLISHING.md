# Publishing and repository settings

> How this port is published safely, what goes into the repository, and how
> the private repository is configured.

---

## 1. What other ports do

The existing ports in this account (`brucelee-vita` and `tasm-vita`) follow
the same model:

- repository with loader source, build scripts, documentation and LiveArea
  artwork;
- **no** APK/IPA, game executable, content packages, saves or logs;
- README with legal notice, the exact game version, data layout and controls;
- VPK published as a GitHub **Release** asset, with `SHA256SUMS.txt`.

Conclusion: **port code + documentation + loader VPK** can be published;
**game data cannot**. The README must state that users need a legal copy.

This project also:

- Uses LiveArea artwork derived from the original game (icon, background,
  startup and profile images). It is included only to identify the unofficial
  port; all rights remain with the original owners.
- Does not include internal reverse-engineering notes (offset maps, port log,
  status) — they are in `.gitignore`.
- Ships a VPK with `eboot.bin`, `sce_sys/` and `provenance.txt`, but **no**
  `residentevil`, `main.dz`, `.icf`, `.nib`, `intro.mp4`, textures, audio,
  saves or other game data.

---

## 2. Legal checklist before publishing

- [ ] `git ls-files` contains no `.ipa`, `.app`, `residentevil`, `.dz`,
      `.icf`, `.nib`, `.mp4`, save, log or game data.
- [ ] The only game-derived files are the LiveArea art under
      `extras/livearea/`; they are documented in `LEGAL.md`.
- [ ] `extras/livearea/source_*`, `extras/livearea_personal/` and
      `extras/loading*.rgb565` are ignored.
- [ ] `build/`, `data/`, logs and the IPA are ignored.
- [ ] The release VPK uses the tracked LiveArea art (`extras/livearea/`).
- [ ] `unzip -l build/redeg.vpk` shows no game data (only `eboot.bin` and
      `sce_sys/*`).
- [ ] `README.md`/`LEGAL.md` state that users need a legal, decrypted copy.
- [ ] `AUTHORS.md` keeps original and third-party credits.
- [ ] The release does not attach the IPA, game data or saves.
- [ ] A takedown channel (GitHub Issues) is described in `LEGAL.md`.

---

## 3. Repository settings

The repository is **private** and owner-only, matching the Spider-Man port:

1. Name: `redeg-vita`.
2. Visibility: **private**.
3. Description: `Unofficial PS Vita port of Resident Evil: Degeneration (iOS
   1.03.00). Loader-only; game data not included.`
4. Topics: `resident-evil`, `resident-evil-degeneration`, `capcom`, `ios`,
   `homebrew`, `port`, `ps-vita`, `psvita`, `vitagl`.
5. **Collaborators:** none.
6. **Features:** Issues on; Wiki, Projects and Discussions off.
7. **General:** disable auto-merge; allow only squash merge; enable
   automatically delete head branches.
8. **Actions:** disabled (or read-only workflow permissions and no automatic
   PR approval).
9. **Branches/rulesets:** protect `main` if the account plan allows it.

> GitHub Free does not expose branch protection/rulesets for private
> repositories. On that plan, owner-only is enforced by having **no
> collaborators** and keeping the repository private. If the plan supports
> rulesets, use the same configuration as `brucelee-vita`:
>
> - require a pull request before merging;
> - 1 approval;
> - dismiss stale approvals;
> - require conversation resolution;
> - require linear history;
> - no force pushes and no deletions;
> - owner/admin bypass enabled.

Tag protection for `v*` (if available):

- restrict deletions/updates/non-fast-forward;
- only the owner creates/updates releases.

---

## 4. Release

1. Build `build/redeg.vpk` and `build/SHA256SUMS.txt`.
2. Create a tag/release `v1.0` (or the current version).
3. Attach **only** `redeg.vpk` and `SHA256SUMS.txt`.
4. Release body: important/legal notice, required iOS IPA version, installation
   link, controls, tested hardware, SHA-256 and credits.
5. Never attach the IPA, `residentevil`, `main.dz`, game data, saves or logs.

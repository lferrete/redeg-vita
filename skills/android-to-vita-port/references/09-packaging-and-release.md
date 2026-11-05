# 09 — Empacotamento e release

> VPK, SELF, LiveArea, dados externos, testes e legalidade.

---

## 1. VPK/SELF

- `vita_create_self(... UNSAFE NOASLR)` para homebrew.
- `vita_create_vpk` pode quebrar com espaços no caminho; use
  `vita-mksfoex` + `vita-pack-vpk` (PowerShell) como fallback.
- Título/ID/versão; `ATTRIBUTE2=12` quando aplicável.
- O VPK contém `eboot.bin`, `sce_sys/` e LiveArea — **não** o jogo/assets.

## 2. LiveArea

- `icon0.png` 128×128, 1:1
- `pic0.png` 960×544, 16:9, **full-bleed**
- `bg0.png` 840×500, **full-bleed**
- `startup.png` 280×158, 16:9
- PNG **8-bit indexado (`P`)**. `icon0` RGBA pode aparecer como bolha branca
  vazia no Vita.
- Full-bleed = cover crop (escala para cobrir e recorta); **não** cole o key
  art quadrado 1:1 centralizado num gradiente, senão a arte parece 4:3 com
  barras laterais.

Gere **somente** da arte deste jogo (ícone/key art do APK). Nunca reaproveite
`extras/livearea`, `extras/livearea_personal`, `source_icon.*` ou
`loading*.rgb565` de outro port. Apague/ignore esses leftovers antes de
buildar. Veja o checklist detalhado em
`ios-to-vita-port/references/08-packaging-livearea.md` (as regras de arte são
as mesmas).

Ao substituir arte manualmente, atualize `ux0:/app/<TITLEID>/sce_sys`,
`ux0:/appmeta/<TITLEID>` e `ur0:/appmeta/<TITLEID>` se existir, e reinicie.
O `ur0:/shell/db/app.db` (`tbl_appinfo_icon.iconPath`) pode apontar para
`ur0:appmeta/<TITLEID>/icon0.png`.

## 3. Dados do jogo

Documente no README:

```
ux0:data/<jogo>/
  lib/armeabi/*.so
  assets/...
  content/*.pakh, *.pakdNNN
  metadata.xml
  save/
```

O usuário copia os dados da cópia legal dele.

## 4. Instalação/teste

- Instalar VPK; se `promote` falhar, sobrescrever
  `ux0:/app/<TITLEID>/eboot.bin` via FTP.
- Testar em console limpo (sem saves antigos) e com save existente.
- Testar boot, menus, options, gameplay, pause, save/load, saída.
- 30+ min de estabilidade.

## 5. Release checklist

- [ ] Boot limpo.
- [ ] Menus/save/load/options.
- [ ] Gameplay/pause/resume.
- [ ] Todos os controles + touch.
- [ ] Áudio (música/SFX/volume/qualidade).
- [ ] FPS estável; sem crash 30+ min.
- [ ] Save não corrompe.
- [ ] Sem logs/dumps de debug.
- [ ] README/dados/legal.
- [ ] Créditos.

## 6. Legal

- Código do loader: MIT (ou a licença escolhida).
- Jogo, `.so`, assets, saves: propriedade dos detentores.
- Não redistribua o APK, os `.so`, os pacotes ou a arte original.

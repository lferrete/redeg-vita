# Checklists — port Android → PS Vita

> Imprima/cole no início de cada port. Marque só o que foi **verificado no
> hardware**.

---

## 0. Antes de começar

- [ ] APK e dados obtidos legalmente; não serão redistribuídos.
- [ ] Console com CFW, kubridge, FdFix, `libshacccg.suprx`.
- [ ] VitaSDK/MSYS2/Python/pyelftools funcionando.
- [ ] Espaço em `ux0:`; FTP/command port acessíveis.
- [ ] Keep-awake disponível; plano de backup de saves.
- [ ] Nunca tocar em firmware/`os0:`/`vd0:`/`PSP2UPDATE`.

---

## 1. Intake

- [ ] `unzip -l`, ABI, `AndroidManifest.xml`.
- [ ] Libs `.so` identificadas (motor, plataforma, JNI).
- [ ] `readelf -h/-d/-s`, imports classificados.
- [ ] Strings do motor/engine/versão.
- [ ] Assets/pacotes grandes mapeados.
- [ ] Fluxo de boot (main → init → loop) desmontado.
- [ ] Estratégia escolhida (loader + shims / híbrido).
- [ ] Dados copiados para `ux0:data/<jogo>`.

---

## 2. Loader

- [ ] Todas as libs carregam e relocam.
- [ ] 0 imports não resolvidos.
- [ ] Construtores rodam na ordem correta.
- [ ] Activity/ZACTIVITY/ANativeActivity preenchidas.
- [ ] `ZAndroidEntry`/JNI_OnLoad chamado.
- [ ] Command line `-c <content root>`.
- [ ] `metadata.xml` correto.
- [ ] Service/clipboard/persistent storage/atomics tratados.
- [ ] Watchdog/breadcrumb ativos.
- [ ] Log chega ao loop principal.

---

## 3. Gráficos

- [ ] EGL/vitaGL inicializa.
- [ ] Shader cache e `libshacccg` OK.
- [ ] Resolução 960×544; viewport correto.
- [ ] `zpgfxDisplayIsValid` = 1.
- [ ] Modos de vídeo (Find/GetInfo/Count) coerentes.
- [ ] Primeiro frame; logos/menus.
- [ ] Sem crash em 5+ min.
- [ ] FPS aceitável; sem stutter grosseiro.

---

## 4. Input

- [ ] D-pad, analógico, face buttons, Start/Select, L/R.
- [ ] Menus navegam/confirmam/voltam.
- [ ] Pause/resume com Start.
- [ ] Cross confirma no pause; Circle fecha/volta; submenus navegam.
- [ ] Touches de luta desligados enquanto pausado.
- [ ] Touch físico funciona.
- [ ] HUD mapeado (ataques, pad, pause).
- [ ] Ataques por toque (down/up) funcionam.
- [ ] Movimento analógico.
- [ ] Movimento D-pad.
- [ ] Analógico + D-pad combinam; diagonal normalizada.
- [ ] Deadzone/drift OK.
- [ ] Auto-tap só na tela de espera.
- [ ] Sem thread de poll travada.
- [ ] 10+ min sem perder input.

---

## 5. Áudio

- [ ] Porta aberta em 48000; volume máximo.
- [ ] `getMinBufferSize`/`<init>` OK.
- [ ] `getState` = 1; `getPlayState` = 3.
- [ ] `write` chamado; `byte[]`/offset em bytes.
- [ ] PCM não-zero (pico/RMS medidos).
- [ ] Taxa in/out em tempo real.
- [ ] Resampler FIR (não linear/cúbico).
- [ ] Espúrios ≤ -60 dB nos tons de teste.
- [ ] Ganho sem clipping.
- [ ] Música + SFX + pause + troca de fase.
- [ ] Config/save de volume correto.
- [ ] Sem dump/log por amostra no release.
- [ ] Sem underrun/clicks em 10+ min.

---

## 6. Save/config

- [ ] Paths virtuais (`/sdcard`, `/data/data/<pkg>`, `/save`, `/content0`).
- [ ] Save cria/load/salva.
- [ ] Options persistem.
- [ ] Backup/restore testado.
- [ ] Edição de save só com app fechado.
- [ ] Formato documentado (magic/tabela/valores).

---

## 7. Estabilidade/performance

- [ ] 30+ min de gameplay sem crash.
- [ ] Sem leak óbvio de memória.
- [ ] FPS estável; frame time.
- [ ] Sem erros de áudio/vídeo recorrentes.
- [ ] Pause/resume, troca de cena, save/load.
- [ ] Reinício do app preserva save/config.

---

## 8. Release

- [ ] VPK instala/promove em console limpo.
- [ ] Dados documentados (`ux0:data/<jogo>`).
- [ ] LiveArea/ícone/título/versão.
- [ ] Créditos/licença; sem assets redistribuídos.
- [ ] Logs de debug/dump removidos.
- [ ] README/PORTING_PLAN atualizados.
- [ ] Skill/docs atualizados com o que aprendeu.

---

## 9. Tabela de sintomas → causas prováveis

| Sintoma | Ver primeiro |
|---|---|
| Não carrega `.so` | kubridge, base, memória, path |
| `Unresolved import` | `dynlib.c`/ordem de dependência |
| Para em `zpInitializeSystem` | JNI/Activity/metadata/command line |
| `EnumeratePackages` vazio | paths/pacotes |
| Tela preta com loop | display-valid/viewport/formato |
| Controle morto após boot | thread de poll bloqueada |
| Toque no lugar errado | coordenadas centradas normalizadas |
| Ataque não funciona | HUD touch vs key event |
| Áudio mudo, porta aberta | `write`/`getState`/track descartada |
| Áudio mudo, `write` chamado | mixer/config/callback/volume |
| Chiado/estridência | resampler (alias) |
| Som baixo | mix baixo/ganho/config |
| Cortes/estalos | underrun/I/O na thread/grain |
| “Freeze” de ~110 s | tela desligando |
| Input de automação ignorado | `SCE_CTRL_INTERCEPTED` |
| FTP travado | reiniciar console |
| Save não persiste | path/exit overwrite/backup |

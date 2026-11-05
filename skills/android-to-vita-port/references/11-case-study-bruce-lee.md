# Estudo de caso — Bruce Lee: Dragon Warrior (Android → PS Vita)

> Documento histórico/técnico do port real. Serve para mostrar **como** os
> problemas apareceram e foram resolvidos; não copie offsets nem coordenadas
> para outros jogos.

---

## 1. O alvo

- APK: `10938-Bruce-Lee-Dragon-Warrior-v1.15.26.apk`
- Package: `com.digitallegends.brucelee`
- ABI: `armeabi` (ELF32 ARM, EABI5, softfp), GCC 4.4.3, não-stripped.
- Engine: **KRM** (namespace `krm`) + **ZSystem/ZPlatform** (Sony Ericsson).
- GLES 2.0; shaders GLSL em runtime.
- Dados: `Core.pakh` (índice) + `Core.pakd000` (~105 MB) dentro de
  `data.zpak`; bootstrap em `assets/BruceLee.zpak`.

### 1.1 Libs

| Arquivo | Papel |
|---|---|
| `lib/armeabi/libdefault.so` | motor KRM + jogo; exporta `ZInternalMain`, `ZMain`, `GameClass` |
| `lib/armeabi/libzplatform.so` | ZSystem/ZPlatform: `zp*`, `zc*`, `zpgfx*`, `zpinp*`, `zpadr*`, EGL/GLES |
| `lib/armeabi/libjava-activity.so` | ponte JNI da Activity; chama `ZAndroidEntry` |

Estratégia: **loader + shims**, sem reescrever o jogo.

---

## 2. Bring-up: do zero ao primeiro frame

### 2.1 Loader e imports

- `so_util` carrega `libzplatform.so` em `0x98000000` e `libdefault.so` em
  `0x99000000`.
- Imports resolvidos por dependência; 256/256 do zplatform e 349/349 do
  default cobertos por `source/dynlib.c` + shims.
- `memset`/`memcpy` do `libdefault` são thunks para `zp_memset`/`zp_memcpy`
  do zplatform, que por sua vez importa `memset` do libc. Resolver por ordem
  de carga (sem `DT_NEEDED`) cria recursão infinita; o `so_util` já faz certo.
- Atomics bionic (`__zcatomic_*`) usam a página helper `0xFFFF0FC0`, que não
  existe no Vita; substituídos por `__sync_*`.

### 2.2 Activity falsa e `ZAndroidEntry`

O layout da ZACTIVITY foi recuperado de `libjava-activity.so`:

```
+0x08  callbacks (word 0 = 0x70)
+0x10  ANativeActivity
+0x14  JavaVM*
+0x18  jobject (token)
+0x5c  input driver (word 0 = 0x260)
+0x60  surface holder (jobject em +0x0c)
+0x68  ANativeWindow (token)
+0x6c  largura 960
+0x70  altura 544
```

- `ZAndroidEntry()` precisa rodar depois dos construtores; ele preenche a
  tabela de input via `dlopen("libandroid.so")`.
- O teardown do zplatform chama `input_driver[0x24c/4]` e espera uma flag que
  só o input Android seta; anular esse callback evita o spin infinito.
- Primeiro word dos blocos de callbacks/input é **tamanho/versão**; se for
  menor que o esperado (`0x70`, `0x260`), a lib retorna cedo.

### 2.3 Linha de comando e metadata

- `ZInternalMain` espera `"<exe> -c <content root>"`; com `argc=1` o
  `zpInitializeSystem()` aborta.
- O `metadata.xml` do conteúdo precisa ser o **manifesto de aplicação**
  (com `<platforms><platform type="android">`), não o do container do
  `data.zpak`.
- `getSystemString(1)` deve retornar `"android"`; era `"US"` e o título nunca
  casava.

### 2.4 Stubs de boot

- `_zpServiceInitialize` → 0 (licenciamento Android não existe).
- `_zpClipboardInitialize` → 0 (clipboard não é necessário).
- `_zsysPersistentStorageReadData` → `0x30003` (sem storage; o jogo recria).
- `_zpWindowGetSize` → 960×544.
- `zpgfxDisplayIsValid` → 1.

### 2.5 EULA

`CheckEulaAcceptance()` em `libdefault+0x18d4c0` lia `EULA_ACCEPTED` e
levantava `no_eula`/`eula_accepted`. O estado de disclaimer não tinha input
mapeado para o Vita. Patch:

- `0x18d550` (BNE) e `0x18d5e0` (BEQ) → branch para `0x18d5e4` (aceito).
- Verificar opcodes originais; escrever com
  `kuKernelCpuUnrestrictedMemcpy`; `kuKernelFlushCaches`.

### 2.6 Display 960×544

`CZdkDisplay` pegava width/height 0 do window fake e não achava modo de
vídeo. Hooks:

- `GetWidth`/`GetHeight` → 960/544.
- `FindVideoMode(w,h,fmt)` → 0 se 960×544.
- `GetVideoModeInfo(0)` → 960×544/formato 3.
- `GetVideoModeCount` → 1.
- `zpgfxDisplaySetup` → preencher width/height antes do original copiar.
- `zpgfxDisplayIsValid` → 1.

Resultado: menu/logo/fight renderizando em 960×544 nativo, ~60 FPS.

### 2.7 Emulador local e primeiros bloqueios

Antes do hardware, um emulador ARM local (Unicorn + Capstone + pyelftools)
carregava os dois `.so` nas mesmas bases, aplicava relocações e stubs de
libc/GL/EGL/JNI. Ele revelou, em ordem:

1. **Recursão `memset`/`memcpy`:** `libdefault` exporta thunks para
   `zp_memset`/`zp_memcpy` do zplatform, que importa `memset` do libc.
   Resolver por ordem de carga (sem `DT_NEEDED`) entra em recursão infinita.
2. **`ZInternalMain(argc, argv)` é `main()`:** o Android passa
   `"<exe> -c <content root>"`; com `argc=1`, `zpInitializeSystem()` rejeita.
3. **`activity+0x5c` NULL:** `zpTerminateSystem()` lia
   `[[activity+0x5c]+0x24c]`; sem a tabela de input, data abort logo após o
   registro de `getSystemString`.
4. **Tamanho dos callbacks:** `ZAndroidEntry()` retorna cedo se o primeiro
   word de `activity+0x08` for `<= 0x6f`; o valor correto é `0x70`.
5. **Metadata:** o motor lê o **manifesto de aplicação** (com
   `<platforms><platform type="android">`), não o manifesto do container.
6. **`getSystemString(1)`:** precisa ser `"android"`, não `"US"`.
7. **Janela/surface:** `_zpWindowGetHandle()` devolve `activity+0x68`; sem ele
   `_zpgfxInitialize()` fica em loop de `zcThreadSleep()`. O surface holder em
   `activity+0x60` precisa de um `jobject` em `+0xc`.

Detalhes adicionais estão em `PORTING_PLAN.md` (§13–14).

---

## 3. Input: menus, HUD e luta

### 3.1 Camada correta

- `zinpGamepadPushButtonState` para teclas.
- `zinpTouchPushState` para touch.
- Não chamar `GameClass::OnInputEvent` da thread de poll.

### 3.2 Poll na thread principal

Uma thread dedicada de poll ficou presa em `sceCtrlPeekBufferPositive` após
boots lentos; o controle “morria”. A solução foi pollar no hook
`zpgfxDisplayIsValid`, na thread do jogo.

### 3.3 Touch

O struct de touch tem coordenadas centradas normalizadas em `+20/+24`
(`x_px = (x+0.5)*width`). Tipos `7/6/8`; flags `1/4/2`. O teste com
`MouseDown` confirmou `[mouse] DOWN x=830 y=430`.

### 3.4 HUD

`CTouchScreen::AddButton` logou os retângulos:

```
punch  = 720,0   240x272
kick   = 720,272 240x272
pad    = centro 160,408, raio 183
pause  = 0,95    120x102
top    = 380,0   200x108
```

A luta é touch-only. Ataques viraram toques sintéticos nos centros; movimento
virou arraste no pad.

### 3.5 Mapeamento final

- **Square/Triangle** → soco (toque no botão superior direito).
- **Cross/Circle** → chute (toque no botão inferior direito).
- **Analógico esquerdo + D-pad** → movimento (arraste no pad).
- **Start** → pause/back (B).
- **Circle** → “voltar” nos menus, ignorado na luta.
- **Pause** (`GamePause`): o menu confirma com **Select**, não A. Cross passou
  a ser traduzido para Select nesse estado; Circle toca o botão **RESUME**
  (fecha o pause); nos submenus, Circle→B volta. Os toques de luta são
  desligados no pause — antes, socos/movimento sintéticos clicavam em
  RESUME/SETTINGS sem querer.
- **Auto-tap** em `gameloadtouchwait` para iniciar a luta.

### 3.6 D-pad

Pedido do usuário: D-pad também move o personagem. Solução: o D-pad entra no
mesmo vetor do analógico, com deadzone zeroando o drift e normalização de
diagonais. Resultado no log: `-1.00,0.00`, `1.00,0.00`, `0,-1`, `0,1`; menus
continuam usando os key events.

---

## 4. Áudio: a caçada completa

### 4.1 Primeiro diagnóstico

Log:

```
[CAudioOutZdk] Open(freq: 44100, bps: 16, stereo: 1)
dlopen(libOpenSLES.so)
dlsym(opensles/crypto, slCreateEngine): stubbed
java_shim: registered 'android/media/AudioTrack/<init>' as id 0x1008
java_shim: ERROR: zadrSetOutputVolume not being called
```

OpenSL não existia; o motor caía no driver JNI `AudioTrack`.

### 4.2 Primeira implementação (porta abria, som mudo)

Implementamos o construtor do `AudioTrack` sobre `sceAudioOut`:

```
AudioTrack: sceAudioOut aberto (48000 Hz stereo, 512 frames/bloco;
           origem 44100 Hz, 2 ch)
```

Mas **nada era escrito**. O log não mostrava `write`.

### 4.3 Causa 1 — métodos registrados sem prefixo

O FalsoJNI registra `write`, `play`, `getState`... pelo nome **puro**; só
`<init>` tem `android/media/AudioTrack/<init>`. O shim testava
`strstr(full_name, "AudioTrack")` e nunca casava. `write` caía no stub e
retornava 0. Corrigido com `at_method_name()` por nome puro.

### 4.4 Causa 2 — `getState` = 0

O driver chama `getState()` após criar a track e só aceita `1`. O stub
retornava 0, então o driver **descartava a track** logo depois de abrir a
porta. Corrigido: `getState=1`, `getPlayState=3`, `getMinBufferSize=4096`,
`getMaxVolume=1.0`, `getMinVolume=0.0`, `write` como `byte[]`.

### 4.5 PCM ainda zero

Depois do dispatch, `write` era chamado, mas o PCM vinha todo zero. O jogo
estava na tela “enable sound” com a opção **NO** selecionada. Com **YES**,
apareceu o primeiro PCM não-zero. Lição: confirme o estado do jogo antes de
culpar o shim.

### 4.6 Volume do jogo

O dump do config mostrou:

```
'sound_volume' 0.000000
'music_volume' 0.000000
```

O save `save/SaveGame.zpak` (ZIP) contém `settings.dat`; os floats estavam em
0.0. Formato: tabela de chaves + records de 16 bytes
(`00 00 00 60 | key_index | 00 00 00 20 | float`). Chaves 65/66. Patch para
1.0, com backup e app fechado.

Mesmo em 1.0, o PCM continuou baixo: pico ~-20 dBFS, RMS ~-37.7 dBFS. O motor
não aplica o config como ganho no mixer. Solução: **ganho digital** no shim
(4× e depois 6×), com clamp.

### 4.7 Qualidade — o “ruído”

O som melhorou de volume, mas ficou com chiado/estridência. A causa era o
resampler 44100→48000. Medições com Catmull-Rom:

| Entrada | Espúrio | Nível |
|---:|---:|---:|
| 5 kHz | 8.9 kHz | -46.6 dB |
| 10 kHz | 13.9 kHz | -26.4 dB |
| 15 kHz | 18.9 kHz | -13.8 dB |
| 18 kHz | 21.9 kHz | -7.6 dB |

O espúrio de 15 kHz em -13.8 dB é audível. Trocamos por um **FIR polifásico**
(1024 fases × 48 taps, Kaiser beta 10, fc 0.42, normalização por fase, ring
8192, pré-pad 24), com espúrios ≤ -67 dB. O som ficou “ótimo” segundo o
usuário.

### 4.8 Fatos do `sceAudioOut`

- MAIN só aceita **48000 Hz**; 44100 retorna `0x80260008`
  (`INVALID_SAMPLE_FREQ`), apesar do header listar 44100.
- Volume máximo = 32768.
- Grain 512; `sceAudioOutOutput` paceia.
- Taxas medidas: `in≈44000/s`, `out≈47900/s` (tempo real).

### 4.9 Estado final do áudio

- Porta 48000 estéreo, volume máximo.
- FIR ativo; ganho 6×; pico de saída ~13k–17k.
- Música e SFX audíveis, sem ruído perceptível.
- Dump de PCM removido do build final (era diagnóstico).

---

## 5. Save/config — formato descoberto

`SaveGame.zpak`:

```
metadata.xml
settings.dat (4878 bytes)
```

`settings.dat`:

```
magic "1FDKENONCTX3N" + padding + u32 key_count (78)
key table: u32 hash, u32 name_len (sem NUL), name + NUL
value area: records de 16 bytes:
  00 00 00 60 | key_index u32 LE | 00 00 00 20 | float LE
```

Chaves relevantes: 63 `input_layout`, 64 `input_gui_alpha`, 65
`sound_volume`, 66 `music_volume`, 67 `arcade_time`, 68 `versus_time`, 69
`survival_time`, 70 `camera_tilt`, 71 `language`, ...

Edição: descompactar, alterar os floats, rezipar preservando compressão,
enviar com o app fechado, relançar. Backup obrigatório.

---

## 6. Incidentes de automação/depuração

### 6.1 Tela desligando

O que parecia freeze de ~110 s era a tela desligando. `keepawake` resolveu.

### 6.2 FTP travando

FTP frequentemente pendura; reiniciar o console resolve. `promote` falha com
frequência; sobrescrever `ux0:/app/<TITLEID>/eboot.bin` via FTP é o fallback.

### 6.3 Input injetado não chega

`SCE_CTRL_INTERCEPTED` (`0x00010000`) apareceu intermitentemente. Sequências
longas em lote perderam comandos. A solução foi enviar um press por vez,
confirmar no log (`[pad] buttons=`) e repetir. Em uma sessão, o jogo ficou no
askforsound porque o YES não entrou; um Cross isolado depois funcionou.

### 6.4 Logs deduplicados

O logger junta linhas idênticas (`(x300 seguidas)`); ao procurar um evento,
considere que ele pode estar representado por uma linha com contador.

---

## 7. Linha do tempo resumida

1. Extração do APK, análise ELF, imports, loader.
2. Emulador local (Unicorn) revela ZACTIVITY, command line, metadata,
   service/storage/atomics.
3. Boot no hardware; EULA; display 960×544; primeiro frame.
4. Input: menus, touch, HUD, ataques, pause, auto-tap.
5. Áudio: OpenSL stub → AudioTrack JNI → dispatch → getState → YES →
   config/volume → ganho → resampler FIR.
6. Qualidade/volume final; D-pad; documentação/skill.

---

## 8. Números e offsets deste port (apenas referência)

- `libdefault.so` EULA: `0x18d550`/`0x18d5e0` → `0x18d5e4`.
- `libzplatform.so` activity global: `0x2bb7f8`.
- ZEVENT touch: down 7, move 6, up 8; key down 18, up 17.
- ZP buttons: 0 up, 1 down, 2 left, 3 right, 4 A, 5 B, 6 X, 7 Y, 8 start,
  9 select, 10 L, 11 R.
- HUD: punch 720,0 240×272; kick 720,272 240×272; pad 160,408 r=183.
- Áudio: MAIN 48000; `0x80260008`; ganho 6×; FIR 1024×48, fc 0.42.
- Save: settings.dat keys 65/66; floats `00 00 80 3f` = 1.0.

**Não reutilize offsets**: cada jogo/versão tem os seus. Use o método, não o
número.

---

## 9. O que faria diferente no próximo port

- Implementar logging estruturado e dump de PCM desde o início.
- Testar `getState`/`getPlayState` antes de abrir a porta.
- Medir o PCM (pico/RMS) antes de mexer em volume.
- Usar FIR desde a primeira versão do resampler.
- Validar automação de input com confirmação no log, nunca em lote.
- Manter keepawake e um script de deploy confiável desde o dia 1.
- Documentar cada descoberta imediatamente (este documento).

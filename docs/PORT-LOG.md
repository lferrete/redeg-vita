# Diário do port — Resident Evil: Degeneration (iOS → Vita)

## 2026-11-05 — Meshes, tabela de seno e versão 1.0.1

O registro completo, incluindo hipóteses rejeitadas, hashes de builds,
logs no Vita, comparação do dump em memória, regressão com Unicorn e
retestes está em [GRAPHICS-INVESTIGATION-2026-11-05.md](GRAPHICS-INVESTIGATION-2026-11-05.md).
O loader rebaseava a palavra numérica `0x0012000c` no LUT de seno como
ponteiro. A correção protege as tabelas e restaura o skinning nativo.
Este diagnóstico substitui as alegações históricas sobre matrix mode.

## 2026-11-02 — Sessão 1 (bring-up)

### Intake

- IPA validado: 12,86 MB; binário ARMv6 decriptado; `main.dz` com 48 streams
  LZMA, todos descomprimidos com tamanho exato (18,5 MB).
- Conclusão: jogo completo (13 áreas × 2 + mercenários, 8 idiomas, áudio).

### Análise

- Mach-O non-PIE, base 0x1000; 17 dylibs; 461 imports; 362 stubs; 74 relocs
  externas; 5 classes ObjC (`GLView`, `MainView`, `s3eAppDelegate`,
  `MyCLController`, `S3EViewController`).
- `main` em 0x31410 chama `UIApplicationMain(argc, argv, nil, nil)`.
- Delegate `s3eAppDelegate` vem do `MainWindow.nib`.
- Marmalade/S3E: engine estática no executável; dados em DZip/LZMA; saves em
  `HOME/Documents`.

### Decisão

Loader Mach-O próprio + runtime ObjC mínimo + shims iOS, com vitaGL para
EGL/GLES1. Rejeitada a versão Android (touchHLE) — o usuário quer o IPA
original.

### Implementação

- `redeg-vita/`: loader, runtime ObjC, classes Foundation/UIKit/EAGL,
  shims libc/CF/CG/áudio/misc, tabela de imports, gl_syms gerado do vitaGL.
- VPK com título `REDEGEN01` e arte gerada do `iTunesArtwork`.

### Bugs e correções

1. **Arte do Bruce Lee no bubble** — `extras/livearea` foi copiado do port
   anterior. Corrigido gerando ícones/fundos do key art do RE a partir do
   `iTunesArtwork` 512×512.
2. **Data abort em 0x98000000** — bloco alocado como RX (`0x0C20D050`) e o
   loader escrevia com `memcpy`. Trocado para RW (`0x0C20D060`) +
   `kuKernelMemProtect(__TEXT, RX)`.
3. **`Mach-O: vmsize 0x5449`** — `segment_command_t` sem `cmd,cmdsize`;
   o parser lia os bytes do comando como `segname`. Corrigido (sizeof 56).
4. **Logs de 0 byte** — fd mantido aberto; data abort perdia a atualização de
   diretório no FAT. Trocado para open/append/write/`sceIoSyncByFd`/close por
   linha.
5. **`vita_logs` com diretório** — `RETR ... 550`. Usar `vita_ls` + `vita_pull`.
6. **`promote` falhou (0x8010113D)** — fallback: `vita_push` do `eboot.bin`
   para `ux0:/app/REDEGEN01/eboot.bin` com `dangerous=true`.

### Próximo

- Ver o log da build com o parser corrigido, confirmar onde para
  (`UIApplicationMain`/delegate/EAGL).
- Implementar touch e áudio depois do primeiro frame.

## 2026-11-02 — Sessão 2 (config + rebase)

### Progresso

- `s3eFileInit()` + `s3eFileSetDrivePrefix("rom://")` antes do `main`:
  o engine lia a config antes de instalar os prefixes. `s3eFileInit` é um
  memset one-shot (bitmask), então a chamada do engine depois vira no-op.
- Removido `s3eFileSetRoot` com caminho real: o root é concatenado depois do
  prefixo de drive e corrompia caminhos absolutos do Vita.
- Corrigido o rebase cego de `__TEXT,__const`: ele corrompia tabelas ASCII
  (a tabela de modos "r" virava "b"), e o engine rejeitava
  `fopen("s3e.icf","rb")` com "Mode is invalid".
- `darwin_stat_t` agora é `__attribute__((packed))` (108 bytes). O memset de
  112 bytes zerava o `r7` salvo do chamador, causando crash no epílogo de
  `s3eFileChooseDrive`.
- Rebase de pools literais **Thumb-1** (`ldr rX,[pc,#imm8*4]`) além dos ARM.
- Heurística de "início de função" para ponteiros anônimos (Thumb/ARM) e
  cascata para tabelas de ponteiros dentro de `__text`.
- `performSelectorOnMainThread:waitUntilDone:YES` chamado de dentro de um
  functor na própria main thread agora executa inline (evita deadlock).
- `UIImage initWithCGImage:`/`drawInRect:`/`drawAtPoint:` implementados.
- `drawRect:` do `setNeedsDisplay` restaurado no tick da main thread.

### Resultado

A config carrega (sem alerta) e o engine avança por áudio/imagens. O bloqueio
atual é um crash em vm `0x291dc` ao executar inicializadores estáticos
(ponteiro/tabela ainda não rebaseado).

---

## 2026-11-02 (rodada 2) - rebase de vtables, stack switch e logger

### Corrigido

- **Passe "thumb thunk table" (run de 2+ palavras impares) removido.**
  Ele corrompeu o prologo do helper de arrays em `0x1010dc`:
  `0x000eb5ff` e `0x001c0017` viraram `0x980ea5ff`/`0x981bf017`, e o
  guest pulava para `0x9891711a`. Confirmado lendo a memoria do core dump.
  No lugar entrou um detector de **vtables Itanium**:
  `[-offset_to_top][typeinfo*][fn0][fn1]...`, com validacao do typeinfo
  (segunda palavra = string imprimivel). 198 vtables detectadas, 1519
  palavras rebaseadas.
- **Scan Thumb-1 com falso positivo em instrucao ARM.** O halfword `0x4ff0`
  de `push {r4-r11,lr}` em `0x28f00` era lido como `ldr r7,[pc,#0x3c0]` e
  rebaseava o addend PIC em `0x292c4` (quebrando o stack switch). Agora o
  scan Thumb ignora qualquer alvo que um `ldr` ARM tambem referencia.
- **Stack switch do s3e.** `_s3eStackSwitchReadConfig` zera
  `_g_iPhoneGlue2+0`; o primeiro `s3eFileRead` trocava `sp` para 0. O port
  aloca um stack de 1 MB e forca o caminho `noStackSwitch` (`b 0x29284` em
  `0x291cc`). O engine passou a ler o `main.dz` e iniciar as tabelas.
- **Logger.** `out[1700]` virou buffer estatico (`s_out`) e so a main thread
  escreve log. As pthreads de guest chamavam `sceIoOpen`/`snprintf` e
  abortavam com `SCE_KERNEL_ERROR_ILLEGAL_ADDR`.
- **Imagem RWX** (diagnostico) para excluir falha de permissao de pagina.

### Estado

O engine abre `main.dz`, le o header pelo callback de file I/O e comeca as
tabelas de decodificacao (`0x48ca6`, `0x3681c`). O crash atual e
`stop reason 0x10006` (illegal address) em instrucoes puramente de registro
(`adc`, `subs`), com CPSR.A=1 - abort **impreciso**. Suspeitas: ponteiro
nao rebaseado no heap s3e, ABI de stack ARMv6 (4 bytes) x SceLibKernel
(8 bytes), ou pagina do stack/fibre desmapeada.

### Proximos passos

1. Capturar `DFAR`/limpar o bit A e localizar a escrita invalida original.
2. Revisar alinhamento de stack nas entradas dos shims.
3. Continuar ate `[gl] N frames`.

---

## 2026-11-02 (rodada 3) — Renderização 60 FPS, Orientação Landscape `0°` e Input

### Corrigido

1. **Override em memória de `s3e.icf` / `app.icf` sem tocar nos arquivos originais (`source/shims/libc.c`):**
   - Restauramos `ux0:/data/redeg/game/s3e.icf` e `app.icf` aos bytes originais do IPA e implementamos a substituição em memória dentro de `redeg_read` quando o descritor corresponde a `s3e.icf` ou `app.icf` (`MemSize=67108864`, `DerbhBucketSize=33554432`, `DispFixRot=Landscape`).
2. **Alinhamento de 8 bytes da stack em `_s3eStackSwitchCallS3EFuncR12*` (`source/main.c`):**
   - As 6 variantes de `_s3eStackSwitchCallS3EFuncR12*` faziam `push {r4-r11, lr}` (36 bytes = 4 mod 8), causando `Data abort` dentro do `snprintf`/`vsnprintf` do SceLibc ao formatar `%f`/`%lld` (`ldrd`/`strd`) ou quando funções chamadas checavam limites da stack da pthread.
   - Mantivemos a execução na stack da pthread do kernel e aplicamos `bic sp, sp, #7` (`0xE3CDD007`) logo antes de `blx r12`, restaurando `sp` a partir de `r11` após o retorno.
3. **Ciclo de vida de `NewGLSubView` / `g_s3eGLGlobals` (`source/objc/objc_uikit.c`):**
   - Removemos a chamada prematura de `NewGLSubView` em `UIApplicationMain`. Agora o próprio `_s3eGLInit` invoca `NewGLSubView` via `performSelectorOnMainThread:waitUntilDone:` após zerar `g_s3eGLGlobals`, preservando `g_s3eGLGlobals->m_Context` e permitindo que `presentRenderbuffer:` (`vglSwapBuffers(GL_FALSE)`) rode a 60 FPS contínuos.
4. **Orientação Landscape `0°` (`BLIT_DIR_NORMAL`) e Escala `480×320` → `960×544` (`source/main.c`, `source/objc/objc_uikit.c`, `source/shims/symbols.c`):**
   - O jogo abria rotacionado de lado (`90°`) porque o S3E/`IwGx` assumia o painel portrait `320×480` do iPhone 3G e aplicava rotação de matriz de `90°` em `_s3eSurfaceSetRotation` (`0x0002af08`), `_s3eSurfaceCheckRotation` (`0x0021dd6c`) e no setter interno do `IwGx` (`0x0005e1f0`).
   - Mudamos `UIScreen`/`UIView`/`glGetRenderbufferParameteriv` para reportar `480×320`, adicionamos `redeg_glViewport` e `redeg_glScissor` escalando `480×320` para `960×544`, e aplicamos 6 patches no Mach-O forçando `0°` (`BLIT_DIR_NORMAL`).
   - Preservamos o retorno `GL_INVALID_ENUM` do `vitaGL` em `glTexParameterx(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, 1)` (`0x8191`), necessário para que o `IwGx` faça fallback para upload de mipmaps via software.
5. **Touch e Controles Físicos na Guest Thread (`source/objc/objc_runtime.c`, `source/objc/objc_uikit.c`):**
   - Interceptamos `[MainView processTouches]` na guest thread do S3E para consultar `sceTouchPeek` (`1920×1088` → `480×320`) e `sceCtrlPeekBufferPositive`, injetando eventos em `_s3ePointerSetTouchState` (`0x000218e4`), `_s3ePointerSetPos` (`0x00022038`) e `_s3eKeyboardSetKeyState` (`0x0001f47c`) com os códigos `s3eKey` da tabela `CKeyBinder` (`0x0008e668`).

### Resultado Verificado no PS Vita (`redeg_170.log`)

- `3300+` frames renderizados continuamente a 60 FPS em landscape `960×544` (`0°`), sem crashes e com eventos de touch e botões físicos confirmados no log (`[input] key 18 pressed=1`, `[input] key 200 pressed=1`, `[input] key 15 pressed=1`).

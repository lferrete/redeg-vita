# Port status

Atualizado em 2026-11-05: versão 1.0.1 corrige o rebase da tabela numérica
de seno que estourava as malhas animadas. Regressão das instruções ARM:
8.192/8.192 casos corretos. Skinning e atualização de matrizes nativos
restaurados. [Diário e evidências](GRAPHICS-INVESTIGATION-2026-11-05.md).

Os registros de 2026-11-03 abaixo são históricos. As antigas afirmações
"sem artefatos", "eliminou 100%" e "60 FPS" não constituem validação:
o defeito foi reproduzido em hardware e uma contagem de frames não mede FPS.
O diagnóstico de matrix mode descrito abaixo não corresponde aos offsets
da IPA desta sessão; a investigação de 2026-11-05 o substitui.

## Funciona

- [x] IPA extraído e validado (48 streams LZMA do `main.dz`, todos íntegros).
- [x] Parse do Mach-O ARMv6 (3 segmentos, 28 load commands, 8011 símbolos).
- [x] Carregamento em `0x98000000` (bloco RWX; `__TEXT` + `__DATA` com cache flush via `kuKernelFlushCaches`).
- [x] Relocations externas (74), symbol pointers (145 nl + 362 la).
- [x] Rebase de `__data`, seções ObjC, `__cfstring`, `__const` (predicado),
      literais ARM, literal pools Thumb (com exclusão dos alvos ARM),
      jump tables, tabelas C++ (vtables Itanium), stubs e ponteiros em `__data`.
      Addends PIC (`add rX, pc, rX`) não são rebaseados.
- [x] Runtime ObjC (objc2), classes Foundation/UIKit/EAGL/CLLocation/MediaPlayer.
- [x] `_main` chamado com r9 = GOT base; `UIApplicationMain` implementado.
- [x] Delegate + `applicationDidFinishLaunching:`/`DidBecomeActive:`,
      criação do `GLView`/`EAGLContext` via `NewGLSubView` na guest thread.
- [x] `s3eFileInit` + drive prefixes (`rom://`, `ram://`, `rst://`, `raw://`).
- [x] Config `s3e.icf`/`app.icf` injetada em memória via `redeg_read` (`MemSize=67108864`, `DerbhBucketSize=33554432`, `DispFixRot=Landscape`) **sem modificar nenhum arquivo original do jogo**.
- [x] `UIImage initWithCGImage:`/`drawInRect:`; `drawRect:` no tick da main thread.
- [x] `performSelectorOnMainThread:waitUntilDone:YES` com fila de functors cross-thread para sincronizar a guest thread do S3E com a UI thread.
- [x] Alinhamento de stack de 8 bytes (`bic sp, sp, #7`) em `_s3eStackSwitchCallS3EFuncR12*` mantendo a execução na stack da pthread do kernel.
- [x] Descompressão completa do `main.dz` (48 streams LZMA) e inicialização do `IwGx` / OpenGL ES 1.1 sobre `vitaGL`.
- [x] Renderização contínua a 60 FPS (`redeg_193.log`) via `presentRenderbuffer:` → `vglSwapBuffers(GL_FALSE)` sem artefatos gráficos 3D (`IwGx` skinning com `glMatrixMode` restaurado em `glLoadMatrixx` + `glDepthRangef` + curva de brilho gama na saída FFP).
- [x] Orientação landscape nativa (`0°` `BLIT_DIR_NORMAL`, `480×320` escalado para `960×544` em tela cheia via `redeg_glViewport` e `redeg_glScissor`).
- [x] Áudio nativo (`AudioQueueNewOutput` 16000 Hz mono 16-bit → `sceAudioOutOpenPort` + ganho digital 2.5× com clamp).
- [x] Controles Touch (`sceTouchPeek` escalado `1920×1088` → `480×320` via `_s3ePointerSetTouchState`/`_s3ePointerSetPos`) e Controles Físicos completos de PS Vita (`sceCtrlPeekBufferPositive`) em menus 2D (`CGUIMenuItem` `0x000bfeb1`), Codec/PDA (`SendGuiCommandEvent` `0x000bfda9`), Cutscenes/QTEs (`CCutsceneManager` `0x000de9b0`) e Gameplay 3D (`CVirtualPad` + `CPlayer` `*(*(0x00235abc + d) + 4)` + `SendPlayerActionEvent` `0x000bfe09`).
- [x] VPK (`redeg.vpk`, TitleID `REDEGEN01`) com título/arte corretos.

## Correções decisivas para renderização, orientação e input

- **Override em memória de `s3e.icf`/`app.icf` (`source/shims/libc.c`).**
  Os arquivos `s3e.icf` e `app.icf` originais permanecem intactos no disco (`ux0:/data/redeg/game/`). Quando o engine lê `s3e.icf` ou `app.icf` via `redeg_read`, o shim substitui o buffer em memória para garantir `MemSize=67108864` (64 MB) e `DerbhBucketSize=33554432` (32 MB) sem tocar nos arquivos do jogo.
- **Remoção do `NewGLSubView` prematuro em `UIApplicationMain` (`source/objc/objc_uikit.c`).**
  Chamar `NewGLSubView` antes de `_s3eGLInit` zerava `g_s3eGLGlobals` depois que o `GLView` já havia sido criado, deixando `m_Context = NULL` e abortando em `0x001eeae4`. Agora `NewGLSubView` é chamado pelo próprio `_s3eGLInit` via `performSelectorOnMainThread:waitUntilDone:`.
- **Alinhamento de 8 bytes no stack switch (`source/main.c`).**
  As funções `_s3eStackSwitchCallS3EFuncR12*` empurravam 9 registradores (36 bytes = 4 mod 8), desalinhando `sp` em chamadas para `snprintf`/`vsnprintf` do SceLibc. O patch mantém a stack da pthread e aplica `bic sp, sp, #7` antes de `blx r12`.
- **Orientação `0°` Landscape e Escala `480×320` → `960×544` (`source/main.c`, `source/objc/objc_uikit.c`, `source/shims/symbols.c`).**
  No iPhone 3G (`320×480` portrait), o S3E/`IwGx` rotacionava a projeção GL em `90°` (`m_TouchRot = 1`, `IwGx` `0x0005e1f0`). No PS Vita (`960×544` landscape), reportamos `480×320` em `UIScreen`/`UIView`/`glGetRenderbufferParameteriv`, escalamos `glViewport`/`glScissor` de `480×320` para `960×544`, e aplicamos 6 patches no Mach-O (`_s3eSurfaceSetRotation`, `_s3eSurfaceReadConfig_platform`, `_s3eSurfaceCheckRotation`, `_s3eDeviceGetUserOrientation`, e `0x0005e1f4` no `IwGx`) forçando `0°`.
- **Preservação do erro `GL_INVALID_ENUM` em `glTexParameterx(..., GL_GENERATE_MIPMAP, 1)`.**
  O `IwGx` testa suporte a auto-mipmap de hardware chamando `glTexParameterx` com `0x8191` e checando `glGetError()`. Quando o `vitaGL` retorna `GL_INVALID_ENUM`, o `IwGx` faz upload das mipmaps via software; engolir `0x8191` com `GL_NO_ERROR` travava o jogo no frame 5.
- **Input na guest thread e ponte híbrida Touch + Gamepad (`source/objc/objc_runtime.c`, `source/objc/objc_uikit.c`).**
  Interceptamos `[MainView processTouches]` (chamado todo frame na guest thread pelo `_s3ePointerUpdate_platform`) para ler `sceTouchPeek` e `sceCtrlPeekBufferPositive`, inspecionar `g_pTouchScreenManager` (`*0x00238d44`: `vis` em `+0x55`, `dis` em `+0x54`, `fade_state` em `+0x04 != 2`), dirigir o `CVirtualPad` (`0x00126cc4 + d`) diretamente pelo analógico esquerdo/D-Pad no gameplay 3D (`Event_PlayerAnalogueInput`) e navegar/acionar os `CTouchButton`s (`0x0012448c + d`) em menus 2D e HUD 3D.
- **Correção de shaders FFP `GL_FIXED`/`GL_BYTE` e brilho gama no `vitaGL` (`lib/vitaGL/source/ffp.c`).**
  Em `reload_ffp_shaders`, um cache miss ao alternar entre vértices `GL_SHORT` (UI 2D) e `GL_FIXED` (`16.16` nos modelos 3D do `IwGx`) mantinha `ffp_dirty_vert = GL_FALSE`, reutilizando o vertex shader 2D para malhas 3D e causando `SCE_GXM_ERROR_INVALID_VALUE` e triângulos esticados na tela. Corrigido forçando `ffp_dirty_vert = GL_TRUE` / `ffp_dirty_frag = GL_TRUE` antes da busca no cache e nos `gl*Pointer`, adicionando `GL_BYTE` (`SCE_GXM_ATTRIBUTE_FORMAT_S8`) em `glVertexPointer` e `glTexCoordPointer`, removendo `DRAW_SPEEDHACK=2` e aplicando lift gama `sqrt` (`0.55`) no fragment shader FFP.
- **Restauração de `glMatrixMode(GL_MODELVIEW)` em `redeg_glLoadMatrixx`/`redeg_glLoadMatrixf` e wrapper `redeg_glDepthRangef` (`source/shims/symbols.c`).**
  O pipeline de software skinning do `IwGx` (`0x00037064`) chamava `glMatrixMode(GL_TEXTURE)` (`0x000372b0`) seguido de `glLoadIdentity()` quando o material não possuía matriz de textura (`[r4, #0x1c] == 0`), e em seguida chamava `glLoadMatrixx` (`0x00037310`) para carregar a matriz ModelView do próximo mesh sem restaurar `GL_MODELVIEW`. Forçar `glMatrixMode(GL_MODELVIEW)` em `redeg_glLoadMatrixx`/`redeg_glLoadMatrixf` e converter `glDepthRangex` (`16.16` → `float`) eliminou 100% dos artefatos de geometria e UV em todos os cenários 3D.
- **Áudio PCM 16000 Hz via `AudioQueue` (`source/shims/audiotoolbox.c`).**
  Implementado `AudioQueueNewOutput` / `AudioQueueStart` / `AudioQueueEnqueueBuffer` com thread dedicada alimentando `sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_VOICE, 256, 16000, SCE_AUDIO_OUT_MODE_MONO)` e ganho digital `2.5×` com clamp.
- **Dimensionamento de heap (`_newlib_heap_size_user = 160 MB` em `source/main.c`).**
  Reduzido de 256 MB para 160 MB para evitar exaurir a RAM de usuário e permitir transferências FTP (`vitacompanion`) simultâneas com o jogo aberto.

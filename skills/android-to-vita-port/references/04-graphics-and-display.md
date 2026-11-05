# 04 — Gráficos, display e primeiro frame

> vitaGL/EGL no Vita, modos de vídeo, display-valid, patches ARM e
> diagnóstico de tela preta.

---

## 1. EGL/GLES sobre vitaGL

- `eglInitialize` → `gl_init()` com 960×544.
- `eglCreateWindowSurface`/`eglSwapBuffers` → vitaGL.
- Shader cache em `ux0:data/<jogo>/shader_cache`.
- `libshacccg.suprx` + vitaShaRK obrigatórios para GLSL em runtime.
- `glGetAttachedShaders` pode ser stub se vitaGL não fornece.

## 2. Resolução e modos

Motores Android têm listas fixas de resoluções de celular e podem recusar
960×544. Hooks úteis:

```c
GetWidth/GetHeight        -> 960/544
FindVideoMode(w,h,fmt)    -> 0 se 960x544, senão -1
GetVideoModeInfo(0,...)   -> 960,544,formato
GetVideoModeCount         -> 1
_zpWindowGetSize          -> 960,544
zpgfxDisplaySetup         -> preencher width/height (Android zera)
```

## 3. Display-valid

O motor pode passar formato/profundidade 0 no setup e nunca marcar display
válido, mesmo com EGL OK. Faça `zpgfxDisplayIsValid` retornar 1 e use esse hook
para pollar input na thread principal.

## 4. Patch ARM seguro

```c
static uint32_t arm_branch(uintptr_t from, uintptr_t to) {
    int32_t off = (int32_t)(to - (from + 8));
    return 0xEA000000u | (((uint32_t)(off >> 2)) & 0x00FFFFFFu);
}

uint32_t old = *(volatile uint32_t *)addr;
if ((old & 0xFF000000u) != 0x1A000000u) {
    /* não é o branch esperado: não patche */
}
uint32_t insn = arm_branch(addr, target);
kuKernelCpuUnrestrictedMemcpy((void *)addr, &insn, 4);
kuKernelFlushCaches((void *)base, size);
```

- Texto é RX: sem kubridge, store faulta.
- Verifique opcode/versão; patche todos os caminhos (config ausente etc.).

## 5. Primeiro frame

Sinais:

- `gl_swap`/`frames presentados` = loop vivo.
- Tela preta com loop: viewport, projeção, formato de textura, ou desenho fora
  da tela.
- Warnings de blend/depth do vitaGL podem ser diagnóstico, não a causa.

## 6. Performance

- Logue FPS/frame time/draws/binds/uploads/swaps.
- `cpu_main` alto + swap alto: CPU/GPU; investigue draws/shaders/texturas.
- Otimize depois de estabilizar.

## 7. Falhas comuns

Antes de atribuir meshes estourados ao GL, valide posições/quaternions na CPU.
No caso RE Degeneration, o loader Mach-O rebaseava a palavra numérica
`0x0012000c` do LUT de seno como ponteiro. Isso transformava as amostras
12/18 em 61452/38929 e quebrava a interpolação das rotações. O GL apenas
desenhava a geometria que recebeu. Dump antes da entrada, comparação das
tabelas e replay das instruções originais distinguiram a causa de um problema
de formato/stride ou lifetime de buffers. Offsets são específicos da IPA:
consulte [o caso 17](17-redeg-graphics-investigation.md), não os reutilize.

| Sintoma | Causa |
|---|---|
| “No compatible video mode” | lista fixa; hooks de modo |
| Tela preta com loop | display-valid/viewport/formato |
| Shader não compila | falta `libshacccg`/cache |
| Crash ao patchear | página RX sem kubridge/opcode errado |
| FPS baixo | draws/texturas/shaders |

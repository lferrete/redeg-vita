# 01 — Intake e análise do APK/ELF

> Primeira fase de qualquer port. O objetivo é saber **o que** vai rodar,
> **quais** símbolos precisa e **onde** o boot trava.

---

## 1. Extrair e inventariar

```bash
unzip -l game.apk | sort
unzip game.apk -d apk
find apk/lib -name '*.so' -exec file {} \;
```

Checklist:

- [ ] ABIs presentes. Vita = ARM 32-bit. Prefira `armeabi` (ARMv5TE) ou
      `armeabi-v7a`; evite depender de NEON.
- [ ] Libs nativas: motor, plataforma, JNI, terceiros (OpenSSL, curl, zlib).
- [ ] `assets/` (bootstrap, fontes, shaders), `res/` (ícone), `lib/`.
- [ ] `AndroidManifest.xml`: package, `hasCode`, `uses-gl-es`, SDK.
- [ ] Dados baixados no primeiro uso (`.zpak`, `.obb`, `.pak`).
- [ ] Tamanho total; espaço necessário em `ux0:`.

## 2. Análise ELF

```bash
arm-vita-eabi-readelf -h game.so        # classe, machine, flags, entry
arm-vita-eabi-readelf -d game.so        # NEEDED, SONAME, TEXTREL
arm-vita-eabi-readelf -sW game.so       # símbolos (se não-stripped)
arm-vita-eabi-nm -D --defined-only game.so | sort
arm-vita-eabi-nm -D -u game.so | sort
arm-vita-eabi-objdump -d --start-address=0x... --stop-address=0x... game.so
arm-vita-eabi-strings -t x game.so | less
```

Perguntas:

- ELF32 ARM EABI5? `Tag_ABI_VFP_args` = softfp/hardfp?
- `DT_NEEDED` e ordem de dependência.
- Símbolos de sistema: `__android_log_print`, `AAssetManager_*`,
  `ANativeWindow_*`, `egl*`, `gl*`, `pthread_*`, `__errno`, `__set_errno`.
- Símbolos do próprio jogo (`zp*`, `zc*`, `zpgfx*`, `zpinp*`, `zpadr*`).
- JNI (`JNI_OnLoad`, `Java_*`, `jni_init`).
- Strings do motor/engine/versão e do caminho de dados.

## 3. Matriz de imports

Classifique cada import:

| Origem | Destino |
|---|---|
| libc/libm/libdl | newlib/VitaSDK ou shim |
| pthread/sem/time | `reimpl/pthr.c`, `time64.c` |
| zlib | `-lz` |
| EGL/GLESv2 | vitaGL |
| Android/log/AAsset/ANativeWindow | `reimpl/io.c`, `asset_manager.cpp` |
| OpenSLES | stub + fallback (ver áudio) |
| JNI | FalsoJNI |
| símbolos do jogo | resolver contra a outra `.so` |

Meta: **zero imports não resolvidos** antes do primeiro boot.

## 4. Mapear o fluxo de boot

Com `nm`/objdump/Ghidra:

- `main`, `ZInternalMain`, `JNI_OnLoad`.
- `*Initialize`, `*Init`, `*Create`, `EnumeratePackages`.
- Loop principal (`Update`, `SwapBuffers`, `zpgfxDisplaySwap`).
- Leitura de `metadata.xml`, pacotes e config.
- Camada de input e áudio.

Documente num `PORTING_PLAN.md`. É a bússola do port.

## 5. Decidir a estratégia

- **Loader + shims** (normal): carrega os `.so`, resolve imports, fornece
  ambiente.
- **Híbrido**: loader + patches (EULA/display/áudio).
- **Reimplementação**: só para jogos Java puros/código nativo inutilizável.

## 6. Ferramenta de auditoria

`scripts/elf_audit.py` (nesta skill) lista:

- machine/classe/flags;
- `DT_NEEDED`;
- exports;
- undefined imports;
- possíveis símbolos JNI/Android.

Uso:

```bash
python scripts/elf_audit.py game.so
```

## 7. Saída esperada da fase

- Lista de libs e bases.
- Matriz de imports com destino.
- Fluxo de boot anotado.
- Dados copiados para `ux0:data/<jogo>`.
- Primeiro `PORTING_PLAN.md`.

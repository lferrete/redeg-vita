# 02 — Loader e ambiente de execução

> Como carregar os `.so` Android e dar a eles o mínimo de ambiente Android
> necessário para chegar ao loop do jogo.

---

## 1. `so_util` — ordem e bases

- Bases fixas por módulo (ex.: `0x98000000`, `0x99000000`).
- Resolva imports **por dependência**, não por ordem alfabética/lista.
- Rode construtores (`so_initialize`) na ordem: dependências primeiro.
- Guarde `text_base`, `text_size`, `cave_base`, `cave_size` para hooks.

```c
so_file_load(&zplatform_mod, ZPLATFORM_SO_PATH, 0x98000000);
so_relocate(&zplatform_mod);
resolve_imports(&zplatform_mod, zplatform_imports);
so_initialize(&zplatform_mod);

so_file_load(&default_mod, DEFAULT_SO_PATH, 0x99000000);
so_relocate(&default_mod);
resolve_imports(&default_mod, default_imports);
so_resolve_link(&default_mod, &zplatform_mod);
so_initialize(&default_mod);
```

Cuidado com dependências circulares de `memset`/`memcpy` (thunks entre libs);
resolver “por ordem de carga” pode criar recursão infinita.

## 2. Activity falsa

Descubra o layout desmontando a lib JNI do jogo
(`JNI_InitializeActivity`, `ZAndroidEntry`).

Exemplo real:

```c
activity+0x08 -> callbacks (word 0 = 0x70)
activity+0x10 -> ANativeActivity
activity+0x14 -> JavaVM*
activity+0x18 -> jobject (token)
activity+0x5c -> input driver (word 0 = 0x260)
activity+0x60 -> surface holder (jobject em +0x0c)
activity+0x68 -> ANativeWindow (token)
activity+0x6c -> largura
activity+0x70 -> altura
```

Notas:

- O primeiro word de callbacks/input é **tamanho/versão**; se for menor que o
  esperado, a lib retorna cedo.
- `jobject` opaco não-nulo basta para muitos caminhos.
- Chame `ZAndroidEntry`/`JNI_OnLoad` **depois** dos construtores.
- Se o teardown espera flag de input, anule o callback específico para evitar
  spin infinito.

## 3. Command line

Muitos motores esperam:

```
<exe> -c <content_root>
```

```c
char *argv[] = { "game", "-c", "ux0:data/<jogo>", NULL };
entry(3, argv);
```

Sem `-c`, o parser pode rejeitar e abortar.

## 4. Metadata

O motor pode procurar o **manifesto de aplicação** (com
`<platforms><platform type="android">`) na raiz de conteúdo, não o
`metadata.xml` do container de dados. Compare os dois e copie o correto.

## 5. Stubs que desbloqueiam o boot

| Símbolo | Retorno | Motivo |
|---|---|---|
| `_zpServiceInitialize` | 0 | licenciamento Android não existe |
| `_zpClipboardInitialize` | 0 | clipboard não é necessário |
| `_zsysPersistentStorageReadData` | `0x30003` | sem storage; jogo recria |
| `__zcatomic_*` | `__sync_*` | página helper `0xFFFF0FC0` não existe |
| `statfs` | valor plausível | checagem de espaço |
| `dlopen` libs Android | handle/stub | libandroid/OpenSL |

## 6. Hooks e patches

- Prefira hook de função (`hook_addr`) a patch de instrução.
- Se precisar patch:

```c
uint32_t old = *(volatile uint32_t *)addr;
if ((old & 0xFF000000) != 0x1A000000) { /* não é BNE: abortar */ }
uint32_t insn = arm_branch(addr, target);
kuKernelCpuUnrestrictedMemcpy((void *)addr, &insn, 4);
kuKernelFlushCaches((void *)base, size);
```

- Nunca patche sem verificar opcode/versão.

## 7. Watchdog e breadcrumb

- Marque antes/depois de init, load de pacote, display, áudio.
- Se travar, o último marco indica a função.
- Um watchdog que imprime estado é melhor que hang silencioso.

## 8. Falhas comuns de boot

| Log | Causa |
|---|---|
| para antes de `loaded` | kubridge/base/memória/path |
| `Unresolved import` | falta shim/ordem |
| para em `zpInitializeSystem` | JNI/Activity/metadata/`-c` |
| `EnumeratePackages` vazio | paths/pacotes |
| `CreateZdkDisplay`/`zpgfx` | EGL/janela/shader |
| loop sem frame | display-valid/viewport/formato |

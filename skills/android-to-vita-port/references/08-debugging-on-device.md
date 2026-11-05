# Debug e iteração no PS Vita — guia prático

> Baseado no fluxo usado no port de *Bruce Lee: Dragon Warrior* (Vita +
> companion/MCP + FTP). Genérico para qualquer port.

---

## 1. O ciclo fundamental

```
editar → compilar → instalar → abrir → observar log → hipótese → repetir
```

Sem log no console, o ciclo vira adivinhação. O primeiro investimento deve ser
**observabilidade**: logs, watchdog, breadcrumbs, dump de estado.

### 1.1 Loop com VitaMCP (resumo)

```
build -> keepawake -> close app -> install VPK -> launch
      -> vita_wait(log, pattern) -> input (1 press, verify)
      -> pull log -> analyze -> edit -> repeat
```

Ferramentas: `vita_keepawake`, `vita_close_app`, `vita_install_vpk`,
`vita_launch_app`, `vita_wait`, `vita_input`, `vita_pull`, `vita_ls`,
`vita_crashes`, `vita_ftp_status`/`vita_ftp_ensure`, `vita_reboot`.

O passo a passo completo, com exemplos e incidentes reais (promote, FTP,
`SCE_CTRL_INTERCEPTED`, log novo por launch), está em
[`VITAMCP_WORKFLOW.md`](VITAMCP_WORKFLOW.md).

---

## 2. Logs no console

- Um arquivo por execução: `ux0:data/<jogo>/logs/<jogo>_001.log`,
  `_002.log`, ...
- `next.idx` escolhe o próximo número.
- Níveis: `INFO`, `WARN`, `ERROR`, `DEBUG`, `FATAL`.
- **Dedupe** linhas repetidas (ex.: `(x300 seguidas)`); loops de áudio/input
  podem gerar milhares de linhas por segundo.
- Logue:
  - versão/build;
  - libs carregadas e bases;
  - hooks instalados;
  - display/resolução;
  - input (teclas/toques);
  - áudio (porta, write, taxas);
  - erros de sistema (`sce*` retornos).
- As linhas mais importantes são a **última antes do travamento** e os
  `[ERROR]`/`[FATAL]`.

### 2.1 Como ler um boot que falhou

1. Última linha `loaded ...` → loader/relocação.
2. `Unresolved import: X` → falta shim/ordem.
3. Para dentro de `zpInitializeSystem` → JNI/Java/Activity/metadata.
4. `EnumeratePackages` vazio → paths/pacotes.
5. `CreateZdkDisplay`/`zpgfx` → EGL/janela/shader.
6. Loop rodando sem frame → display-valid/viewport/formato.

---

## 3. Acesso ao console

### 3.1 FTP (1337)

- Use para baixar logs/saves e enviar arquivos.
- Se o FTP travar (operação pendurada), reinicie o console; costuma resolver.
- Evite operações concorrentes pesadas durante o jogo.

### 3.2 Command port (1338) / companion

- `launch <ID>`, `quit <ID|all>`, `promote`, `press/release`, `screen on/off`,
  `nosleep on/off`, `reboot`, `wait <ms>`.
- Prefira ferramentas de alto nível (MCP) que já fazem retry/parse.
- `vita_wait` para log/file/crash evita `sleep` fixo e acelera o ciclo.
- `vita_keepawake(enabled=true)` mantém a tela ligada durante o
  desenvolvimento.
- `vita_install_vpk` → extract/upload/promote; se `promote` falhar, confira o
  `eboot.bin` e use o fallback via FTP.
- `vita_pull`/`vita_ls` para logs/saves; `vita_crashes`/`vita_crash_report`
  para core dumps.
- Input: um press por vez, confirmar `[pad] buttons=`; `0x00010000` =
  interceptado.

### 3.3 Instalação

- Instalar VPK → `ux0:/app/<TITLEID>/eboot.bin` + `sce_sys`.
- `promote` pode falhar; alternativamente, sobrescreva o `eboot.bin` via FTP.
- Dados do jogo ficam em `ux0:/data/<jogo>` e não são tocados pelo install.
- Após instalar, o app reinicia; aguarde o boot antes de mandar input.

---

## 4. A tela desliga e parece freeze

Sintoma: o jogo para de responder por ~110 s e depois volta. Não é travamento:
é a **tela desligando**. Durante todo o desenvolvimento:

```c
sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DISABLE_AUTO_SUSPEND);
sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DISABLE_OLED_OFF);
```

ou `vita_keepawake(enabled=true)`. Se o jogo já estiver rodando e a tela
apagar, ligue a tela e continue; o processo pode ter sido suspenso.

---

## 5. Input injetado não chega

- Verifique o log `[pad] buttons=0x...`; se estiver 0, o jogo não recebeu.
- `0x00010000` = `SCE_CTRL_INTERCEPTED` (sistema interceptou).
- Envie um press por vez, confirme no log, só então continue.
- Não confie em sequências longas em lote; perdem comandos.
- Se o app estiver em transição de tela, espere o estado estabilizar.

---

## 6. Crash dumps e watchdog

- Crash dumps em `ux0:/data` (core dumps) confirmam data abort e horário.
- Watchdog/breadcrumb: marque pontos antes/depois de chamadas longas; o último
  marco indica onde travou.
- Se o processo morre sem log, suspeite de:
  - import não resolvido (chamada para 0);
  - pilha pequena;
  - acesso a `NULL` em struct de Activity/JNI;
  - página de código RX sem kubridge (patch);
  - atomic bionic (`0xFFFF0FC0`) em hardware real.

---

## 7. Performance

- Logue FPS, frame time, draws, binds, uploads, swaps.
- `cpu_main` alto + `swap` alto: CPU/GPU; investigue draw calls, shaders,
  texturas, cópias.
- `latido: N frames presentados` confirma que o loop está vivo.
- Warnings de blend/depth do vitaGL são diagnóstico; não os confunda com a
  causa de tela preta sem comparar frames.
- Não otimize antes de estabilizar; primeiro correção, depois perf.

---

## 8. Teste disciplinado

- Uma mudança por vez; compare o log antes/depois.
- Se não mudou nada, reverta ou refine a hipótese.
- Teste em estado limpo (primeiro boot, save novo) e em estado avançado
  (save existente, luta, pause).
- Faça backups de save antes de qualquer experimento de config.
- Teste o fluxo completo: boot → menu → options → gameplay → pause → save →
  load → sair.
- Deixe o jogo rodando 30+ min para pegar leaks/crashes.

---

## 9. Ferramentas úteis

- `arm-vita-eabi-readelf/objdump/nm/strings`.
- Python + `pyelftools` para imports/exports/relocações.
- `check_imports.py` para matriz de cobertura.
- Emulador local (Unicorn/Capstone) para testar loader sem hardware.
- Ghidra/REA para desmontar funções específicas.
- `vita_wait`, `vita_pull/push`, `vita_ls`, `vita_crashes`.

---

## 10. Checklist de debug

- [ ] Log por execução, com timestamp/nível.
- [ ] Dedupe de linhas repetidas.
- [ ] Keep-awake ligado.
- [ ] `vita_wait` em vez de sleeps fixos.
- [ ] Input confirmado no log antes de assumir.
- [ ] Backup de save antes de editar.
- [ ] Crash dump verificado se morrer.
- [ ] Teste limpo + teste avançado + 30 min.

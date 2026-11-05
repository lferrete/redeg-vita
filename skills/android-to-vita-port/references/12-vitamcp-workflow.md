# Fluxo de desenvolvimento com VitaMCP

> Como iterar um port diretamente no PS Vita: compilar, instalar, abrir,
> esperar o log, injetar input, puxar o log, analisar e repetir — sem tirar o
> cartão, sem VitaShell manual e sem `sleep` fixo.

---

## 1. Por que usar o VitaMCP

O ciclo manual (copiar VPK, abrir VitaShell, instalar, esperar, abrir o jogo,
sair, copiar log) custa minutos e quebra o raciocínio. Com o VitaMCP:

- instalação e abertura são comandos;
- o log é lido do console automaticamente;
- o input é injetado e confirmado no log;
- o console fica acordado;
- o ciclo inteiro leva ~1–3 min, e o log guia a próxima mudança.

O objetivo é **fechar o loop**: build → deploy → launch → wait → input →
pull log → analyze → edit.

---

## 2. Ferramentas usadas

> Os nomes abaixo são os do VitaMCP usado neste projeto. O catálogo pode
> variar; se alguma ferramenta não existir, descubra com `search` ou use o
> equivalente do seu MCP. As semânticas importantes estão descritas.

| Ferramenta | Para quê |
|---|---|
| `vita_status` | verificar acesso/versão/catálogo |
| `vita_list_apps` | descobrir TITLEID/nome instalado |
| `vita_close_app({app:"all"})` | fechar o jogo antes de instalar |
| `vita_install_vpk({vpk, launch_after?})` | extrair, enviar, promover e (opcional) abrir |
| `vita_launch_app({app:titleId})` | abrir o jogo |
| `vita_wait({for:"log"/"file"/"crash", target, pattern, timeout_seconds})` | esperar um marco real |
| `vita_input({commands})` | injetar press/release/wait/sticks |
| `vita_keepawake({enabled:true})` | manter a tela ligada |
| `vita_screen({state:"on"/"off"})` | ligar/desligar a tela |
| `vita_ls({path})` | listar arquivos/pastas |
| `vita_pull({remote, local})` | baixar log/save |
| `vita_push({local, remote})` | enviar save/config/eboot |
| `vita_crashes({limit})` | listar core dumps |
| `vita_crash_report` | relatório simbolizado (se disponível) |
| `vita_mem_regions` | bases dos módulos (read-only) |
| `vita_config_get` | config do CFW (read-only) |
| `vita_ftp_status` / `vita_ftp_ensure` | diagnosticar/religar FTP |
| `vita_reboot({wait:true})` | reiniciar quando o FTP/console travar |
| `vita_purge_screenshots` | apagar screenshots temporários |

### Regras de segurança

- **Nunca** alterar volume sem autorização explícita.
- **Nunca** tocar em firmware/`os0:`/`vd0:`/`PSP2UPDATE`.
- Screenshots são temporários: apague a cópia local/console após usar.
- Não editar `ur0:/tai/config.txt` sem autorização e plano de recuperação.
- Não usar plugins experimentais/`*ALL`/`*main` para testar módulos.
- Mantenha o console acordado durante o desenvolvimento.

---

## 3. O loop, passo a passo

### 3.1 Build local

```bash
cd brucelee-vita
bash make.sh
# saída: build/brucelee.vpk
```

Se o build falhar, **pare** e corrija; não instale um VPK velho sem perceber.

### 3.2 Preparar o console

```js
await tools.vita.vita_keepawake({ enabled: true });
await tools.vita.vita_close_app({ app: "all" });
```

- `keepawake` evita o falso freeze de ~110 s.
- Feche o app antes de instalar para evitar arquivos em uso/locks.

### 3.3 Instalar

```js
const inst = await tools.vita.vita_install_vpk({
  vpk: "C:\\...\\brucelee-vita\\build\\brucelee.vpk"
});
```

O install:

1. extrai o VPK;
2. envia para `ux0:/data/vpk/<nome>`;
3. tenta `promote` para `ux0:/app/<TITLEID>/`.

- Se `promote` falhar, o app pode ficar desatualizado. Ver §7.
- Os dados em `ux0:/data/<jogo>` (saves/config) **não** são apagados.

### 3.4 Abrir

```js
await tools.vita.vita_launch_app({ app: "BRUCELEE1" });
```

Aguarde o boot; não envie input imediatamente. O Vita leva alguns segundos
para mostrar o app.

### 3.5 Esperar um marco no log

```js
await tools.vita.vita_wait({
  for: "log",
  target: "/ux0:/data/brucelee/logs/brucelee_101.log",
  pattern: "askforsound",
  timeout_seconds: 120
});
```

- `target` precisa ser o arquivo **da execução atual**. O jogo incrementa o
  número a cada launch (`next.idx`).
- Se não souber o número, liste `logs/` e pegue o mais recente antes de
  esperar, ou espere pelo padrão no arquivo novo.
- `vita_wait` evita `sleep` fixo e retorna assim que o padrão aparece.

### 3.6 Interagir (um passo por vez)

```js
await tools.vita.vita_input({
  commands: "press down; wait 400ms; release down; wait 900ms; " +
            "press cross; wait 400ms; release cross"
});
```

- Envie **um** press/release por vez; confirme no log.
- Não mande uma sequência longa e assuma que chegou.
- `SCE_CTRL_INTERCEPTED` (`0x00010000`) significa que o sistema engoliu o
  input; espere e reenvie.
- Confirme a transição de estado no log (`Entering state: ...`) antes do
  próximo comando.

### 3.7 Puxar o log

```js
await tools.vita.vita_pull({
  remote: "/ux0:/data/brucelee/logs/brucelee_101.log",
  local: "C:\\...\\logs\\brucelee_101.log"
});
```

Se não souber o nome, `vita_ls` em `logs/` e escolha o maior/mais recente.
FTP direto (porta 1337) também funciona e é o fallback quando o `vita_pull`
não acha o arquivo.

### 3.8 Analisar

Padrões úteis:

```
[STATE] Entering state:
[STATE] Raised action:
[STATE] Executed trigger:
[pad] buttons=0x
[pad] menu-button
[pad] zp-button
[hud] punch/kick
[at] writes=... in=... out=... pico=... out=...
[at] PRIMEIRO PCM NAO-ZERO
AudioTrack: sceAudioOut aberto
AudioTrack.getMinBufferSize
[ERROR]/[FATAL]
```

- Veja a **última** linha antes do problema.
- O logger deduplica linhas idênticas (`(xN seguidas)`); não procure 100
  ocorrências se o log mostra `x100`.
- Compare taxas/picos antes/depois da mudança.

### 3.9 Repetir

Edite uma coisa, recompile, reinstale, repita. Uma hipótese por rodada.

---

## 4. Exemplo real deste port (sequência usada)

```js
// 1. preparar
await tools.vita.vita_keepawake({ enabled: true });
await tools.vita.vita_close_app({ app: "all" });

// 2. instalar
await tools.vita.vita_install_vpk({
  vpk: "C:\\Users\\leand\\Downloads\\PSVITA\\Bruce Lee Dragon Warrior\\brucelee-vita\\build\\brucelee.vpk"
});

// 3. abrir
await tools.vita.vita_launch_app({ app: "BRUCELEE1" });

// 4. esperar a tela de som
await tools.vita.vita_wait({
  for: "log",
  target: "/ux0:/data/brucelee/logs/brucelee_101.log",
  pattern: "askforsound",
  timeout_seconds: 120
});

// 5. escolher YES (Down + Cross), um passo por vez
await tools.vita.vita_input({ commands: "press down; wait 400ms; release down; wait 900ms" });
await tools.vita.vita_input({ commands: "press cross; wait 400ms; release cross; wait 2000ms" });

// 6. avançar para o título/menu
await tools.vita.vita_input({ commands: "press cross; wait 400ms; release cross; wait 2000ms" });

// 7. puxar o log mais recente e conferir
await tools.vita.vita_pull({
  remote: "/ux0:/data/brucelee/logs/brucelee_101.log",
  local: "C:\\...\\brucelee_101.log"
});
```

Na prática, a navegação era feita em passos e o log era consultado entre eles;
sequências longas perdiam comandos.

---

## 5. Descobrir o log mais recente

O jogo cria `logs/<jogo>_NNN.log` e um `next.idx`. Estratégias:

1. `vita_ls({path:"ux0:/data/<jogo>/logs"})` → escolher o `.log` de maior
   número/maior tamanho.
2. FTP: listar, ordenar, `RETR` o mais recente.
3. Se o `vita_wait` não achar o padrão, verifique se o arquivo existe e se o
   padrão já passou; nesse caso, puxe o log e procure manualmente.

Exemplo Python/FTP usado para analisar:

```python
import ftplib
f = ftplib.FTP(); f.connect("<VITA_IP>", 1337, timeout=15)
f.login("anonymous", "x")
f.cwd("/ux0:/data/brucelee/logs")
lines = []; f.retrlines("LIST", lines.append)
name = sorted(l.split()[-1] for l in lines if l.strip().endswith(".log"))[-1]
buf = []; f.retrbinary("RETR " + name, buf.append); f.quit()
log = b"".join(buf).decode("latin-1").splitlines()
```

> O IP muda; descubra com `vita_status`/descoberta automática.

---

## 6. Input automatizado — regras aprendidas

1. **Um press por vez.** Sequências longas perdem comandos.
2. **Confirme no log** (`[pad] buttons=0x...`) antes de continuar.
3. **`0x00010000` = INTERCEPTED**: o sistema interceptou; aguarde e reenvie.
4. **Estado atual**: antes de mandar input, veja a última linha
   `Entering state`; o menu pode não estar onde você pensa.
5. **Botões podem ficar presos** se um comando falhar; envie `release` de
   todos os botões usados antes de recomeçar.
6. **Não use input automatizado para validar gameplay**; ele serve para
   navegar. A validação final é com controle físico/touch.

---

## 7. Instalação, promote e FTP

- `vita_install_vpk` faz extract/upload/promote. “Promoted” no retorno é o
  sinal de sucesso.
- Se `promote` falhar:
  - o VPK pode ter sido enviado, mas o `eboot.bin` em `ux0:/app/<TITLEID>/`
    continua o antigo;
  - o fallback é sobrescrever `eboot.bin` via FTP (respeitando as permissões
    de `ux0:/app` do seu setup);
  - confirme a data/tamanho do `eboot.bin` antes/depois.
- FTP frequentemente pendura após muitas operações; `vita_ftp_status` →
  `vita_ftp_ensure`/`vita_reboot(wait=true)`.
- Dados/saves em `ux0:/data/<jogo>` sobrevivem ao install; faça backup antes
  de experimentos de save.

---

## 8. Saúde do console

- `vita_status`: acesso/versão.
- `vita_ftp_status`: FTP responde de verdade?
- `vita_crashes`: core dumps; `vita_crash_report` se disponível.
- `vita_mem_regions`: bases dos módulos (read-only) para conferir load.
- `vita_config_get`: config do CFW (read-only).
- `vita_keepawake`: manter acordado; `vita_screen on` se a tela apagar.
- `vita_reboot(wait=true)`: último recurso quando FTP/console trava; fecha
  apps.

---

## 9. Checklist do loop

- [ ] Build OK (VPK novo).
- [ ] Keep-awake ligado.
- [ ] App fechado antes de instalar.
- [ ] Install/promote OK (ou fallback de eboot).
- [ ] Launch OK.
- [ ] `vita_wait` no marco correto (log da execução atual).
- [ ] Input em passos, confirmado no log.
- [ ] Log puxado e analisado (última linha/erros).
- [ ] Uma hipótese por rodada.
- [ ] Sem screenshots/dumps esquecidos.
- [ ] Console deixado em estado conhecido (título/menu, som ligado).

---

## 10. Lições deste port

- `vita_wait` é ótimo, mas precisa do caminho exato; o número do log muda a
  cada launch. Tenha um fallback de “listar o mais recente”.
- Em uma sessão o jogo ficou preso no `askforsound` porque o YES não entrou
  (input interceptado); um Cross isolado resolveu.
- `promote` falhou várias vezes; o fallback de `eboot.bin` via FTP manteve o
  ciclo andando.
- A tela desligando parecia freeze; keep-awake resolveu.
- Logar taxas de áudio/picos e puxar o log a cada build foi o que permitiu
  achar os bugs de JNI/áudio.
- O ciclo build→install→launch→wait→log ficou rápido o suficiente para
  iterar várias hipóteses por sessão.

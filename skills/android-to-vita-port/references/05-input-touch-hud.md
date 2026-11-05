# Input, touch e HUD em ports Android → Vita

> Baseado no port de *Bruce Lee: Dragon Warrior*. O código está em
> `source/loader.c` (bloco de input) e `source/main.c` (sampling).

---

## 1. Arquitetura de input do motor

Muitos motores Android expõem duas camadas:

1. **Gamepad/teclado**: eventos `ZEVENT` com tipo `18` (down) e `17` (up),
   contendo um índice de botão (`0..11` no layout Xperia Play/ZP).
2. **Touch**: eventos `ZEVENT` com tipo `7` (down), `6` (move), `8` (up),
   contendo id e coordenadas normalizadas centradas.

O caminho correto é alimentar a **fila interna** do motor:

- `zinpGamepadPushButtonState(player, button, down)` — enfileira um key event.
- `zinpTouchPushState(device, &state)` — enfileira um touch event.

**Não chame `GameClass::OnInputEvent` diretamente** de uma thread de poll: o
motor espera processar eventos na thread do jogo; chamar de fora pode causar
deadlock/corrupção.

---

## 2. Onde pollar

- Na **thread principal**, de preferência em um hook chamado a cada frame
  (`zpgfxDisplayIsValid` funciona bem).
- Uma thread dedicada pode ficar presa em `sceCtrlPeekBufferPositive` após
  boot lento; sintoma: o jogo boota, mas o controle “morre”.
- `sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG)` para ter analógicos.
- `sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, START)` para touch físico.

---

## 3. Estruturas e constantes

### 3.1 Touch do ZSystem

```c
typedef struct {
    int device;                 // +0
    int flags;                  // +4: 1=down, 4=move, 2=up
    unsigned short id;          // +8
    unsigned short pad;         // +10
    float unused_x;             // +12
    float unused_y;             // +16
    float pos_x;                // +20 (centered normalized)
    float pos_y;                // +24 (centered normalized)
} zp_touch_state_t;
```

Conversão do motor:

```
x_px = (pos_x + 0.5) * width
y_px = (pos_y + 0.5) * height
```

Ou seja, `pos_x/pos_y ∈ [-0.5, +0.5]`. Mandar pixel cru cai no canto errado.
Tipos: `down=7`, `move=6`, `up=8`; flags `1/4/2`.

### 3.2 Botões ZP

```
0=up, 1=down, 2=left, 3=right,
4=A (Cross), 5=B (Circle), 6=X (Square), 7=Y (Triangle),
8=start, 9=select, 10=L, 11=R
```

Mapeamento Vita comum:

| Vita | ZP |
|---|---|
| D-pad | 0..3 |
| Cross | 4 (A) |
| Circle | 5 (B) |
| Square | 6 (X) |
| Triangle | 7 (Y) |
| Start | 8 (ou B, para pause) |
| Select | 9 |
| L/R | 10/11 |

---

## 4. Menus vs luta

- **Menus**: use key events (D-pad + face buttons). Circle costuma ser
  “voltar”; Cross “confirmar”.
- **Luta**: se o HUD é touch-only, os botões de ataque precisam ser **toques
  sintéticos** nos retângulos do HUD; key events podem não fazer nada.
- **Pause**: mapeie Start para o botão de pause/back do motor. Se o usuário
  pedir “Start pausa, Circle não pausa na luta”, envie B apenas no menu e
  ignore Circle durante a luta.
- **Movimento**: analógico e/ou D-pad → arraste de toque no pad virtual.

Não envie key events de face buttons durante a luta se eles forem usados como
ataque por toque; pode gerar ação duplicada.

### 4.1 Cuidado: o menu de pause pode usar outra tecla de confirmação

Nem todo menu do motor usa a mesma tecla. Neste port:

- **Menu principal / menus normais**: Cross = **A** (4), Circle = B (5).
- **Pause (`gui/pause.ui`, estado `GamePause`)**: confirma com **Select** (9),
  não com A. Cross precisa ser traduzido para Select nesse estado.
- **Voltar no pause principal**: não há ação de “back” por tecla; Circle pode
  tocar o botão **RESUME** na tela (é um menu touch-friendly à direita).
- **Submenus do pause** (`ingamesettingsmain`): Circle = B funciona como
  voltar.
- **Ataques/movimento do HUD**: devem ser desligados enquanto pausado; toques
  sintéticos de luta podem clicar em botões do pause (foi o que acontecia:
  soco em (830,180) acionava RESUME; movimento à esquerda acionava SETTINGS).

Como detectar: o motor loga `[STATE] Entering state: GamePause`,
`ingamesettingsmain`, `confirmbacktomainmenu`. Mantenha um flag
`g_paused`/`g_pause_main` e mapeie as teclas por estado. Edge case: se o
jogador pausar segurando Cross, não dispare um Select imediato — inicialize o
estado do botão ao entrar no pause.

Lição genérica: **descubra a tecla de confirmação de cada UI nos logs/screenshot**;
não assuma que o pause usa o mesmo A do menu principal.

---

## 5. Descobrir o HUD

Hooks de diagnóstico:

- `CTouchScreen::AddButton(id, x, y, w, h, key)` — loga cada área e a tecla
  associada. É a forma mais direta de mapear o HUD.
- `MouseDown(x, y, id, ts)` — confirma as coordenadas recebidas.
- `CInputManager::LoadInputConfig` — mostra tabelas de teclado/analógico/touch.
- `VirtualKeyEvent` — mostra qual ação o motor gerou a partir do toque.

Exemplo de log:

```
[area] id=0 rect=720,0 240x272 key=...
[area] id=1 rect=720,272 240x272 key=...
[pad] fixed c=160,408 r=183
```

Com isso:

- **Soco** = centro do retângulo superior direito.
- **Chute** = centro do retângulo inferior direito.
- **Pad** = centro/raio do pad fixo esquerdo.
- **Pause** = área superior esquerda (se existir).

Envie `touch down` no centro, aguarde alguns frames, `touch up`. Para golpes
que precisam de hold, mantenha down até soltar o botão físico.

---

## 6. Movimento: analógico + D-pad

```c
float dx = ((float)pad.lx - 128.0f) / 127.0f;
float dy = ((float)pad.ly - 128.0f) / 127.0f;

/* Zere o drift quando o D-pad for usado. */
if (dx*dx + dy*dy < 0.35f*0.35f) { dx = 0; dy = 0; }

if (buttons & SCE_CTRL_LEFT)  dx -= 1;
if (buttons & SCE_CTRL_RIGHT) dx += 1;
if (buttons & SCE_CTRL_UP)    dy -= 1;
if (buttons & SCE_CTRL_DOWN)  dy += 1;

float mag2 = dx*dx + dy*dy;
if (mag2 > 1.0f) {
    float inv = 1.0f / sqrtf(mag2);
    dx *= inv; dy *= inv; mag2 = 1.0f;
}

if (in_fight && mag2 > 0.35f*0.35f) {
    // touch down (primeira vez) ou move a cada N frames
    // pos = centro_do_pad + (dx,dy) * raio_util
} else {
    // touch up se estava down
}
```

Detalhes:

- **Y do Vita cresce para baixo**: `UP` subtrai, `DOWN` soma.
- Normalize diagonais para não ficarem 1.41× mais rápidas.
- Use um raio útil menor que o raio do pad (ex.: 120 de 183) para não sair da
  área.
- Envie `move` a cada ~3 frames; não precisa a 60 Hz.
- Se o pad for fixo (não um stick flutuante), o down inicial e os moves usam
  o mesmo id de toque.

---

## 7. Auto-tap em telas de espera

Telas “toque para continuar” podem impedir o avanço. Detecte o estado (ex.:
`Entering state: gameloadtouchwait`) e envie um toque após N frames. Faça isso
**só** nesse estado; um toque automático em menus pode selecionar coisas sem
querer.

---

## 8. Automação de input no PC (MCP/companion)

Problemas reais:

- `SCE_CTRL_INTERCEPTED` (`0x00010000`) aparece quando o sistema intercepta o
  input (menu rápido, notificação, etc.). O jogo não recebe nada.
- Uma sequência longa em lote pode perder comandos; o estado do menu não
  avança.
- O log `[pad] buttons=0x...` é a fonte de verdade para saber se o botão
  chegou.

Boas práticas:

1. Envie **um** press/release por vez.
2. Verifique o log (`[pad] buttons=`, `menu-button`, mudança de estado).
3. Se não chegou, espere e reenvie; se `INTERCEPTED`, aguarde/feche o overlay.
4. Confirme cada transição de estado antes de mandar o próximo comando.
5. Para navegar menus, D-pad + Cross é mais confiável que sequências longas.

---

## 9. Mapeamento final deste port (exemplo)

- **Soco**: Square e Triangle → toque no botão superior direito.
- **Chute**: Cross e Circle → toque no botão inferior direito.
- **Movimento**: analógico esquerdo e D-pad → arraste no pad fixo.
- **Pause**: Start → B (abre o pause). No `GamePause`, Cross → Select
  (confirma) e Circle → toque em RESUME (fecha); nos submenus, Circle → B
  (volta). Os toques de luta ficam desligados enquanto pausado.
- **Menus**: Cross=A, Circle=B, Square=X, Triangle=Y; D-pad navega.
- **Auto-tap** em `gameloadtouchwait` para iniciar a luta.

Ajuste os retângulos/ids para cada jogo; não copie coordenadas.

---

## 10. Checklist de input

- [ ] D-pad, analógico, face buttons, Start/Select, L/R.
- [ ] Menus navegam e confirmam.
- [ ] Pause/resume com Start.
- [ ] Touch físico funciona (se o jogo usa).
- [ ] HUD mapeado (ataques, pad, pause).
- [ ] Ataques por toque funcionam com hold.
- [ ] Movimento analógico e D-pad; diagonais normalizadas; sem drift.
- [ ] Auto-tap só na tela de espera.
- [ ] Nenhum deadlock/thread de poll travada.
- [ ] Teste de 10+ minutos sem perder input.

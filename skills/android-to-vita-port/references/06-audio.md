# Áudio Android no PS Vita — AudioTrack JNI + sceAudioOut + FIR

> Guia genérico baseado no port de *Bruce Lee: Dragon Warrior*. O código de
> referência está em `source/java_shim.c` (bloco `AudioTrack`).

---

## 1. O problema

Jogos Android usam uma destas pilhas:

1. **OpenSL ES** (`libOpenSLES.so`, `slCreateEngine`, buffer queues).
2. **AudioTrack JNI** (`android/media/AudioTrack` + `write`).
3. **AAudio** (Android 8+, raramente em jogos antigos).
4. Engine própria que fala com um driver nativo.

O Vita não tem nenhuma delas. O objetivo é capturar o PCM que o jogo produz e
entregá-lo a `sceAudioOut`, respeitando taxa, canais, volume e tempo real.

---

## 2. Descobrir o caminho usado

Sinais no log:

```
[CAudioOutZdk] Open(freq: 44100, bps: 16, stereo: 1)
dlopen(libOpenSLES.so)
dlsym(opensles/crypto, slCreateEngine): stubbed
java_shim: registered 'android/media/AudioTrack/<init>' as id 0x1008
```

- Se `slCreateEngine` é stub e o jogo continua, ele tem fallback JNI.
- Se o jogo registra métodos `AudioTrack`, esse é o caminho.
- Se ele chama `zadr*`/`AAudio*`, intercepte a saída nativa.

Confirme no binário:

```bash
arm-vita-eabi-nm -D libdefault.so | grep -i -E 'audio|opensl|zadr|mixer'
arm-vita-eabi-objdump -d --start-address=... --stop-address=... libdefault.so
```

Neste port, o motor tentou OpenSL, falhou (stub) e caiu no driver
`_ZPAudioDriverOut_jni`, que usa `AudioTrack`.

---

## 3. FalsoJNI — o detalhe que quebra tudo

O FalsoJNI registra métodos dinamicamente. O nome completo
(`classe/metodo`) só é montado para **construtores**. Para métodos comuns, o
nome é **puro**:

```
java_shim: registered 'android/media/AudioTrack/<init>' as id 0x1008
java_shim: registered 'flush' as id 0x1009
java_shim: registered 'getMaxVolume' as id 0x100a
java_shim: registered 'getMinVolume' as id 0x100b
java_shim: registered 'getPlayState' as id 0x100c
java_shim: registered 'getState' as id 0x100d
java_shim: registered 'pause' as id 0x100e
java_shim: registered 'play' as id 0x100f
java_shim: registered 'release' as id 0x1010
java_shim: registered 'stop' as id 0x1011
java_shim: registered 'setStereoVolume' as id 0x1012
java_shim: registered 'write' as id 0x1013
java_shim: registered 'getMinBufferSize' as id 0x1014
```

Se o shim testa `strstr(full_name, "AudioTrack")` em `write`, **nunca casa**.
Foi a primeira causa do silêncio: `write` caía no stub e retornava 0.

Solução: identifique pelo nome puro (`write`, `play`, `getState`, ...) e trate
o construtor separadamente.

---

## 4. Assinaturas e semântica

| Método | Assinatura | Retorno correto |
|---|---|---|
| `getMinBufferSize` | `(III)I` | >= 4096 bytes |
| `<init>` | `(IIIIII)V` | objeto opaco |
| `getState` | `()I` | **1** (`STATE_INITIALIZED`) |
| `getPlayState` | `()I` | **3** (`PLAYSTATE_PLAYING`) |
| `play`/`pause`/`stop`/`release` | `()V` | no-op/estado |
| `setStereoVolume` | `(FF)I` | 0 |
| `write` | `([BII)I` | bytes escritos |
| `flush` | `()V` | reset |
| `getMaxVolume` | `()F` | 1.0 |
| `getMinVolume` | `()F` | 0.0 |

**`write` recebe `byte[]`**. O offset é em **bytes**, não em amostras:

```c
jbyteArray arr = va_arg(args, jbyteArray);
int offset = va_arg(args, int);
int size = va_arg(args, int);
JavaDynArray *jda = (JavaDynArray *)arr;
const uint8_t *bytes = (const uint8_t *)jda->array + offset;
int frames = size / (2 * channels);
```

`JavaDynArray` (FalsoJNI) tem `array` contíguo e `len` em elementos; para
`byte[]`, stride 1.

---

## 5. Fluxo do driver (por que a porta abria e ficava mudo)

O driver `_ZPAudioDriverOut_jni`:

1. `getMinBufferSize(44100, 12, 2)`.
2. `NewObject(AudioTrack, stream=3, rate=44100, channels=12, fmt=2, buf, mode=1)`.
   Nosso construtor abria `sceAudioOut` aqui.
3. `getState()`; se `!= 1`, chama `release`/descarta a track e retorna falha.
4. Cria a thread que chama o callback do motor e `write(byteArray, 0, bytes)`.
5. `getPlayState()`; se `!= 3` e ainda não started, `play`.

Com `getState` retornando 0, a porta era aberta e logo liberada. O log dizia
"porta aberta", mas nada era escrito. **Sempre valide o ciclo inteiro**, não
só a abertura.

---

## 6. `sceAudioOut` — restrições reais

```c
int port = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_MAIN,
                               512, 48000, SCE_AUDIO_OUT_MODE_STEREO);
int vol[2] = { SCE_AUDIO_OUT_MAX_VOL, SCE_AUDIO_OUT_MAX_VOL };
sceAudioOutSetVolume(port, SCE_AUDIO_VOLUME_FLAG_L_CH |
                            SCE_AUDIO_VOLUME_FLAG_R_CH, vol);
```

- **MAIN = 48000 Hz only.** `44100` retorna `0x80260008`
  (`SCE_AUDIO_OUT_ERROR_INVALID_SAMPLE_FREQ`), apesar do header listar 44100.
- `SCE_AUDIO_OUT_MAX_VOL` = 32768.
- Grain 512 frames = ~10,7 ms; múltiplo de 64.
- `sceAudioOutOutput` paceia a saída; monitore a taxa real.
- Se você abrir a porta em 44100 e falhar, faça fallback para 48000 + resample.

### Medindo a taxa

```c
SceUInt64 now = sceKernelGetProcessTimeWide();
double secs = (now - last_us) / 1e6;
double in_rate  = (frames_written - last_frames) / secs;
double out_rate = (grains - last_grains) * GRAIN / secs;
```

Esperado: `in_rate ≈ taxa do jogo` (ex.: 44100) e `out_rate ≈ 48000`.
Se `out_rate` for muito maior, há overrun; se muito menor, underrun/clicks.

---

## 7. Resampling 44.1 → 48 kHz

### 7.1 Por que linear/cúbico não basta

Interpoladores são filtros passa-baixa pobres. Medições com Catmull-Rom
(44.1→48, blocos de 64 amostras):

| Entrada | Espúrio | Nível |
|---:|---:|---:|
| 1 kHz | 8.6 kHz | -80.7 dB |
| 5 kHz | 8.9 kHz | -46.6 dB |
| 10 kHz | 13.9 kHz | -26.4 dB |
| 15 kHz | 18.9 kHz | -13.8 dB |
| 18 kHz | 21.9 kHz | -7.6 dB |

O espúrio de 15 kHz em -13.8 dB é claramente audível como chiado/estridência.
O problema piora com conteúdo alto (pratos, gritos, música comprimida).

### 7.2 FIR polifásico (fractional delay)

Projeto usado:

- `P = 1024` fases, `K = 24` taps de cada lado (`48` taps por saída).
- Corte `fc = 0.42` (≈18.5 kHz), janela Kaiser `beta = 10`.
- Normalização por fase para ganho DC = 1.
- Tabela `float coef[1024][48]` (~192 KB), construída uma vez no `at_open`.
- Ring buffer `8192` frames, pré-pad de `K` zeros.
- `step = 44100 / 48000 = 0.91875`.
- Processa enquanto `out_t + K < total_in` (precisa de K amostras futuras).
- Latência ≈ `K / 44100` ≈ 0.54 ms.

Resultado medido: espúrios ≤ **-67 dB** (limitados pela quantização de fase
com 1024 fases; 4096 fases baixaria para ~-79 dB, ao custo de 4× memória).

### 7.3 Fórmula

Para cada saída, seja `t` a posição fracionária em amostras de entrada,
`i = floor(t)`, `frac = t - i`, `p = floor(frac * P)`:

```
h[j] = sinc(2*fc*(j - frac)) * kaiser((j - frac)/K)   (j = -K+1..K)
y    = sum_j x[i+j] * h[j] / sum(h)
```

No código, `h` já está normalizado por fase na tabela.

### 7.4 Ring buffer e concorrência

- O driver pode escrever blocos de 64 frames; o FIR precisa de 24 amostras
  futuras. O ring absorve isso.
- `at_ring_put` copia o bloco (com wrap); `at_process_fir` consome enquanto
  houver dados + futuro.
- Se houver **duas threads** de áudio (duas tracks), o ring global é
  corrompido. Logue/controle a criação de tracks; idealmente uma track só.
- No `flush`, zere ring e `out_t` para não misturar áudio antigo.

### 7.5 Ganho e clipping

O mix deste jogo tinha pico ≈ -20 dBFS e RMS ≈ -37.7 dBFS. Com ganho 6×:

- pico ≈ -4.4 dBFS (≈ 19k de 32767);
- RMS ≈ -22 dBFS;
- sem clipping na maior parte; clamp protege picos.

Se o jogo já entrega pico perto de 0 dBFS, **não aplique ganho**. Meça antes.

---

## 8. Volume do jogo (config/save)

Neste port, `save/SaveGame.zpak` (ZIP) contém `settings.dat` com
`sound_volume` e `music_volume` em 0.0. O formato:

```
magic "1FDKENONCTX3N" + padding + u32 key_count
key table: u32 hash, u32 name_len (sem NUL), name + NUL
value area: record de 16 bytes:
  00 00 00 60 | key_index u32 LE | 00 00 00 20 | float LE
```

No caso, `key 65 = sound_volume`, `key 66 = music_volume`; alterar o float
para `1.0` (`00 00 80 3f`) e rezipar. Faça backup e edite com o jogo fechado.

**Atenção:** mesmo com volume 1.0, o mixer pode continuar baixo (o motor pode
não aplicar o config no boot). Meça o PCM; se continuar baixo, use ganho
digital no shim. Não mexa no volume do sistema sem autorização.

### 8.8 Código de referência (resumo do shim)

```c
/* --- AudioTrack JNI: dispatch por nome puro --- */
jint jni_shim_int_call(jmethodID id, va_list args) {
    const char *n = method_name_for_id(id); /* "write", "getState", ... */

    if (!strcmp(n, "getMinBufferSize")) {
        int rate = va_arg(args, int), ch = va_arg(args, int);
        int fmt = va_arg(args, int);
        return 4096;
    }
    if (!strcmp(n, "write")) {                  /* ([BII)I */
        jbyteArray arr = va_arg(args, jbyteArray);
        int offset = va_arg(args, int);          /* bytes */
        int size   = va_arg(args, int);
        JavaDynArray *jda = (JavaDynArray *)arr;
        const uint8_t *pcm = (const uint8_t *)jda->array + offset;
        at_write_pcm((const int16_t *)pcm, size / (2 * at_channels));
        return size;
    }
    if (!strcmp(n, "getState"))     return 1;   /* STATE_INITIALIZED */
    if (!strcmp(n, "getPlayState")) return 3;   /* PLAYSTATE_PLAYING */
    if (!strcmp(n, "setStereoVolume")) return 0;
    return 0;
}

/* --- FIR fracionário (Kaiser) --- */
#define AT_PHASES 1024
#define AT_K      24
#define AT_TAPS   (2 * AT_K)

static float at_coef[AT_PHASES][AT_TAPS];

static float at_i0(float x) {
    float sum = 1.0f, y = x * x * 0.25f, term = 1.0f;
    for (int k = 1; k < 32; k++) {
        term *= y / (float)(k * k);
        sum += term;
        if (term < sum * 1e-7f) break;
    }
    return sum;
}

static void at_build_coef(void) {
    const float fc = 0.42f, beta = 10.0f;
    float denom = at_i0(beta);
    for (int p = 0; p < AT_PHASES; p++) {
        float frac = (float)p / (float)AT_PHASES;
        float sum = 0.0f;
        for (int k = 0; k < AT_TAPS; k++) {
            float t = (float)(k - (AT_K - 1)) - frac;
            float h = (t > -1e-6f && t < 1e-6f)
                        ? 2.0f * fc
                        : sinf(6.28318530718f * fc * t) /
                          (3.14159265359f * t);
            if (t > -AT_K && t < AT_K) {
                float q = t / (float)AT_K;
                h *= at_i0(beta * sqrtf(1.0f - q * q)) / denom;
            }
            at_coef[p][k] = h;
            sum += h;
        }
        for (int k = 0; k < AT_TAPS; k++)
            at_coef[p][k] /= sum;              /* ganho DC = 1 */
    }
}

/* --- Ring buffer e processamento --- */
#define AT_RING 8192                     /* frames, potência de 2 */
static int16_t at_ring[AT_RING * 2];
static unsigned at_ring_w;
static long long at_total_in;
static double at_out_t, at_step;

static void at_process_fir(void) {
    const unsigned mask = AT_RING - 1;
    while (at_out_t + (double)AT_K < (double)at_total_in) {
        int i = (int)at_out_t;
        float frac = (float)(at_out_t - (double)i);
        int p = (int)(frac * (float)AT_PHASES);
        if (p >= AT_PHASES) p = AT_PHASES - 1;
        const float *c = at_coef[p];
        int base = i - (AT_K - 1);
        float l = 0.0f, r = 0.0f;
        for (int k = 0; k < AT_TAPS; k++) {
            unsigned idx = (unsigned)(base + k) & mask;
            l += (float)at_ring[idx * 2]     * c[k];
            r += (float)at_ring[idx * 2 + 1] * c[k];
        }
        at_grain[at_grain_frames * 2]     = at_gain_clip(l);
        at_grain[at_grain_frames * 2 + 1] = at_gain_clip(r);
        if (++at_grain_frames == AT_GRAIN) at_output_grain();
        at_out_t += at_step;                 /* 44100/48000 */
    }
}
```

- `at_ring_put` copia o bloco com wrap e incrementa `at_total_in`.
- `at_open` pré-pad a ring com `AT_K` zeros e seta `at_out_t = AT_K`.
- `at_output_grain` chama `sceAudioOutOutput(port, at_grain)` e zera o contador.
- O código completo está em `source/java_shim.c`.

---

## 9. Diagnóstico passo a passo

1. **Porta abriu?**
   Log `AudioTrack: sceAudioOut aberto (48000 Hz...)`; erro de
   `sceAudioOutOpenPort`.
2. **`write` está sendo chamado?**
   Log contador; se não, dispatch/JNI ou track descartada.
3. **PCM é não-zero?**
   Logue pico/amostras das primeiras chamadas. Zero = mixer mudo/callback.
4. **Taxa correta?**
   `in≈44100/s`, `out≈48000/s`.
5. **Volume?**
   Pico do PCM, ganho, clipping, config do jogo.
6. **Qualidade?**
   Teste tons (1k/5k/10k/15k) no simulador do resampler; procure espúrios.
7. **Estabilidade?**
   `sceAudioOutOutput` erros, underruns, stutter; remova logs/dumps pesados da
   thread de áudio.

### Erros comuns

| Sintoma | Causa provável | Correção |
|---|---|---|
| Porta abre, silêncio total | `write` não chamado / track descartada | dispatch por nome; `getState=1` |
| `write` chamado, pico 0 | mixer mudo/config/callback | volume do save; conferir callback; testar tom |
| Som existe, chiado/estridência | resampler ruim | FIR polifásico |
| Som baixo | mix baixo / ganho | medir pico; ganho com clamp |
| Som corta/estala | underrun, I/O na thread, grain errado | remover dump/log; conferir taxa |
| Som acelerado/grave | sample rate/canais errados | conferir `step`, `channels`, `write` bytes |
| Clipping | ganho alto | reduzir ganho/limiter |

---

## 10. Checklist de áudio

- [ ] Porta 48000, volume máximo.
- [ ] `getMinBufferSize`, `<init>`, `getState=1`, `getPlayState=3`.
- [ ] `write` com `byte[]`/offset em bytes.
- [ ] PCM não-zero; taxas em tempo real.
- [ ] FIR (não linear/cúbico) para 44.1→48.
- [ ] Ganho medido, sem clipping.
- [ ] Config/save de volume correto.
- [ ] Sem dump de PCM/log por amostra no release.
- [ ] Teste com música + SFX + pausa + troca de fase.

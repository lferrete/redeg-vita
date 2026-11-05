# 03 — JNI falso e APIs Android

> O jogo espera uma JVM. O FalsoJNI fornece `JNIEnv`/`JavaVM` sem Dalvik; seus
> shims dão semântica aos métodos que o jogo chama.

---

## 1. Arquitetura

O FalsoJNI implementa:

- `FindClass`/`GetObjectClass` → tokens de classe.
- `GetMethodID`/`GetStaticMethodID` → IDs (estáticos ou dinâmicos).
- `NewObject`/`Call*Method` → despacho por ID/tipo de retorno.
- Arrays (`JavaDynArray`), strings, `GetStringUTFChars`, etc.

Layout:

```c
typedef struct {
    uint32_t magic;
    jarray   array;   /* dados contíguos */
    jsize    len;
    FIELD_TYPE type;
} JavaDynArray;
```

`byte[]` = 1 byte/elemento; `short[]` = 2; `int[]` = 4. Não assuma stride 4.

## 2. O gotcha dos nomes

O FalsoJNI monta `classe/metodo` **só para `<init>`**. Métodos comuns são
registrados pelo nome **puro**:

```
registered 'android/media/AudioTrack/<init>' as id 0x1008
registered 'write' as id 0x1013
registered 'getState' as id 0x100d
```

Um shim que exige `strstr(full, "AudioTrack")` **não casa** com `write`.
Identifique pelo nome puro e trate o construtor separadamente.

## 3. Tipos de retorno

Derive do signature (`)V`, `)I`, `)F`, `)Z`, `)L...;`). Garanta handlers para
todos os tipos usados. Um `getState` que retorna 0 por padrão pode fazer o
driver descartar um objeto válido.

## 4. Strings, arquivos e sistema

Retornos comuns:

- `getSystemString(1)` → `"android"` (nome da plataforma no metadata).
- `getLanguage`/`getCountry` → `"en"`/`"US"` (ou o esperado).
- `getFilesDir`/`getExternalFilesDir`/`getCacheDir` → paths em `ux0:`.
- `getExternalStorageState` → `"mounted"`.
- `getPackageName` → package do APK.
- `getText`/`toString` → `""`; `list` → array vazio.
- `GetCACertsPEM` → `byte[0]`.

## 5. Objetos opacos

`ClassLoader`, `Activity`, `AssetManager`, `Surface`, `Context`: retorne token
não-nulo e estável. Se o jogo chamar um método nesse token, o FalsoJNI cai no
shim; devolva outro token/string conforme o tipo.

## 6. Assinaturas AudioTrack

```text
getMinBufferSize(III)I
<init>(IIIIII)V
getState()I          -> 1
getPlayState()I      -> 3
play()V / pause()V / stop()V / release()V
setStereoVolume(FF)I -> 0
write([BII)I         -> byte[]! offset em bytes
flush()V
getMaxVolume()F      -> 1.0
getMinVolume()F      -> 0.0
```

Implementação de `write`:

```c
if (!strcmp(n, "write")) {
    jbyteArray arr = va_arg(args, jbyteArray);
    int offset = va_arg(args, int);
    int size   = va_arg(args, int);
    JavaDynArray *jda = (JavaDynArray *)arr;
    const uint8_t *pcm = (const uint8_t *)jda->array + offset;
    int frames = size / (2 * channels);
    feed_audio((const int16_t *)pcm, frames);
    return size;
}
```

## 7. Debug

- Logue cada `GetMethodID`/registro (nome + signature + ID).
- Logue chamadas (com nível; evite spam).
- Se “método não existe”, veja o último `registered` e a assinatura.
- Confirme `Call*` vs `CallStatic*`; ambos precisam despachar.
- `NewObject` lê argumentos do `va_list`: ints, floats promovidos a double,
  objetos.

## 8. Falhas comuns

| Sintoma | Causa |
|---|---|
| método nunca chamado | nome com prefixo de classe |
| retorno 0 derruba objeto | `getState`/`getPlayState` errados |
| string/arquivo errado | retorno fixo errado (`getSystemString`) |
| crash em array | stride/tipo do `JavaDynArray` errado |
| `NewObject` corrompe args | ordem/tipo no `va_arg` |

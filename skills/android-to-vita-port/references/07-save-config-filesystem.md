# 07 — Save, config e filesystem

> Como redirecionar paths Android, entender pacotes/saves e editar config com
> segurança.

---

## 1. Paths virtuais

Intercepte `open`, `fopen`, `stat`, `opendir`, `mkdir`, `unlink`:

| Android | Vita |
|---|---|
| `/sdcard` | `ux0:data/<jogo>/sdcard` |
| `/data/data/<pkg>/files` | `ux0:data/<jogo>/files` |
| `/game` | `ux0:data/<jogo>/game` |
| `/save` | `ux0:data/<jogo>/save` |
| `/content0` | `ux0:data/<jogo>/content` |
| `/ext0` | `ux0:data/<jogo>/ext0` |

- `opendir` de raízes virtuais pode precisar cair em `content/`.
- `statfs` pode precisar de retorno plausível.
- O motor pode ter gerenciador ZIP/pacote próprio; não reimplemente se
  funciona, só direcione paths.

## 2. Pacotes

- `.pakh` (índice) + `.pakdNNN` (dados) — leia o header no binário.
- `.zpak` costuma ser ZIP (assets/save).
- `EnumeratePackages` vazio = path/opendir errado.

## 3. Reverter save/config

Metodologia:

1. Backup + copiar para PC.
2. `file`, `zipinfo`, `strings -el`, hexdump.
3. Procurar nomes de opções (`volume`, `language`, `difficulty`).
4. Identificar tabela de chaves e mapa chave→valor (índice/offset/tipo).
5. Identificar floats (`1.0 = 00 00 80 3f`, `0.25 = 00 00 80 3e`).
6. Editar com o jogo **fechado**, rezipar preservando compressão, enviar.
7. Relançar e confirmar no log que o jogo leu o valor.

Exemplo real (`settings.dat` dentro de `SaveGame.zpak`):

```
magic "1FDKENONCTX3N" + padding + u32 key_count
key table: u32 hash, u32 name_len (sem NUL), name + NUL
value records (16 bytes):
  00 00 00 60 | key_index u32 LE | 00 00 00 20 | float LE
```

`key 65 = sound_volume`, `key 66 = music_volume`; float `1.0`.

## 4. Editar ZIP

```python
import zipfile
zin = zipfile.ZipFile("SaveGame.zpak")
zout = zipfile.ZipFile("SaveGame_patched.zpak", "w", zipfile.ZIP_DEFLATED)
for info in zin.infolist():
    data = open("extract/" + info.filename, "rb").read()
    zout.writestr(info, data, compress_type=info.compress_type)
zout.close()
```

- Preserve a ordem/compressão dos entries.
- Envie com o app fechado; se ele reescreve no exit, o patch se perde.
- Sempre mantenha o original.

## 5. Falhas comuns

| Sintoma | Causa |
|---|---|
| save não aparece | path errado/opendir |
| save corrompe | formato/zip errado |
| config ignorado | editado com app aberto |
| jogo recria save | versão/metadata mismatch |
| “sem espaço” | `statfs` stub |

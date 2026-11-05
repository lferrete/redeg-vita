# Investigação dos artefatos gráficos — 2026-11-05

Correção implementada e instalada na versão 1.0.1; reteste final concluído.
A declaração anterior de que todos os artefatos 3D
haviam sido eliminados não foi confirmada: os triângulos esticados foram
reproduzidos na abertura pelo stream do Vita MCP.

## Ambiente e evidências

- Projeto iOS Mach-O ARMv6, não Android/so_util. Jogo 1.03.00.
- SHA-256 do Mach-O original:
  `9e451d0bbdf4aff2e5ad40eabb577e474464ca855dc41375845e4a54f73a5267`.
- Vita Companion 1.08; `REDEGEN01`; IP descoberto por `vita_status`.
- VitaSDK `arm-vita-eabi-gcc` 10.3.0, MSYS2, CMake Debug (inclui `-O0`).
- Árvore Git limpa no início, HEAD `74d8570`.
- Evidências privadas em `logs/2026-11-05/` (ignoradas pelo Git).
- Backup do save original: `backup/save` (20.000 bytes), antes de aceitar
  New Game. Backup do executável instalado: `backup/installed`;
  SHA-256 `e2e133c446d34387de395fb3a67e7d743e76d3c2172bee125264ee3c73ac35eb`.
- Backup do ELF local: `backup/local_redeg.elf`. O ELF local inicial era
  mais recente que o SELF inicial; não presumir correspondência entre eles.

## Ciclo de compilação e teste

No PowerShell, a partir da raiz do projeto:

```powershell
& C:/msys64/usr/bin/bash.exe -lc 'export VITASDK=/c/vitasdk; export PATH="$VITASDK/bin:/usr/bin:$PATH"; cd "/c/Users/leand/Downloads/PSVITA/Resident Evil Degeneration IOS/redeg-vita/build"; make -j8'
```

No Vita MCP: keep-awake; fechar somente `REDEGEN01`; instalar o VPK;
conferir resultado; no fallback enviar `build/eboot.bin` para
`ux0:/app/REDEGEN01/eboot.bin` com `dangerous:true`; abrir; listar logs;
baixar o novo log e conferir a data de build. `vita_close_app` retorna antes
de necessariamente liberar o executável: um STOR imediato falhou com 550.
Conferir FTP/app e tentar novamente após a condição mudar resolveu.

O `promote` desta sessão falhou com `0x80870005`; upload bem-sucedido do VPK
não prova atualização do executável instalado. Nenhum plugin de boot ou
firmware foi modificado. O stream já existente é carregado pelo próprio app.

`vita_screenshot` forneceu LiveArea válida, mas um PNG preto no jogo. Foi
usado `vita_stream_start` seguido de `vita_stream_frame` e inspeção do JPEG
recente. Screenshots temporários foram apagados via `vita_purge_screenshots`.
Não há autorização para guardar capturas permanentemente nesta sessão.

O `vita_pull` por vezes retornou EEXIST depois de já criar o arquivo local.
Conferir existência, tamanho e conteúdo antes de repetir; usar novos nomes.

## Experimentos

| Rodada | Alteração | Evidência privada | Resultado |
|---|---|---|---|
| Baseline | Executável instalado original | `baseline_previous.log`, `baseline_005.log`, `baseline_full`, `baseline_end`; logs Vita 004/005 | Artefatos reproduzidos na abertura; o shim skin32 reportava muitos overflows, o que não demonstra a causa. |
| A | Desativar a substituição de `IwAnimSkin_Offset`; manter o caminho original | `A_native_skin`, `A_native_full`; log 006; VPK `8af0e83f747f034e231262f4d0e44090316c8717a832f4cc8020ec027e5af4ff` | Artefatos e `[3D-ANOM]` persistem. Remover skin32 sozinho é insuficiente. |
| Auditoria | Flag `ux0:/data/redeg/graphics_probe` habilita dump antes de entrar no guest | `image-audit` (7.589.888 bytes), logs 007/008; VPK `eac5d8461561e746eedfe02ed72be81465a9564812da97c263e38cd9ba6d2c57` | Rotinas de skinning e transformação comparadas; máscaras numéricas indevidamente rebaseadas encontradas. |
| B | Preservar sete constantes numéricas verificadas nos pools literais | `B_numeric_literals`, `B_numeric_full`; log 009; VPK `a52d241f034a1e2cf2c8f3c5723414c11b5eeeb24f84f4de88ba0f4c7044896b` | Corrige um bug real do loader; artefatos ainda aparecem. |
| C | Remover também o branch que forçava sempre o caminho completo da matriz | `C_original_matrices`; log 010; VPK `3e04ad1c636a8ab3f07f98941a93d4a1544f84853ea0bf052845c095635d1cd6` | `[3D-ANOM]` persiste; alteração sozinha não resolve. |
| D | Copiar entradas/saídas no instante do transform, comparar no draw | `D_vertex_snapshots`, `D_vertex_full`; log 011; VPK `7ebe4db5dc47cecc040eef828527a47e9bdaf6d2c06cc94ce6e7ac3e12d3bc65` | `changed_since_transform=0` nos casos anômalos: posições já deformadas na CPU. |
| E | Executar skinning nativo com trampoline e comparar um osso com a transformação C | `E_skin_reference`; log 012; VPK `b03c64b4155c40d407c358b4c311661783197d7999702f2da86719127b4b15ec` | O resultado nativo corresponde ao cálculo C com truncamento de 16 bits. A matriz de entrada já está errada. |
| F | Registrar quaternions locais e cadeia de pais dos ossos afetados | `F_bone_quaternions`, `F_bone_full`; log 013; VPK `0dd936a6cf026cbbd217f4d8533cd205f6c1f1c922bfa3bffe8aac9a62a3e862` | Quaternions de alguns ancestrais têm componentes próximos de 30.000, quando a norma esperada em Q12 é aproximadamente 4.096. |
| G | Proteger seno/atan contra rebase de `__data`; manter os probes para comparação | `G_fixed_image`, `G_fixed_full`, `G_gameplay_status`, `G_communications`, `G_moving_gameplay`; log 014; VPK `a631e4179e5439f670ea0b519bd7fdf0d75955e7fd838874ff31231b37e3d879` | 8.192 casos de seno/cosseno corretos; zero `[3D-ANOM]` e zero `[skin-probe]` no log coletado até a primeira fase. Leon e os zumbis aparecem íntegros. |
| H | Remover os shims/probes pesados, restaurar rotinas nativas e empacotar 1.0.1 | `final_image`, `final_boot`, `final_installed_eboot`, `iwmath-final.json`, `final_stability_complete`; logs 015/016; VPK `ca3c42b73e14437746469d1a3c20bdf621b6a2917e85a3876bd57881151c777a` | Instalação/promote confirmado, SELF instalado idêntico ao compilado; 8.192 casos corretos. Execução contínua de 30 min 51 s, 58.440 frames e nenhum novo crash dump. |

### Correção principal: seno numérico corrompido

O quarto de onda de seno do IwMath é um array de 1.025 `uint16_t` em
VM `0x235e9c`, seguido de alinhamento até `0x2366a0`. A tabela de atan,
de 513 `int32_t`, ocupa `0x2366a0..0x236ea4`.

O loader interpretava a palavra em `0x235ea0`, `0x0012000c`, como ponteiro
para `.rodata`. Somava o delta `0x97fff000` e gravava `0x9811f00c`.
Isso transformava as amostras de seno **12 e 18** em **61.452 e 38.929**.
O número `0x12000c` também é um endereço legítimo dentro das vtables do
executável. Excluir esse valor de todos os ponteiros seria incorreto: a
proteção precisa identificar o intervalo numérico de origem.

Cadeia reconstruída no executável original:

1. `CIwAnimPlayer` atualiza a pose em `0x90a84`; o helper `0x896f4`
   interpola os registros de 14 bytes (posição + quaternion).
2. Slerp Q12 em `0xec29e` chama seno `0x733a0` e cosseno `0x73340`.
3. Esses helpers leem o quarto de onda em `0x235e9c`.
4. Com a tabela alterada, pequenas rotações geram pesos enormes na slerp;
   componentes de quaternions excedem a faixa normal.
5. `CIwAnimBone` em `0x806e0` converte os quaternions por `0xfe942`,
   compõe as matrizes dos pais e propaga o erro aos filhos.
6. `IwAnimSkin_Offset` em `0x6c43c` transforma posições com essas matrizes;
   o truncamento nativo de 16 bits torna os triângulos desconexos.

O snapshot D observou `changed_since_transform=0` nos draws anômalos.
Portanto os vértices já eram inválidos ao sair da CPU; a reutilização posterior
do buffer e o GPU não explicam esses casos medidos. A rodada E confirmou que
o cálculo nativo não era o defeito primário. Ampliar/clamp de coordenadas
apenas tratava uma consequência e não recuperava a animação.

A correção protege as duas tabelas numéricas no passe de rebase de `__data`.
O skinning e o caminho rápido de matrizes originais foram restaurados. A build
final remove 649 linhas de tentativas de skinning/diagnóstico de `gl_syms.c`.

### Regressão com as instruções ARM originais

`tools/verify_iwmath.py` usa Unicorn para executar seno e cosseno **do jogo**
com todos os 4.096 ângulos de uma volta, tanto na imagem original quanto na
imagem carregada pelo Vita. O teste exige o SHA-256 exato da IPA analisada.

```powershell
py -3 tools/verify_iwmath.py ../_ipa_extract/Payload/residentevil.app/residentevil logs/2026-11-05/image-audit --output logs/2026-11-05/iwmath-baseline.json
py -3 tools/verify_iwmath.py ../_ipa_extract/Payload/residentevil.app/residentevil logs/2026-11-05/final_image --output logs/2026-11-05/iwmath-final.json
```

O primeiro comando deve falhar: 16 divergências / 8.192 casos. O segundo
passa com zero divergências. Isso é replay focado de CPU, não emulação do
jogo completo. A comparação final também confirma que as tabelas permanecem
byte a byte iguais às originais e que os prólogos do skinning e do updater
de matrizes não estão mais patcheados.

Para obter um novo dump, criar o arquivo vazio
`ux0:/data/redeg/graphics_probe`, iniciar a build de desenvolvimento, baixar
`ux0:/data/redeg/logs/loaded-image.bin` e remover a flag. A ausência da flag
mantém o dump desabilitado. O arquivo contém código do jogo e permanece
privado. Nenhum arquivo de dados original do jogo precisa ser corrigido.

### Artefatos da build final

- VPK: `build/redeg.vpk`, versão 1.0.1, SFO APP_VER `01.01`.
- VPK SHA-256:
  `ca3c42b73e14437746469d1a3c20bdf621b6a2917e85a3876bd57881151c777a`.
- SELF SHA-256, conferido por download do executável instalado:
  `b9810a710062a37fe5bac448687b8ecdfbb69dc858cb7e931d086aac94bf594d`.
- O pacote contém exatamente oito arquivos: SELF, SFO, quatro PNGs,
  template LiveArea e provenance. Não contém jogo, dump nem save.
- O `promote` funcionou na instalação final. Foram conferidos também SFO
  e provenance, além do banner v1.0.1 no log.
- Save original restaurado e conferido por download antes do reteste:
  `d935daa70e5499a7bbc73a4011e8acbd296bb37a62cfb7772e19b4a91e6bad9b`.
- Início do reteste contínuo da build final: **06:22:11 UTC**, log 016.
- Aos 20 minutos, `final_stability_20m` já registra 40.380 frames na mesma
  execução. A contagem não é benchmark de FPS. Houve inspeção de stream
  recente durante caminhada/giro/mira/disparo, comunicações, mapa, inventário
  via toque e visualização 3D da munição; os objetos apareceram íntegros.
- A navegação por R nas abas do PDA não foi confirmada; a troca por toque
  foi observada. Não ampliar esta correção gráfica para alegar validação
  completa de controles físicos, QTEs ou todas as fases.

## Bug confirmado: constantes tratadas como ponteiros

`looks_like_function` aceita qualquer número ímpar dentro da faixa do código
Thumb como ponteiro. O rebase passou de `0x0003ffff` para `0x9803efff` em
seis pools e de `0x00002000` para `0x98001000` em outro.

- VM `03c274`, `03cb80`, `098ca8`, `0c47c8`, `0ed9ec`, `100868`: `3ffff`.
- VM `0938e4`: `2000`, carregado em `093874` e usado por `orrs`/`bics` em
  flags; não é endereço.
- A exceção verifica endereço **e valor original**. Ela é específica deste
  executável; não se deve excluir todos os valores semelhantes em outros ports.
- `numeric-corruption.json` registra o antes/depois medido no dump.
- `tools/audit_graphics_image.py` gera uma comparação reproduzível. Os arquivos
  de imagem contêm código original e devem permanecer privados.

## Limitações e correções de interpretação

- `[gl] N frames` é contagem, não medição de FPS; não comprova 60 FPS.
- `[3D-ANOM]` é heurística de comprimentos de arestas, não prova isolada de
  corrupção: perspectiva/near clipping e buffers antigos podem confundir.
- O antigo trace guardava ponteiros para entradas, e só as lia no draw.
  Sem snapshot, valores estranhos podem vir da reutilização legítima do scratch.
- A documentação antiga cita `037064`/`0372b0`/`037310` como pipeline gráfico.
  Desmontagem desta IPA mostra outras funções nessa faixa, incluindo parser
  numérico; não reutilizar esses offsets como prova de bug de matrix mode.
- REA 6.8.0 abriu e identificou o Mach-O, mas o provider IDA configurado
  respondeu `fetch failed`. O provider Ghidra desta integração no Windows
  recusa Mach-O (aceita PE/raw 6502). Foi usado Capstone local e comparação
  com dump real, sem alegar decompilação pelo REA. Como fallback posterior,
  Ghidra 12.1.4 foi executado diretamente, usando a instalação já existente.
  A análise global atingiu o timeout de 100 s; o projeto parcial foi salvo e
  funções específicas foram decompiladas com `-process residentevil
  -noanalysis -postScript DumpGraphics.java ...`. Outputs com erro de TMode
  ou endereço incorreto foram descartados e confrontados com Capstone.
  O projeto/Java/outputs ficam em `logs/2026-11-05/`.
- Não chamar a investigação concluída enquanto restarem artefatos visuais.

## Reteste final e encerramento

- Execução 016 da build final: **06:22:11 a 06:53:02 UTC** — **1.851 s
  (30 min 51 s)**, sem reiniciar o app durante essa janela.
- `final_stability_complete` registra **58.440 frames** apresentados. Essa
  contagem inclui períodos de carregamento e menus e não é medição de FPS.
- O stream foi inspecionado com frames recentes em abertura, primeira fase,
  caminhada, giro, mira/disparo, comunicações, mapa, inventário por toque e
  objeto 3D de munição. Não se observaram os estouros de malha nesses casos.
- `vita_wait(for:crash)` foi usado em várias janelas de 25–50 s. A listagem
  final dos três dumps mais recentes permaneceu idêntica à inicial; todos
  anteriores à investigação. Nenhum novo dump foi produzido nos retestes.
- Depois de fechar o app, o save original foi restaurado novamente e baixado
  para `restored_save_final`: **20.000 bytes**, SHA-256
  `d935daa70e5499a7bbc73a4011e8acbd296bb37a62cfb7772e19b4a91e6bad9b`.
- O jogo foi reaberto e deixado no menu principal (execução 017). O save
  baixado depois de abrir o menu continuou idêntico ao original, em
  `restored_save_after_menu`. O stream foi encerrado e as capturas temporárias
  foram removidas com `vita_purge_screenshots`.
  O purge não cobre `screenshots/stream/latest.jpg` (cache JPEG de 9.250 bytes).
  A tentativa de excluí-lo com `Remove-Item -LiteralPath` após parar o stream
  foi recusada pela revisão automática do cliente (`blocked by policy`). Esse
  único cache permaneceu no PC; nenhum PNG da sessão permaneceu.
- O teste não comprova todas as fases, QTEs ou controles físicos manuais.
  As entradas foram injetadas via Vita MCP e seus efeitos foram inspecionados.
- O flag de dump e a imagem temporária no Vita foram removidos. As imagens
  de código e logs de evidência permanecem somente nos diretórios locais
  privados; o pacote final contém apenas o loader e a apresentação do app.
- `evidence-manifest.json` contém fingerprints de evidências privadas;
  `stability-observations.json` contém as observações de estabilidade.

## Notas operacionais adicionais

- Um trampoline de diagnóstico com `ldr pc, =0x9806b444` falhou na montagem
  por literal pool fora do alcance, pois `gl_syms.c` era grande. A versão
  temporária usou `movw ip` + `movt ip` + `bx ip`. Foi removida da build final.
  A primeira compilação também avisou sobre `abs` sem `<stdlib.h>`; corrigido.
- O helper de seno/cosseno não usa stack e foi reproduzido diretamente em
  Unicorn em modo ARM. Misturar modo Thumb/ARM em endereços inferidos gera
  pseudocódigo inválido; uma saída de decompilador com erro não é evidência.
- `tools/sync_port_skill.ps1` apontava para uma skill inexistente herdada de
  outro port e apagava destinos inteiros. Agora usa a skill ativa em
  `~/.agents/skills/android-to-vita-port`, atualiza este case study e faz
  cópia por merge, preservando outras referências. A skill ativa e as cópias
  OpenCode foram sincronizadas; `quick_validate.py` passou.
- O save original foi restaurado com o app fechado e validado byte a byte.
  O primeiro `Continue` da build final carregou a primeira fase e as
  comunicações com Hunnigan. As ações de teste usaram uma cópia restaurável;
  a preservação final foi conferida também depois de reabrir o menu.

## Versionamento da documentação e skill

Na preparação do commit, a skill global foi exportada para
`skills/android-to-vita-port/` com seus scripts, referências e fontes de
diagnóstico. Não contém executáveis, dados de jogos, saves, logs brutos ou
tokens. Endereços privados dos dispositivos nos exemplos do snapshot foram
substituídos por `<VITA_IP>`; usar descoberta do Vita MCP em cada sessão.

`PORT-LOG.md`, `PORT-STATUS.md` e `tools/sync_port_skill.ps1` agora são
versionados. O sincronizador usa a cópia do repositório por padrão, atualiza
a referência 17 a partir deste relatório e faz merge nos diretórios de skills,
sem apagar arquivos de destino. Inputs do jogo e evidências privadas continuam
ignorados pelo Git; os hashes e resultados ficam registrados aqui.

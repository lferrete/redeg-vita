# TASM complete recoverable investigation record

Generated from the maintained repository documents. Planned steps are not completed work; inherited notes live in reference 16.


---

## Source: docs/CURRENT_STATE.md

# Estado para retomar a investigação

Atualizado em 2026-11-05, experimento 193. Qualidade, carregamento, performance
e estabilidade têm pendências. Este registro não aprova uma release.

## Estado instalado

- Vita Companion<VITA_IP>, TASMVITA1, CPU444MHz, HUD no boot193.
- Binário192 diagnóstico, VPK SHA-256
  `E4B11C052A2E007E715672C6A39BCA9C8480183B9134B386018649437B296C1D`.
  ELF/VPK/fontes/cache/flags em `../_logs/build-192-shader-lifecycle-quiet/`.
  Mesmo binário nos boots192 e193. **Não considerar quiet191/192 release validada.**
- Cache: DEBUG_SOLOADER=OFF, TASM_NATIVE_PROFILE=OFF,
  TASM_NATIVE_CITY_STREAMING=ON, TASM_DEV_HARNESS=ON. **Defaults frescos191:
  DEBUG/harness/nativeprofile OFF**, city/compaction/tag ON, softwareETC1 OFF.
  Harness deste teste explicitamente ON, poll1s/state5s. Há logsperf/watchdog/
  heartbeat mesmo com DEBUG OFF. telemetry_fast restaura250ms/1s, telemetry_slow
  retorna1s/5s; comandos e mudanças de menu forçam estado. Campo dev_io_us não
  inclui o ciclo de I/O ainda em andamento ao montar o JSON.
- Sem override XML ativo. Backup remoto
  `ux0:/data/tasm/vita_config_override.profile177.xml`. Fallback187 reproduz as
  61 propriedades177. XML gerado2844B SHA
  `5B3CB9B93C2B3853443959F89694C5B5E61CA844D4B61FDF8E6A480349A81337`,
  conferido em187/189; Lv2/fachada detalhada persistem.
- CityVBO/sync são uma política pareada defaultON188, independente deDEBUG.
  Enable markers removidos; hardware188 validou sem eles. Os três markers
  `native_city.disable`, `native_city_vbo.disable`, `native_city_sync.disable`
  têm prioridade no próximo boot. Controle190 com native_city.disable validou
  CPU/compare indisponível, fachada preservada e36,77FPS; marker removido e
  política GPU restaurada/confirmada em191. Não houve troca de binário em190.
- Probes adicionais ausentes: actor_dispatch, texture_policy, brightness_ray,
  resource_watch, sync_probe, mesh/batchVBO, scheduler/material. Fixconddeadline
  nunca ativado; cache iluminação OFF. Não reinstalar plugins/clock500.
- Saves fechados em `../_logs/save-189-before-cpu-control`. Sem edição manual de
  posição/HP/progresso. Rename157–159 mantém Lv2 em várias aberturas.
- As duas capturas191 foram inspecionadas e apagadas PC/Vita; só métricas retidas.

## Evidência atual e limites

- Preservar offsets vitaGL135 e citysync143.188 city_shadow0/6400 páginas.
 189 request1 durante streaming deu1/divergência; request2 após estabilizar deu0.
  Escritas CPU podem ser concorrentes e pendentes; não omitir nenhum resultado.
  Harness publica city_check_request/result: ACK sozinho não prova sucesso.
-191 publica também requested/active/pending, different/marked/uncovered pages
  do último check e total de erros de sync. Request1:10 divergentes/12 marcadas,
  uncovered0/pending1. Request2 estabilizado:0/6400, pending0/errors0. Após primeiro
  percurso:200 divergentes/218 marcadas, uncovered0/pending1; repetição16 zerou.
  Após segundo percurso, request24 zerou. Checks/capturas fora das medições.
- XML189 e fachada esquerda conferidos: hash de região
  `41156e461ef98b354f989fb100bd83850a78926d6cdb8639256fbdcf3c355aba`, igual177.
- ABBA mesma pose189, telemetria ON/OFF/OFF/ON,12–13s sem capturas:
  37,65 /38,22 /38,20 /36,27FPS. Há deriva temporal; sugere custo de arquivos,
  não ganho exato no percurso. `../_logs/quiet189-telemetry-abba.json`.
  Polling cmd.json4Hz persiste com telemetry_pause; escrita estado1Hz.
  Comparação/captura também são PERF_DEVFILES; não atribuir todo custo à escrita.
- ABBA191 no MESMO binário,fast/slow/slow/fast:36,915/38,072/38,087/36,630FPS,
  I/O26,153/6,912/7,011/26,297ms/s (~73% menor nesta pose). Slow mantido.
  `../_logs/slow191-telemetry-abba.json`. Isso reduz custo de diagnóstico,
  **não** resolve o gargalo principal: percursos19120,94 e20,91FPS, poses distintas.
  Fachada roof191 conserva hash41156e...355aba; segunda captura chegou a fachada
  de vidro junto da parede, personagem visível. Clipping não foi certificado.
- Release sem harness191 compilou exit0, sem novos warnings, SHA
  `04C35645F7C87DBF0AC4FA9C5C45B0BC6A70FDB38AF636AB5457262338F96901`,
  `../_logs/build-191-release-no-harness/`, **não instalado**. Root mantém VPK9BB.
- Distâncias184far120000 e186mid20000 rejeitadas: ganho visual incerto;
  mid caiu para~35FPS parado e aumentou draws. Restituídos far600000/mid10000.
  Near real éfloat10, não inteiro. Percursos acabaram em poses diferentes.
- Travessia177~19,76FPS, áreas pesadas16–23FPS. Pop-in/stutters permanecem.
  Raio iluminação181~1,6ms numa pose; cache100ms ganho pequeno/risco de atraso,
  OFF. Timedwait182 teve zero chamadas e não explica o carregamento observado.
- Crash COLLADA144/163 aberto; batch169 é distinto e experimentoOFF.
- Câmera133 H0,65/V0,75, calibração/histerese/curva. Neutro189 sem drag/delta.
  Clipping junto de paredes e feel angular ainda exigem validar.
  Em191 não há camera_sensitivity.cfg na listagem completa126linhas. Teste20
  válido: raw235/141, right_active/down1, dx6/dy0; release21:108/141/down0/delta0.
  Primeiro teste17 estava interceptado e não vale como validação.

## Próximas ações planejadas

1. REA dos consumidores streaming/LOD e novas medidas I/O;191 reduziu overhead,
   mas gameplay denso continua~21FPS. Processosgrid306b90/LOD305120 já extraídos
   por REA em _logs, sem novos hooks ou mudanças da política nativa.
2. Investigar links6/7/11 sem fragment shader em189–191.188DEBUG passou esses
   relinks;192quiet com trace passou79links, todos na thread40010003;193 mesmo
   binário semmarker/zeroeventos também abriuHUD sem avisos no log coletado.
   Logo não há fix causal certificado nem prova de dependência sódeDEBUG.
   Trace usa ring4096eventos/128KiB estáticos, wrappers create/attach/delete/link,
   dump explícito shader_lifecycle emCSV e resultadoJSON. Marker removido193.
   Nenhum shaderdeleted nativo observado no trace;13programs deletados e recriados.
   Program6(V8/F10),7(V8/F11),11(V8/F16) reconstruídos e ligados corretamente.
   Próxima hipótese: isolar wrappers de criação/anexação, preservando o resto
   do binário/traceOFF. Não chamar alteração de layout/diagnóstico de correção.
3. Não aumentar buffer25MiB
   sem adaptar sync/dirtybitmap. Nenhum resultado futuro está certificado.
4. Mais combate/missões/saves e30min de gameplay ATIVO. Pause/parado não contam.

## Operação e registros

- Diário: [DEBUG_SESSION_2026-11-04.md](DEBUG_SESSION_2026-11-04.md).
  Índice: [PORTING_KNOWLEDGE.md](PORTING_KNOWLEDGE.md).
- `py tools/update_port_knowledge.py` sincroniza refs13–16/helper/heap nas skills
  locais/globais. Referência15 começa por este estado e preserva todo histórico.
- Um cmd.json por vez, request_id único, ACK e resultado explícito.15s se háI/O.
  Timeout interrompe entradas dependentes. Estado antigo não prova boot novo.
- SCE_CTRL_INTERCEPTED65536 impede inputs. Em186 launch_app(close_current:false,
  force:true) retomou o MESMO processo e liberou inputs; não foi novo boot.
- Captura própria frame.bmp no diretório vita-mcp/TASMVITA1: HUD→ACK→Pause→pull→
  inspecionar→apagar PC/Vita. Excluir janela e adjacente das medidas de FPS.
- Stick: `press right-stick 255 128; wait 700ms; release right-stick`.
  Injeção soma deslocamentos ao controle físico, não são posições absolutas.
- Build `& C:/msys64/usr/bin/bash.exe ./make.sh`, esperar exit0. Configure bash-c
  com VITASDK/PATH explícitos. Não misturar outputs/ELF entre diretórios.
- REA CLI existente com REA_OBJDUMP_COMMAND para VitaSDKobjdump; rg para grandes
  disassemblies. Listar paths antes de presumir nomes de arquivos/referências.


---

## Source: docs/PORTING_KNOWLEDGE.md

# Registro de engenharia e conhecimento reutilizável

Este é o ponto de entrada para todo o histórico recuperável do port. O trabalho
continua em andamento. Resultados de uma cena não certificam o jogo completo.
Para retomar, ler primeiro [o estado instalado e o próximo teste](CURRENT_STATE.md).
Atualização191: LEVEL2 continua carregando após a correção de rename157.
BuildingMipmapSkip0 melhorou a definição das fachadas observadas, sem perda de
FPS demonstrada na pose controlada (~37FPS), mas exige mais memória. A travessia
medida177 ficou em19,76FPS; áreas pesadas anteriores ficaram em16–20FPS.
As promoções adicionais de VBO para COLLADA e batch foram rejeitadas/desligadas;
a segunda causou uma falha distinta ao acessar dados CPU ausentes. CityVBO/sync143
continua ativo. Câmera133 mantém calibração e sensibilidade H0,65/V0,75.
Perfis178–181 mediram custo parcial de UpdateEffect/raio de iluminação, cujo
cache continuaOFF. Distâncias184/186 rejeitadas. Perfil177 virou fallback187 e
defaults vitaGL foram alinhados. CityVBO/sync é uma política defaultON188,
validada sem enable markers.189 roda core semDEBUG com harness independente,
XML/fachada conferidos e checkcity divergente no streaming/igual após estabilizar.
191 reduziu I/O periódico do diagnóstico em~73% na comparação local; defaults
frescos desligam DEBUG/harness/perfil, mantendo a política gráfica aceita.
ControleCPU190 validado e removido; GPU191 sem erros/checagens estabilizadas iguais.
Travessias191 continuam~21FPS. Carregamento tardio, crash COLLADA e testes
prolongados seguem abertos; ganho de diagnóstico não resolve o gargalo principal.
As informações anteriores à investigação atual foram recuperadas dos documentos
locais e do código; não são apresentadas como testes refeitos nesta sessão.

## Onde está cada parte do histórico

| Registro | Conteúdo e autoridade |
|---|---|
| [Diário de hardware](DEBUG_SESSION_2026-11-04.md) | Sequência de experimentos 066 em diante, hashes, resultados, falhas, hipóteses descartadas e pendências. Fonte atual das conclusões. |
| [Índice local de evidências](PORTING_EVIDENCE.json) | Arquivos disponíveis, tamanho e SHA-256; logs anteriores preservados mesmo quando falta a descrição original do experimento. |
| `PORTING_PLAN.md`, `PORT_GUIDE.md`, `docs/CASE_STUDY_TASM.md` | Notas herdadas da implementação inicial. Possuem divergências corrigidas pelo diário. Cópia integral preservada na referência 16 da skill. |
| `CHANGELOG.md`, commits `954ee78` e `e82ff95` | Histórico publicado, com alegações antigas qualificadas pela auditoria atual. |
| `.opencode/skills/android-to-vita-port/references/13-case-study-gameloft-tasm.md` | JNI, Bionic, perfil de dispositivo, controle Xperia e auditoria inicial. |
| Referência 14 da mesma skill | Lições verificadas de gráficos, memória, inputs, medição e cubemaps. |
| Referência 15 da mesma skill | Cópia completa deste registro e do diário atual, gerada pelo script abaixo. |
| Referência 16 da mesma skill | Notas históricas integrais, qualificadas como herdadas; não usar offsets/recomendações sem conferir o diário atual. |

Não existe um transcript integral de todas as chamadas antigas neste workspace.
Quando o registro não permite reconstruir uma ação ou seu resultado, mantê-la
como lacuna, sem inventar datas, testes ou causalidade. Cada nova ação material
deve entrar no diário antes de encerrar a etapa.

## Arquitetura herdada: o que já existia

O loader carrega o ELF ARM Android, resolve imports, inicializa JNI/FalsoJNI,
adapta pthreads Bionic, arquivos e relógios, e envia GLES à vitaGL. O áudio passa
por AudioTrack JNI e sceAudioOut com reamostragem de 44,1 para 48 kHz. Os controles
usam o modo Xperia e eventos de touch nativos. O VPK contém o port e LiveArea;
o ELF e os assets do usuário ficam fora dele.

As notas iniciais registram: helpers atômicos Linux não mapeados; diferenças no
tamanho de mutex/atributos Bionic; limites de LwMutex; pilha insuficiente nos
workers; condição de espera que retornava por timeout; conflitos de alocadores;
despacho de construtor JNI; shader vec4 com sete componentes; consultas de arquivos
soltos e cache de archives; ordem de pools de memória; geração de perfil XML;
empacotamento VPK e instalação. As implementações estão no repositório, mas os
tempos de carga, FPS e alegações de estabilidade das notas não foram todos
reproduzidos. A referência 16 conserva os detalhes sem apagá-los.

## Mapa das mudanças desta investigação

| Área | Arquivos | Motivo e limites |
|---|---|---|
| Build | `CMakeLists.txt`, `make.sh` | Rastrear flags/fontes da vitaGL, rebuild completo quando necessário, propagar falhas; esperar a compilação terminar antes de instalar. |
| Geometria | `lib/vitaGL/source/{draw,custom_shaders,shared}.c/h`, `utils/index_range.h` | Retirar bypass count>64, manter cópias com lifetime adequado; limitar cópia à faixa de índices em caminho cliente U16 estritamente delimitado. |
| Texturas | `source/utils/glutil.c`, `lib/vitaGL/source/textures.c` | Filtros cube nível zero, mipmaps da diffuse do traje, status de link antes de introspecção, software ETC1 e descritor de stride de cubemap. |
| Input | `source/main.c`, `source/utils/camera_input.h` | Histerese radial, IDs frontais +2, gesto contínuo, mínimo de tecla, centro calibrado, resposta suave por tempo e compensação de ganho nativo. |
| Diagnóstico | `source/utils/dev.{c,h}`, `glutil.{c,h}` | Estado, captura própria, comandos, probes opt-in e frescor de contadores. Capturas e dumps interferem no desempenho. |
| Perfil | `source/patch.c`, `source/utils/{perf,watchdog}.{c,h}` | Prologues verificados, trampolins ARM chamados por BLX indireto, tempos inclusivos e denominador real para FPS. |
| Imports | `source/dynlib.c` | Encaminhar os imports GL aos wrappers correspondentes sem editar o ELF do usuário em disco. |
| Documentação | README, diário, este registro, skill 13–16 | Corrigir instruções antigas, registrar sucessos e falhas, preservar evidências e manter pendências explícitas. |

O diário detalha os passos intermediários e os efeitos observados. Economizar
bytes de cópia não comprova FPS; um loop vivo não comprova imagem correta;
ausência de um crash em poucos boots não comprova estabilidade.

Atualização144–150: há um crash intermitente no cache nativo COLLADA ainda sem
causa estabelecida. Controles143/144 com marcadoresOFF/ON também iniciaram.
O diagnóstico149 publica relatório de estados exclusivamente na main thread,
adiando enquanto há amostras recursivas em voo.147 separou uma pausa de carga
de1,58s de custos pequenos em estados já estabilizados.148 confirmou Web-Strike,
combo4hits e cenário coerente após streaming/combate, mas ficou~17FPS e não
concluiu missão/save.149 voltou~25–26FPS no telhado inicial.150 não obteve ganho
relevante reduzindo o raio;151 não resolveu água azul com LowRes_Sea0. Ambos
revertidos.152 coleta materiais para identificar a causa. O helper
`tools/make_runtime_config.py` cria experimentos XML com diff/hash, preserva
arquivos existentes e é distribuído com a skill. Ver diário antes de adotar
qualquer configuração experimental.

154 também corrigiu dupla interpretação de formatos printf nos logs do port e
FalsoJNI, com teste literal emhardware. A primeira missão de combate/resgate foi
concluída e chegou LEVEL2; a persistência falhou155 e foi validada após157–159.
O backup genérico Vita contém somente metadados para este loader: preservar
explicitamente login.sav/glcloud.sav/tempSaveFile.dat emdata/tasm antes de editar.
Não confundir o nome de SaveAllBytesOfSaveFile com gravação emdisco: aquelafunção
somente atualiza arraysRAM. O diário acompanha a cadeia nativa de serialização.

155 confirmou a perda deLv2 depois de sair e reabrir.156 alinhou o namespace de
rename com fopen;157 protegeu o save anterior quando a origem está ausente,
com teste emhardware aprovado. O SDK removia o destino antes de tentar mover
uma origem inexistente.158 recolocou o checkpoint948B preservado no principal
e backup, com appfechado e readback/hash idênticos;159 confirmou Lv2 em nova
abertura sem cópia manual, com vários carregamentos posteriores. Nenhuma alegação de port100% ou de crash144
resolvido decorre dessas correções.

## Procedimento reproduzível no ambiente atual

O shell é PowerShell; o build usa `C:/msys64/usr/bin/bash.exe` e VitaSDK
`C:/vitasdk`. Na raiz do repo:

```powershell
& C:/msys64/usr/bin/bash.exe ./make.sh
Get-FileHash build/tasm.vpk -Algorithm SHA256
```

Se `exec_command` retornar `session_id`, consultar `write_stdin` até obter
`exit_code=0`. A saída do make é bufferizada por tail. Instalar antes do fim já
reinstalou o binário anterior no ensaio 082. Conferir log novo, hash e banner;
um timestamp de `__DATE__/__TIME__` não muda se só a biblioteca foi relinkada.
O checksum autoritativo é o VPK final em `build/SHA256SUMS.txt`.

Ordem de teste: fechar TASMVITA1, baixar log estável, instalar VPK pelo Vita MCP,
abrir, esperar o menu real, iniciar gameplay e inspecionar uma captura nova.
Não tratar `menu=HUD` ou `camera_present=1` como fim do loading: aparecem também
durante carga e cinemática. Usar o contador `captures` para não rever imagem antiga.
Manter comandos dev sequenciais; outro cmd.json pode substituir o anterior antes
do poll de 250 ms. Telemetria pausada exige estado sob comando; trace_frame agora
força estado novo ao concluir.

### Reprodução da primeira área

Cross inicia; outro Cross pula a cinemática quando o ícone está disponível.
No telhado, Cross salta e o tutorial muda para Hold X to Web-Sling. Segurar
Cross e mover para a frente por cerca de 1,5–2,4 s leva à área da piscina; a
posição depende do instante e da câmera. Passos curtos de salto podem sair da
água. Correção122: Cross também é o ataque corpo a corpo em combate; Square99
vai ao spray, com modificadores de ombro, e não deve ser usado para testar melee.
Circle fez Web-Strike/ranged nos passos do tutorial e contra-ação em combate.
Triangle foi a esquiva solicitada explicitamente pelo tutorial124.
Não automatizar o percurso às cegas: confirmar cada cena e o progresso do tutorial.
O comentário antigo Start108=pausa/Select109=mapa estava invertido: hardware
mostrou108=mapa e109=pausa/options. Build123 corrige o mapeamento físico.

A medição antiga do Vita tinha neutro esquerdo (145,139) e direito (101,127).
A injeção MCP soma o offset físico: right-stick 210,128 foi lido como 183,127;
128,200 foi lido como 101,199. Para ir reto para frente, left-stick 111,0 foi
lido próximo de 128,11. Esses valores são deste hardware, não defaults de ports.
Em 083 a injeção deixou de chegar ao app apesar da resposta de sucesso; reboot
normal restaurou. Não instalar plugins de memória ou stream para contornar isso.

Atualização 101–104: neutros observados (161,131)/(83,123), calibração independente
dos dois sticks no menu, sem adaptação em gameplay. Com a calibração, usar
left-stick 128,0 para frente; não reaplicar a compensação bruta antiga 111,0.
103 conferiu active/down da câmera 1 durante pressão e 0 em estado novo após
soltar. A compactação esparsa teve ganho parcial ~10,8 para ~15,7 FPS na vista
inicial detalhada; caches 098–102 foram rejeitados e estão OFF. Streaming ainda
demorou para carregar prédio mesmo parado. 104 mede reads/locks, sem otimizar
I/O ainda. Atualização 106–119: pool reduzido para36MiB após medir o pico; ETC1
2D nativo passou testes sintéticos por mip e reduziu uso de memória. Atlas sem
skip e BuildingDistHigh30000 estão sob teste no XML114. Vista inicial ~14–15,5
FPS ainda insuficiente. Cópia NEON, compactação de duas etapas e DMA também
foram rejeitados por regressão medida. O guard de link passou a preservar nomes
GL vivos após falha, com teste no Vita. Detalhes, hashes e limites no diário.

Atualização120–129: perfil separa compactação, bounds e cópia; índices EBO com
vértices cliente também passaram aos caminhos de cópia própria (FBO exato).
Tag/remap compactos125 passaram host/ARM/FBO, com ganho pequeno na vista inicial.
XML124 restaura LowLodMipmapSkip0, melhorando anúncios/fachadas distantes porque
Map__ tem política diferente de CityAtlas; memória prolongada ainda pendente.
Probe nativo identificou bufferCPU25MiB da cidade. Trocar só esse buffer para
VBO129 retirou geometria detalhada: experimento rejeitado e marcador removido,
independentemente do FPS maior.130 investigará divergências shadow/VBO, sem
adotar a mudança. Config124 e caminhoCPU continuam a referência de comparação.

Atualização130–135: comparação confirmou divergência shadowCPU/VBO e ausência
de updates desse buffer; fullcopyGPUidle131 restabeleceu bytes, mas a imagem
seguiu errada.134 reproduziu um segundo defeito independente: offsets32GL eram
truncados no campoGXM16 paraVBOpacked acima64KiB.135 rebaseia o stream e aguarda
regressãoFBO/hardware. Câmera133 tem novos defaults0.65/0.75; parcial, máximo,
reversão, vertical, ambossticks e pausa/retorno ao neutro passaram em gameplay.
Nenhum desses testes certifica missão/save, streaming completo ou FPS aceitável.

Atualização136–143: oVBOprecisa capturar as escritas do allocatorCPU além das
cópias importadas. Probes por página/byte localizaram headers inline no policy;
142preservaABI e notifica blocos1KiB, com cobertura completa nos cenários medidos.
143testa um segundo marcador de syncGPUidle por páginas4KiB, storagepróprio e
notificações atômicas. Cena detalhada e bytes convergiram em testes iniciais,
com24–25FPS na vista inicial semelhante; outrasvistas~20FPS e custos de carga
continuam. É opt-inDEBUG, ainda emvalidação. Skill/diário guardam hashes, falhas
intermediárias e rejeições; não habilitar emrelease só por esses resultados.

### Capturas, probes e REA

A captura própria do app escreve `ux0:/data/vita-mcp/TASMVITA1/frame.bmp`;
a cópia local é temporária. Não usar pngshot/PS+Start para benchmark: Start pausa
o jogo e o caminho apresentou telas pretas. Remover imagens temporárias após QA.
`material_probe.enable` e `texture_probe.enable` são opt-in e devem ficar ausentes
na candidata final. Shaders e dumps dos assets são privados, não entram no VPK/git.

REA IDA falhou no fetch e Ghidra deste provider Windows rejeitou o ELF ARM.
O fallback objdump funcionou com `REA_OBJDUMP_COMMAND` apontando ao binário do
VitaSDK. Disassembly estreita por símbolo/endereço é mais útil que despejar
116 MB de .text. Evidências estão no diário e índice; offsets valem só para
o SHA do ELF registrado. Não reaplicar offsets dos documentos históricos.

### Critérios para novas medições

Separar compilação/upload de texturas, cargas, cinemáticas e capturas das janelas
estáveis. FPS usa tempo monotônico real desde 088. Tempos de funções pais/filhas
são inclusivos e não devem ser somados. Pools RAM/VRAM/PHYCONT não incluem todo
o heap externo mapeado; `vglMemFree(EXTERNAL)=0` é comportamento desta API.
Relatório de heap custou ~265 ms; captura custou segundos. Clock pedido 500 MHz
foi rejeitado, permanecendo 444 MHz; nenhum plugin de overclock foi instalado.

## Próximo trabalho autorizado, ainda não concluído

1. Validar build sem injeção de materiais e candidata não-debug no Vita.
2. Medir física/animação/combate; investigar o maior custo confirmado antes de
   reduzir fidelidade ou eliminar lógica do jogo.
3. Conferir água azul sólida, fading pontilhado, transição triangular do céu,
   streaming/LOD e mais orientações dos reflexos.
4. Testar câmera parcial/plena, reversão, vertical, movimento simultâneo,
   pausa com stick pressionado, toque e retorno ao neutro calibrado.
5. Ampliar missões e sessão ativa prolongada. A primeira missão de combate/resgate
   chegou LEVEL2 e o checkpoint persistiu após correção de rename e novas aberturas.
   Repetir saves/cargas além desse checkpoint. A falha intermitente do cacheCOLLADA
   repetiu em163; não foi corrigida. O crash169 do batchVBO tem outra assinatura.
6. Remover marcadores/dumps temporários, registrar checksums e estado instalado.

Estes itens são planos. Só passam a concluídos com resultado registrado no diário.
Não declarar 100% ou publicar release por ter compilado ou chegado ao primeiro telhado.

## Atualização contínua da skill

Depois de acrescentar uma etapa ao diário:

```powershell
py tools/update_port_knowledge.py
```

O script atualiza o índice de evidências, copia o registro e o diário completos
à referência 15, mantém a referência 16 com as notas herdadas e sincroniza a skill
para o workspace pai. Não apaga pastas e não modifica dados do Vita. A skill
continua progressiva: carregar 14 para lições, 15 para histórico atual, 16 apenas
para reconstrução de contexto antigo. Não carregar todos os logs em cada port.


---

## Source: docs/DEBUG_SESSION_2026-11-04.md

# Investigação no Vita — 2026-11-04

Status: diagnóstico e testes de builds/boots 066–193 em andamento. Há correções locais
validadas em cenários limitados; gráficos e performance continuam pendentes.
Não considerar o port 100% resolvido nem usar esta sessão como aprovação de release.

## Alvo e reprodução

- Console acessível pelo Vita MCP, Vita Companion 1.08, título `TASMVITA1`.
- Biblioteca do APK: `apk_extracted/lib/armeabi-v7a/libSpiderMan.so`, 12.933.332 bytes.
- SHA-256: `c3de282d87fece00b797f23bb0b7cdfe778135cdbfd5678bee19484ce1b5d040`.
- Log preservado: `../_logs/tasm_066_baseline.log`, relativo à raiz do projeto
  `tasm-vita` (o arquivo fica no workspace pai e não é distribuído com o port).
- Captura inspecionada no tutorial inicial: personagem imóvel no teto, mensagem
  “Press X to Jump”, traje pontilhado com cores incorretas e texturas ruins na cidade.
- Saves copiados antes das alterações para o backup do Vita MCP
  `backups/savedata/TASMVITA1-2026-11-04T04-17-09-325Z`.

As contagens de frames dos logs demonstram que o loop apresenta imagens. Não
comprovam qualidade gráfica, correção dos inputs ou desempenho durante um percurso.
Uma medida de aproximadamente 58 FPS no menu não deve ser chamada de FPS de gameplay.
A cena parada do tutorial também não substitui um teste com movimento e streaming.

## Evidências de código e hipóteses

| Questão | Fato observado | Hipótese ou ação pendente |
|---|---|---|
| Geometria e traje | O baseline passava vértices e índices clientes diretamente à GPU para `count > 64`. | Quantidade de índices não prova lifetime. Arrays dinâmicos podem ser reescritos/liberados antes do consumo pela GPU. Comparar com cópia para armazenamento mantido pela vitaGL. |
| Texturas atualizadas | `TEXTURES_SPEEDHACK=1` remove o registro de uso e o copy-on-write de `glTexSubImage2D`. | Possível alteração de textura ainda usada por draws anteriores. Testar separadamente, após o experimento de geometria. |
| Cubemaps | `glTexParameteri/f` são imports diretos; não existe o clamp descrito pela skill antiga. A implementação de cubemap só importa nível zero. | Filtragem de mipmaps sem cadeia completa pode corromper reflexos. Confirmar filtros e comparar um clamp específico; não forçar alpha em shaders indiscriminadamente. |
| Inputs | O baseline não implementava o mínimo de 45 ms nem o deslocamento `+2` dos IDs de toque prometidos na documentação. | Ainda é necessário medir teclas, IDs, transições de menu e retorno dos analógicos ao centro. |
| Câmera | O baseline enviava `UP`/`DOWN`/`MOVE` a cada frame do analógico direito ativo e liberava em uma coordenada deslocada. | Identificar quais eventos e campos mantêm rotação após centralizar. Não concluir causa apenas pela presença dessa sequência. |
| Build | CMake e Makefile não rastreavam mudanças nas flags da vitaGL. | Comparações anteriores podem ter reutilizado objetos com flags antigas; registrar flags e recompilar todos os objetos quando mudarem. |

## Preparação do primeiro experimento

Alterações de código preparadas para comparação, ainda sem resultado registrado:

- Remover as exceções `count > 64` de `draw.c` e `custom_shaders.c`.
- Compilar com `DRAW_SPEEDHACK=0`, preservando o caminho normal de índices em VBO.
- Manter `TEXTURES_SPEEDHACK=1` nesta etapa, para isolar a mudança de geometria.
- Gerar `build/vitagl-build-flags.txt`, usá-lo e o Makefile como dependências e
  executar o make da vitaGL com `-B` quando a biblioteca precisar ser refeita.
- Preparar diagnóstico no próprio aplicativo para capturas e observação de input,
  sem instalar um plugin de memória. A integração do harness deve ser registrada
  com o hash e o log da build efetivamente instalada.

Depois da instalação, comparar o mesmo tutorial, posição e orientação da câmera.
Uma compilação ou instalação bem-sucedida, sozinha, não valida a hipótese visual.

## REA e identidade dos offsets

A tentativa de acesso ao provider IDA falhou no fetch; o provider Ghidra usado
nesta sessão no Windows não aceitou o alvo ELF ARM. Isso limita essas tentativas,
sem demonstrar uma limitação de todas as instalações dessas ferramentas.

O fallback do REA por CLI com
`REA_OBJDUMP_COMMAND=C:/vitasdk/bin/arm-vita-eabi-objdump.exe` funcionou. Evidência
preservada em `../_logs/rea-camera-touch-ended.json`:
`ev_9b3029b2f3a140dbf0a9c1455c5570b7b7c839e03a89f972cf39858fa7e0f052`.
O registro tem autoridade de artefato fornecido, digest antes/depois inalterado,
saída completa e escopo de uma função, `FullScreenRotateJoystick::touchEnded`.
É evidência estática; não observa a execução no Vita nem demonstra, isoladamente,
que a câmera foi corrigida.

Offsets relativos ao ELF identificado acima:

| Símbolo | Offset |
|---|---|
| `Game_nativeOnTouch` | `0x001b9c8c` |
| `GameGLSurfaceView_nativeOnTouch` | `0x001b9f68` |
| `XperiaTouchpad` | `0x001bb0d8` |
| `FullScreenRotateJoystick::touchBegan` | `0x00344b5c` |
| `FullScreenRotateJoystick::touchMoved` | `0x00344bd4` |
| `FullScreenRotateJoystick::touchEnded` | `0x00344d30` |

Os offsets JNI antigos `0x25fb88` e `0x260920` não correspondem a este ELF.
Os demais offsets históricos da documentação precisam de verificação por símbolo
e digest antes de qualquer patch. O loader resolve exports por nome quando possível.

## Pendências de validação

- Comparação visual antes/depois na mesma cena, sem afirmar melhora antecipada.
- Giro, parada e inversão da câmera; centralizar o stick e observar deriva.
- Movimento, salto, combate e combinações de botões; toque simultâneo aos sticks.
- Navegação, pausa/retorno e transições com botões pressionados.
- Carregamento a frio e com cache, troca de LOD e travessia da cidade.
- Verificação independente do efeito de `TEXTURES_SPEEDHACK` e cubemap.
- Salvar/reabrir, múltiplos boots, crashes e sessão prolongada em movimento.
- Registrar cada build instalada, flags, log, evidência visual e resultado real.

As duas cópias locais da skill foram corrigidas para retirar a recomendação de
zero-copy por contagem e separar o histórico de soluções comprovadas nesta sessão.

## Resultados das iterações 067–074

| Log | Alteração / experimento | Resultado observado |
|---|---|---|
| 067 | Cópia segura de vértices/índices; diagnóstico por arquivo | Boot e gameplay funcionaram. O pontilhado do traje persistiu. O pool de 8 MiB passou a estourar em cenas com cópias maiores. Não atribuir o pontilhado ao zero-copy com base nesse teste. |
| 068 | Clamp dos filtros de cubemap para nível zero | O log confirmou duas solicitações `0x2701` convertidas para linear. O traje continuou pontilhado; não é solução suficiente. |
| 069–070 | Sticks com zona morta radial/histerese; gesto direito contínuo; IDs frontais +2; teclas >=45 ms | Movimento e salto executados no tutorial. No telhado seguinte, stick direito ativo produziu `camera_down=1`, `camera_dx=28`, `camera_id=1`; ao soltar, down/deltas voltaram a zero e a orientação ficou parada. Ainda falta regressão ampla. |
| 070 | Alternância de filtro 2D na mesma cena | Não eliminou o pontilhado. A mudança de filtro sozinha não demonstra que uma cadeia de mipmaps existe. |
| 071 | Pool circular de 18 MiB (6 MiB por frame em triple buffering); profiler separa draw de copytex | **Zero avisos de overrun** no log preservado. Tutorial parado: 26–27,6 FPS, ~72 draws/frame, ~10 ms/frame de submissão e ~35 ms/frame no motor sem swap. RAM livre ~7,4 MiB, PHYCONT ~15 MiB, VRAM ~43 MiB. A performance ainda não é satisfatória. |
| 072–074 | Diagnóstico de componentes do material do traje | Na 074, capturas novas confirmaram normal map coerente e pontilhado presente até em diffuse-only. Desligar apenas especular não resolveu. Investigar mipmaps da textura de cor antes de alterar fórmulas de iluminação. |

As builds mantiveram `TEXTURES_SPEEDHACK=1`; essa hipótese ainda não foi testada.
075 prepara teste dirigido de mipmaps e mede custo dos diagnósticos; resultado
ainda não registrado. Não confundir essa preparação com correção validada.

## Resultados das iterações 075–079

| Log | Experimento | Resultado observado |
|---|---|---|
| 075 | Mipmaps dirigidos ao traje, após identificar os samplers efetivamente ligados | Diffuse RGBA 1024x1024 tinha apenas um nível; normal e especular ETC1 128x128 tinham oito. Gerar os dez níveis da diffuse e usar filtro trilinear eliminou o pontilhado severo em captura nova. |
| 076 | Correção de mipmaps por padrão, retirada de relatórios periódicos de heap | Tutorial inicial chegou a 30 FPS, frame máximo de 39 ms na janela medida sem captura. Combate no telhado seguinte ainda caiu a 16–18 FPS. Não extrapolar para toda a cidade. |
| 077 | `TEXTURES_SPEEDHACK=0`, recompilação completa da vitaGL | Não eliminou as fachadas coloridas. Mantém as regras de lifetime/copy-on-write; não é evidência de resolução visual. |
| 078 | Primeiro ensaio dos probes nativos e inspeção de material após link | Crash simbolizado em `glGetUniformLocation`, fragment shader nulo após falha de link. A consulta nova não verificava `GL_LINK_STATUS`. Corrigida; trampolins ARM passaram a usar chamada indireta BLX a partir de Thumb. Esta build não é uma candidata de uso. |
| 079 | Guards de link, probes ARM corrigidos, arquivo limitado com bytes ETC1 antes da conversão | Boot e tutorial progrediram até a piscina/inimigos. Amostra estável: 20–21 FPS, atualização do mundo ~24 ms/frame e render ~19 ms/frame. Não houve streaming/texture uploads nessa janela. Os 415 registros ETC1 (15.699.092 bytes) decodificaram no host sem o padrão de arco-íris visto no jogo; isso orienta a investigação, mas não valida todos os mips/cubemaps. |

Relatórios de memória mediram 264–267 ms e foram retirados do loop; continuam
disponíveis no boot, em falhas e sob comando explícito. A telemetria por arquivo
também deve ser pausada ao medir gameplay. Captura de framebuffer leva segundos
e suas janelas não contam como benchmark. Solicitar 500 MHz retornou erro e o
hardware permaneceu em 444 MHz; nenhum plugin de clock foi instalado.

Identidades: VPK 076 `a8621b1027df53928463520863b82c4afaa4976ca88a591528285e638cb3737c`;
079 `e55703075efe4ed7d610f993fd5d8f68db87bc804dd3b787a56e6a5e6fe04250`.
Logs locais: `../_logs/tasm_075_mipmap_fix.log`,
`../_logs/tasm_076_default_mips.log`, `../_logs/tasm_077_texture_lifetime.log`,
`../_logs/tasm_078_crash_invalid_program.log`,
`../_logs/tasm_079_engine_profile.log`.

080 é um experimento de diagnóstico: detalha colisões/objetos/combate e captura
faces de cubemap somente com `texture_probe.enable`. Marcador e gravação não
devem permanecer ativos em uma candidata final. Resultado ainda pendente.

### Boot 080 e retomada 081

080 abortou no boot: PC `so+0xa83334`, LR `so+0x4c85bc`, DFAR `5`.
O core mostra um `IShader*` de valor `1` no vetor da coleção de shaders, copiado
durante seu crescimento. O incremento de refcount tentou acessar `1+4`.
Apenas prioridade de threads e `strcmp` tolerante a ponteiros baixos não resolvem
essa falha. A causa da entrada inválida permanece aberta; não substituir
atomicamente todos os ponteiros inválidos por nulos nem chamar isso de correção.
O mesmo VPK iniciou no boot 081 e chegou ao gameplay, confirmando intermitência.

081 capturou quatro conjuntos de faces ETC1 de cubemap. As faces decodificadas no
host foram coerentes (céu/cidade), sem o arco-íris das fachadas. Os custos de
colisões e coordenador de combate foram <1 ms por janela parada, enquanto objetos
consumiram ~0,9 s em 5 s em uma cena a ~28 FPS. Os tempos são inclusivos e
dependem de cena/orientação. O marcador de coleta foi removido antes do boot 082.

### Sensibilidade do analógico direito (pedido adicional)

O mapeamento anterior produzia até 28 pixels horizontais e 20 pixels verticais
de arrasto **por quadro**: velocidade extrema e dependente do FPS. O novo
mapeamento calcula deslocamento pelo tempo, conserva frações de pixel, usa curva
progressiva e limita o tempo acumulado a 50 ms após travadas. A zona por eixo
evita inclinação vertical acidental durante uma panorâmica horizontal.

Correção da interpretação anterior: `CameraManager::DoTouchInput` usa
`Application+0x380` como delta em milissegundos (também passado a post-process),
não como largura da tela. O limiar nativo é `0.05 * delta_ms`. O adaptador
compensa esse limiar somente nos eixos em movimento; as travas de câmera do jogo
continuam preservadas. No modo follow, o fator horizontal de entrada é 1 e a
conversão usa aproximadamente meio grau por pixel efetivo.

Configuração opcional `camera_sensitivity.cfg`: `horizontal=1.0`, `vertical=1.0`,
faixa 0.25–2.0 por eixo, carregada no boot. Valores inválidos são ignorados.
Teste portátil `tests/camera_input_test.c` passou com GCC `-Wall -Wextra -Werror`:
gestos de dois segundos em 20/30/40/50/60 Hz, suavidade parcial, isolamento de
eixo, inversão, repouso e limite após travadas. Validação no hardware pendente.

082 foi um boot encerrado ainda com o VPK anterior, detectado pelo banner.
083 recebeu a build de câmera concluída, SHA-256
`4d787bb4f5f01c87880939074927a34adc90e3f7a7f9aee416c089c768c2f7c0`.

### Observação e medição

- Harness de arquivos em `ux0:data/vita-mcp/TASMVITA1/`, compilado com
  `DEBUG_SOLOADER`, sem plugin de memória nem mudanças no tai/config.
- `cmd.json` aceita `capture`, modos de material e filtros; `state.json` relata
  inputs, menu, frame, tempo e campos do rotate pad. A captura BMP lê o display
  do próprio aplicativo; só considerar imagem nova após `captures` aumentar.
- Captura em si interrompe a renderização por segundos. Excluir janelas com
  capturas, compilação ou transição da comparação de FPS de gameplay estável.
- `menu=HUD` pode aparecer durante a carga; sozinho não prova prontidão.
  Em 073 houve uma pausa >10 s seguida de retomada, preservada no log
  `tasm_073_debug_hang.log`; não foi demonstrado crash ou defeito de GPU.
- O campo build nas primeiras versões do harness vinha do horário de compilação
  de um único objeto; não substitui SHA-256 do VPK nem banner do log.
- No perfil antigo, chamadas de desenho eram contabilizadas como `copytex`.
  Desde 071, `draw_submit` tem categoria própria; `copytex=0` na cena medida.
- Usuário reforçou a performance como problema. A validação precisa incluir
  FPS, tempo de frame e travessia/streaming, além de qualidade gráfica e input.


## Resultados 083–086: sensibilidade e ETC1

- 083 recebeu a curva de câmera; o canal nativo de input MCP respondia sucesso
  mas o pad permanecia em zero/128. Reiniciar o console recuperou a injeção em
  084, sem alterações de plugins ou configuração de boot.
- 084 confirmou rotação e parada, e deslocamento parcial menor. A leitura de
  FullScreenRotateJoystick::touchMoved identificou o ganho adicional de 2x em X
  e 3x em Y para largura <=1024; o adaptador foi corrigido para compensá-lo.
  `filter_base` desativou mips apenas no experimento: não resolveu as fachadas
  e trouxe de volta o aliasing do traje. Filtros e mipmaps foram restaurados.
- 085 usou `SAFE_ETC1=1`, a opção oficial da vitaGL que desativa o caminho ETC1
  de hardware. Nas capturas da piscina e após girar a câmera, o padrão multicolorido
  repetitivo das fachadas deu lugar a janelas/reflexos reconhecíveis. A melhora
  justifica continuar esse caminho, sem atribuir ainda a causa a um erro
  específico de swizzle ou do decodificador de hardware.
- Custo de memória em 085: pools livres RAM 2584 KiB / VRAM 6660 KiB / PHYCONT
  1720 KiB; newlib em uso 8365 KiB. Esses três pools **não são toda a memória**:
  a vitaGL também pode alocar no heap newlib mapeado, configurado para 150 MiB.
  `vglMemFree(VGL_MEM_EXTERNAL)` retorna zero por implementação e não mede
  espaço disponível. Streaming prolongado ainda precisa de teste.
- O teste 085 da câmera calibrada ainda não havia sido instalado. Com compensação
  de ganho já ativa, input parcial produziu dx=4/dy=0 e soltura down=0/deltas=0;
  o giro de um segundo foi visualmente controlado. O offset físico rx=101 causou
  mistura de eixos durante input vertical, motivando calibrar neutro no MainMenu.
- 086 adiciona calibração de 32 amostras estáveis no menu e probes de
  CGameObject::Update, CMainCharacter::Update e UpdateLod. Candidato ainda em teste.

Identidades: 085 `32bb298d2ca4d73e179f3f182a165d6c7c678a390527bfe8abf41d83eee701ab`;
086 `b29dcda709e0367241e4c9ef2ef993db31873ba72004484418935ee72febae3e`.
A build de câmera sem software ETC1 foi preservada localmente como
`build/tasm-camera-c1f2272c.vpk` (hash c1f2272c...).
A documentação oficial da opção está no
[README da vitaGL](https://github.com/Rinnegatamante/vitaGL#compatibility-flags).


### Validação 086 e experimento 087

086 reconheceu o neutro físico `(101,127)` no MainMenu. Movimento vertical
injetado (raw 101,199) resultou `camera_dx=0`, `camera_dy=6`, down=1; soltura
retornou down/deltas a zero. Em 085, antes da calibração, o mesmo teste misturava
um dx negativo. Movimento/ataques/esquivas e combos foram executados no telhado;
o tutorial completo e o save seguinte ainda não foram concluídos.

Após uma sessão de combate, os pools em 086 mantiveram RAM 2626 KiB / VRAM
9122 KiB / PHYCONT 3085 KiB livres, newlib em uso 9745 KiB. Não houve um novo
crash nesses boots; isso não encerra a investigação da coleção de shaders.

O perfil 086 mostrou ~160 atualizações de CGameObject por frame, mas a maior
parte era barata; não concluir que reduzir NPCs resolve a queda. Em uma janela
de combate (~16,6 FPS), base-object total 959 ms/5 s e personagem principal
830 ms/5 s são tempos inclusivos, com possível sobreposição. Submissão de draws
continuou perto de 18 ms/frame nessa cena.

087 testa cópias limitadas à faixa de índices realmente utilizada. O caminho
anterior copiava todo o prefixo de vértices até o maior índice mesmo sem usá-lo.
Somente arrays/índices clientes U16 de glDrawElements entram no novo caminho;
VBO, base-vertex, instancing e primitivas convertidas mantêm o comportamento.
Os índices são rebased em uma cópia própria da GPU; nenhum array do jogo é
modificado e a regra de lifetime permanece. Teste host verificou equivalência
de fetch com strides/offsets variados, faixas esparsas e o limite U16.
Métricas `perf-copy` medem bytes copiados/prefixos evitados; resultado de hardware
pendente. SHA-256 087:
`a9f94ac770f1150ad90f9ede716ac5860ee47852894ebddfcc2c5cc077952877`.


087 completou boot/menu e travessia de telhados/escalada com a cópia por faixa.
Janelas registraram 35–100 MiB de prefixos evitados por amostra, mantendo as
cópias de propriedade da GPU. Ainda não há um A/B pareado que comprove ganho
sustentado de FPS em combate. Testes portáteis de câmera e índices passaram
novamente, e a inspeção visual dessas cenas não revelou regressão de geometria.
No telhado externo, newlib usava 12464 KiB de 153600 KiB mapeados; os três pools
livres eram RAM 2382 KiB, VRAM 14259 KiB e PHYCONT 1720 KiB. O espaço externo
mapeado não garante alocação contígua arbitrária, mas invalida inferir OOM pelos
três pools pequenos isoladamente. A aparência de algumas fachadas ainda é
suspeita mesmo com software ETC1; a melhora não equivale à correção total.

088 adiciona uma sonda de materiais opt-in (`material_probe.enable`) e
`trace_frame`: salva shaders/mapa de programa e uma lista binária de texturas
2D por draw em um frame. O contador `trace_frames` permite esperar dados novos.
Essas sondas são exclusivas de DEBUG_SOLOADER e as janelas de coleta não valem
como benchmark. A correção de FPS passa a usar o tempo real entre amostras do
watchdog; os 5000 ms antigos eram nominais e podiam divergir sob carga/I/O.
SHA-256 088: `9047efe4d147cbe626e13ba01c33aed27fa4e3579e6ed8ae870b5d8304b038b2`.
Marcador de materiais está ativo neste ensaio e deve ser removido ao terminar.

### 089: isolamento dos materiais de cenário

Build instalada: `19d871c85e703eb1012f74d5a8fc3329e99413240ed53b416b099d54e96e236a`.
A opção `TASM_WORLD_MATERIAL_DEBUG` injeta modos de textura-base, lightmap e
cubemap apenas nos shaders de cenário reconhecidos. Modo zero preserva a
composição original; os comandos aplicam o uniform imediatamente antes do draw.
A atualização de `trace_frames` agora força state.json novo mesmo com telemetria
pausada, evitando esperar indefinidamente por um contador antigo.

No telhado ao lado da piscina, a fachada apresentou blocos verdes/roxos no modo
original. No modo diffuse, as mesmas janelas ficaram pretas com caixilhos coerentes;
no modo reflection, o padrão defeituoso reapareceu. Isso localiza este artefato
na amostragem do cubemap, sem provar ainda a falha exata de upload/descritor.
O modo lightmap atual mostra valores já escalados pelo shader e satura em branco;
não serve para concluir que o mapa original é branco.

Câmera: input parcial raw `(183,127)` produziu down=1, dx=6 e dy=0 na amostra;
soltura voltou a `(101,127)`, down=0 e deltas=0. O valor dx é por frame e depende
do delta de tempo limitado, portanto não comparar diretamente com outro FPS.

090 testa uma única alteração: cubemaps ETC1 são expandidos sem perda RGB para
RGBA8888 antes do upload swizzled, em vez de RGB888. As texturas 2D mantêm seu
caminho anterior. Hipótese: o caminho RGB888 do cubemap corrompe a transferência
ou a interpretação na GPU. Compilação passou, resultado no Vita pendente.
SHA-256: `d0ccb3bb9bb196714671552a2ca5196dc4a6b228a7f1ff100f1ed1d52e3866f7`.

### 091–093: descritor do cubemap

090/RGBA não eliminou os artefatos. 091 (`9d0b1830912e6f4fd2d9684676c377f9090d30922097394285526332b1bdf615`)
leu 384 KiB do cubemap 701 em memória GPU. Desfazendo o Morton swizzle em host,
as seis faces RGBA apresentaram erro RGB médio **zero** contra o ETC1 original
decodificado. Alpha foi 255 em todos os pixels. Isso refuta corrupção dos pixels
nessa conversão, mas não garante a interpretação do descritor ou sampler.
Descritor coletado: formato `0x0c000000`, tipo CUBE, `word0=0x03e01490`,
`word1=0x4c070007`; mip count codificado zero (API reporta 1).

O código da Vita3K distingue count codificado `0xF` (sem mips, faces contíguas)
de count codificado zero (um nível visível, mas espaço da cadeia completa antes
da face seguinte). Evidência primária:
https://github.com/Vita3K/Vita3K/blob/master/vita3k/renderer/src/texture/cache.cpp
(`upload_texture`, condição `mip_count != 0xF`, avanço por todos os níveis).

A vitaGL cria cubemaps com count lógico 0 (codificado 0xF), mas o final de
`glCompressedTexImage2D` substituía por count lógico 1 quando use_mips=false,
inclusive para faces de cubemap. Assim +X podia parecer correto e as faces
seguintes liam offsets que não correspondiam à alocação contígua.
093 restaura RGB888 (remove a hipótese RGBA sem benefício comprovado) e altera
somente esse seletor para manter count 0 nos cubemaps sem mips. Validação no
hardware ainda pendente. 092 (`2f874b8eb45ac1ca569b623cb60e028fdca750aec1eb35fbe0a968587a5d3b68`)
adicionou binding de cube por draw e probes de UpdateCol/Sync3DObject/StateAutomat,
com prologues conferidos por objdump; não foi usado como benchmark.

093 confirmou a correção no hardware: com composição original (world_debug=0),
as fachadas de vidro vistas na aproximação da piscina passaram a refletir janelas,
céu e prédios coerentes, sem os blocos verdes/roxos do teste 089. A sonda mostrou
os cubemaps 701/122/181 com mip count lógico **0**, mantendo faces contíguas.
SHA-256 093: `6203f98b923bc63d3b7d0628c5462abf9b897e054b584f804c0c98c4902ed869`.
Isso resolve o defeito de stride observado, sem encerrar a inspeção gráfica geral:
água da piscina azul sólida, objetos/folhas pontilhados e uma grande transição
triangular no céu da introdução ainda precisam de classificação/correção.

O combate passou às instruções de ataque e ranged attack, com golpes, esquiva,
teia e arremesso observados. Não foi concluída a missão nem validado o save seguinte.
Perfil em uma janela sem captura: 85 frames/5054 ms (~16,8 FPS), render 4945 ms,
world_render 2326 ms, objetos 1508 ms, personagem principal 997 ms (inclusivos).
UpdateCol 6 ms, Sync3DObject 85 ms e StateAutomat 128 ms no mesmo intervalo: estes
subcaminhos não explicam a maior parte do custo. 094 acrescenta probes de
componentes físico/animação/combate e remove a injeção de materiais. O marcador
material_probe deve ficar desativado durante medição. SAFE_ETC1 agora é padrão
no CMake, mantendo a opção de desativar para comparações controladas.

### 094: perfil sem materiais e registro completo

Instalada 094, SHA-256 `0354fab5774c9e87d67098a6daa343aa5a464ab7887396d90c97eaa1b111348f`.
`TASM_WORLD_MATERIAL_DEBUG=OFF`; marcador material_probe removido antes do boot.
A build completou boot/menu/carga e chegou à piscina. Uma janela estável registrou
89 frames/5070 ms (~17,55 FPS), physics-component 669 ms/3289 chamadas,
blend-animation 0 ms/89, combat-component 4 ms/534, simple-animation 4 ms/6314.
Tempos inclusivos: world render 2140 ms, objetos 1454 ms, main character 682 ms.
Física passa a ser um candidato mensurável (~7,5 ms/frame), mas não foi otimizada.
A função possui um dispatch virtual e chamadas trigonométricas; ainda não há
prova de qual subcaminho concentra seu tempo.

Input vertical raw (101,199) foi recebido com right_active=1/down=1; a amostra
capturou delta zero. Portanto ela confirma ativação/soltura, não a velocidade
do pitch. Ao soltar: right_active=0/down=0/dx=dy=0. Análises anteriores 085/086
mediram deltas não nulos. Regressão ampla continua necessária.

Foi encontrada uma borda de calibração na saída do HUD: o código enviava (128,128)
para liberar o gesto, embora update_right_stick espere o centro físico calibrado.
Com calibração diagonal, esse ponto pode ficar além da zona de soltura. A fonte
agora usa s_camera_center para liberar; **ainda não instalada**, prevista para 095.

A pedido do usuário, criada a documentação de ponta a ponta recuperável:
`PORTING_KNOWLEDGE.md`, índice hashes-only de 560 arquivos de evidência e gerador
`tools/update_port_knowledge.py`. A skill recebe referência 15 com diário integral
e referência 16 com os quatro documentos históricos integrais. As notas antigas
continuam preservadas, mas têm avisos explícitos sobre alegações desmentidas.
Não há transcript integral de cada chamada antiga no workspace; lacunas não são
inventadas. O script foi executado e comparou byte a byte as duas cópias da skill.
Próximas etapas permanecem marcadas como planos, nunca como ações concluídas.

### 095–096: pausa da câmera e prioridade de streaming/LOD

095 (hash `0814ee097941b323d8d153c51b6f4b9d5fb2fb15ef2fc8e2df5f7454e6485d5a`)
instalou a soltura usando o centro calibrado. Teste com estado novo: HUD + stick
raw (183,127) => right_active=1/down=1/dx=6; abrir Start mantendo stick => menu
option, raw (184,127), right_active=0/down=0/deltas=0. Depois o stick foi solto.
Isso valida a transição testada; o primeiro ensaio havia lido state antigo e não
foi usado como evidência. Câmera ainda precisa de feedback de uso e outros modos.

Candidata sem DEBUG_SOLOADER compilou (não instalada):
`build-release-qa/tasm.vpk`, SHA `f89d42e6d6561078ba23b1ab5655006b5febb40cce9bba4dbfa58b6432c3c7e0`.
Teste de câmera host passou. O primeiro comando de índices omitiu o include path
e o shell executou um .exe antigo após falhar a compilação: resultado descartado.
Recompilado com `-Ilib/vitaGL/source` e gate de LASTEXITCODE, o teste novo passou.

O usuário reforçou como prioridade os prédios que só detalham perto e o streaming
lento com lags. Comparação das configs originais recuperadas: default tem LOD
12000/120000/1200000, lowphone preserva alcance streaming 600000, perto 10000,
raios 15000/18000. O perfil do port reduz para 4000/18000/70000, streaming 95000,
perto 6000 e raios 10000/12000. Isso altera diretamente a representação à distância;
não atribuir todo o efeito a FPS ou corrupção.

096 usa **o mesmo VPK 095** e apenas override espacial com os valores originais
acima; pools/NPCs/mips e demais campos preservados. Config anterior baixada para
`../_logs/vita_config_095.xml`. Override SHA
`45594dd2ff8169823c81ecada012f78862f1e2758c52929b5834d6dca7f3055f`.
Na vista inicial, antes de chegar à piscina, prédios à direita passaram de caixas
com fachadas simples para caixilhos, entradas e terraços/árvores detalhados.
Isso mostra efeito visual real do alcance; desempenho e streaming prolongado
continuam em medição. Override ativo neste ensaio, não é perfil final aceito.

A auditoria também encontrou main prioridade 0x40, workers 0xA0..0xBF e afinidade
herdada 0. Prioridade não substitui exclusão mútua e pode atrasar streaming quando
main está sempre runnable; isso permanece **hipótese**. Comentários que afirmavam
árvore sem lock/estabilidade garantida foram corrigidos (a inserção nativa usa
spinlock). Preparado comando `threads` para observar estados/runtime sem mudar
prioridades. A primeira declaração foi inserida dentro do guard C++ por engano;
aviso de compilação mostrou isso e o header foi corrigido antes da instalação.
`TASM_NATIVE_PROFILE` permite desligar probes nativos preservando o harness para
medir seu custo. Build com essa opção OFF será o próximo ensaio; ainda não instalada.

### 096: custo da distância e próximo ensaio 097

O perfil espacial original trouxe detalhes mais cedo, porém a vista inicial caiu
para ~9–10,5 FPS e a travessia testada ~11,8–12,9 FPS, com draws ~127/frame e
submissão ~55 ms/frame na vista inicial. Houve 926 avisos de pool. O texto
`Total of N bytes` mede **o excesso acima do limite**, não uso total. Pico de
excesso 12.991.010 bytes + 6.291.456 bytes reservados = ~18,4 MiB/frame.
`vgl_reserve_data_pool` usa fallback gpu_alloc_mapped_for_cpu + mark_as_dirty
nessa situação; não escreve deliberadamente além do pool. Isso causa alocações
extras e pressão de memória, mas o aviso isolado não prova corrupção. O teste
foi encerrado e o log baixado estável. Memory explícito: RAM 632 KiB / VRAM
776 KiB / PHYCONT 4635 KiB livres; newlib em uso 31226 KiB, capacidade mapeada
153600 KiB. Estes números não garantem margem contígua para o percurso inteiro.

097 aumenta o pool para 72 MiB totais (24 por frame), mantém o override espacial
para comparação e desativa os probes nativos com `TASM_NATIVE_PROFILE=OFF`.
Mantém harness/telemetria sob comando e adiciona `threads` para fotografar
estados e runtime dos workers. VPK compilado e instalado, SHA
`a099f81c8b6f0353766751ca1bb46a805bb9c594b84380b725fcbd202e47247d`.
A mudança conjunta do pool/probes não deve ser usada para atribuir um ganho só
a um deles; o objetivo imediato é tirar o fallback do ensaio espacial e medir.
O aumento não é aceito como perfil final antes de verificar memória e travessia.

A melhoria de distâncias não encerra o problema do usuário: falta combinar
representação distante coerente, entrega de streaming e frame time estável.

### 097: resultado e hipótese de workers revisada

Log completo preservado: `tasm_097_pool72_threads.log`. Zero avisos de overrun
no trecho executado com pool 72 MiB; isso não estabelece margem para toda a cidade.
Ainda houve ~10,7–10,9 FPS na vista detalhada parada, com ~124–126 draws/frame,
submissão ~3,2 s/5 s e solicitações de vértices ~600 MiB/5 s. Desativar probes e
retirar fallback não resolveu o gargalo; não anunciar ganho de FPS desse ensaio.
Uma captura demorou 11,5 s e atravessou a janela de 12 s do wait; o contador
atualizou depois. Timeout de captura não foi crash. Último crash segue o 080.

`threads` mostrou main no **core 1** e worker executando no **core 2**, outros
workers bloqueados em cond sema ou delay. Portanto a suposição de que afinidade
0 fixa todos no mesmo core, presente nos documentos antigos, é falsa para este
runtime. Prioridade sozinha não explica a demora; não foi alterada. O primeiro
dump usa runtime acumulado; os seguintes medem desde o último dump, não desde o
heartbeat de cinco segundos. Não converter esses números em percentuais de CPU.

### 098: experimento de snapshots de geometria

Compilada e instalada SHA
`ed7db5637cdab45458f35b93fdfc034b29693030c5ee9262df5dc93a21d00129`.
Mesma configuração espacial/pool/probes de 097; única otimização é
`TASM_CLIENT_SNAPSHOT_CACHE=ON` (default OFF até validar). Clientes >=4 KiB
podem reutilizar um snapshot GPU-owned do mesmo frame, mesmo endereço e tamanho
suficiente, **somente após memcmp de todos os bytes pedidos**. Mutação de buffers
entre draws gera outra cópia; uma troca de frame invalida os metadados. Evicção
só troca metadados, nunca os bytes submetidos. Não é zero-copy de memória do jogo.
Caminho normal packed glDrawElements e macro unpacked são cobertos; VBO mantém
lifetime estabelecido. Cache de 64 entradas, sem alocação permanente extra de
geometria. Counters `perf-snapshot` registram bytes reutilizados.

Teste host compilado com warnings como erro verificou alteração do último byte,
immutabilidade dos draws anteriores, limites do prefixo, invalidação por frame e
evicção. Passou. Resultado de hardware e benefício de FPS ainda pendentes.

### 098: zero hits no cache inicial; 099 e deriva maior observada

O cache por endereço inicial exato registrou **0 KiB** de reuso nas janelas
inspecionadas. Na vista inicial detalhada mediu ~13,3–13,9 FPS, mas sem hits esse
número não comprova benefício do cache e a comparação de posição/telemetria
não foi pareada. Não habilitar como correção comprovada. 099 amplia somente a
busca para subfaixas inteiramente contidas, deslocamento múltiplo de 16 (preserva
alinhamento), mantendo memcmp integral e propriedade da GPU. Teste host ampliado
passou com subfaixa válida, endereço desalinhado e fim fora do snapshot.

Surgiu uma nova evidência de input: neutro direito estabilizou em (83,123),
fora do limite antigo de admissão de calibração (+/-32 por eixo). Sem aprender
esse centro, o HUD entrou right_active=1 mesmo sem comando de stick, enquanto
o tutorial ainda impedia camera_down. A calibração agora admite até 48 unidades
**radiais**, com as mesmas 32 amostras estáveis e apenas no MainMenu, sem botões.
Gameplay não adapta centro. Não usar apenas maior deadzone como substituto de
calibração e curva; 099 precisa confirmar right_active=0 nesse novo neutro.

099 compilada e instalada, SHA
`63d6cc30664cba62f27920f3fc7343f7a18c32e77cba27570dd21ccc93847577`.
Config espacial e pool 72 MiB seguem experimentais. Fonte de IO também foi lida
para planejar separar leituras de archive, espera e upload, sem alteração de IO
até medir o caminho. Não houve mudança de afinidade, prioridade ou plugin.

### 099: calibração confirmada; cache de subfaixas piorou frame time

Log reconheceu centro (83,123). No HUD, raw (83,123/124) ficou right_active=0,
camera_down=0, sem tocar no stick; é o mesmo neutro que em 098 mantinha
right_active=1. Ainda falta confirmar uso prolongado e outros offsets.
Cache por subfaixas poupou ~56–58 MiB/5 s de cópias, mas a vista detalhada ficou
em ~8,1 FPS, submissão ~3,7 s/5 s. **Rejeitado como melhoria de performance.**
A comparação completa usa memcmp de Newlib, cuja assembly examinada é um loop
word a word (~12 instruções por quatro bytes), capaz de custar mais que a cópia.
Economia em bytes novamente não equivale a economia em tempo.

### 100: snapshots persistentes opt-in e comparação medida

Uma primeira candidata compilada com memcmp comum (hash fed4ff27...) não foi
instalada. A candidata efetivamente instalada é
`cdfd4172a0573f1025d84ffcbffba4dca6cdc31cd9e1e86b0c4c9c9faf0feb39`.
`TASM_CLIENT_SNAPSHOT_CACHE=OFF`, `TASM_PERSISTENT_CLIENT_CACHE=ON` (default OFF).
Ela mantém snapshots em alocações GPU-owned independentes do pool circular,
limite ativo 24 MiB/256 entradas e LRU. Consulta sempre compara todos os bytes,
agora por sceClibMemcmp; contador registra o custo real da comparação. Em miss,
copia por sceClibMemcpy. Mudança de conteúdo aloca outro snapshot; evicção agenda
o antigo com mark_as_dirty (GC nativo de quatro frames), sem sobrescrever dados
in-flight. Em falta de alocação usa o caminho circular já existente. O limite
ativo não inclui bytes aposentados ainda aguardando o GC; medir memória real.

Teste host de cache persistente passou: conteúdo alterado, snapshots anteriores
imutáveis, reuso em frames seguintes, alocação que falha e evicção adiada com
orçamento limitado. Resultado no hardware ainda pendente. Fonte SHF foi conferida:
a revisão atual usa 24 archives/16 FDs, 128 handles e buffer 8 KiB/handle; números
16/6 nas notas herdadas descrevem outra revisão e não devem substituir a fonte.

### Calibração esquerda preparada para 101

O último estado novo de 099 mostrou left_active=1 com raw (161,131) após soltar
movimento: a distância de ~33 unidades fica fora do limiar de soltura 32 mesmo
sem ultrapassar o limiar inicial 40. É evidência de deriva de movimento, distinta
da sensibilidade da câmera. A fonte agora calibra os dois sticks independentemente
no MainMenu, 32 amostras estáveis, raio máximo 48 e sem botões. Os caminhos de
saída do HUD liberam usando o respectivo centro calibrado. A curva de movimento
foi preservada, só o viés de centro é removido. Gameplay não adapta centro.
101 compilada (ainda não instalada), SHA
`142647071766c3a1107fda74a4b448fe1b8c81ba77b189bcc4132d0e61c8a8af`.
VPK e ELF preservados em `../_logs/build-101/`. 100 continua instalada para
isolar o ensaio de snapshots persistentes antes da próxima troca de build.

### 100 rejeitada; variante seguinte preparada

100 sofreu regressão grave: janelas da introdução ~4 FPS, comparações consumindo
3,0–3,4 s por ~40–60 MiB reutilizados. A sessão foi encerrada; não é candidata
aceita. Há forte suspeita de localidade de memória: gpu_alloc_mapped_for_cpu
procura RAM, PHYCONT, BUDGET, **VRAM**, e só então EXTERNAL. Ler snapshots que
caíram em VRAM pode ser muito caro. A localização efetiva de cada snapshot não
foi registrada em 100, portanto ainda é hipótese, não prova de causa.

A build 142647... com calibração esquerda e esse alocador antigo foi preservada
em build-101 mas **não instalada**; não a tratar como validada. A variante seguinte
usa vgl_memalign(MEM_ALIGNMENT, bytes, VGL_MEM_EXTERNAL), o heap newlib já mapeado
pela vitaGL, para não cair em VRAM nem forçar GC. Mantém orçamento de 24 MiB,
checagem integral de conteúdo e aposentadoria adiada; medir novamente tempo,
memória e hits. Ainda não foi instalada quando este registro foi escrito.

Foi corrigido também o isolamento de variantes de build: antes build/ e
build-release-qa/ linkavam o mesmo lib/vitaGL/libvitaGL.a. Um build podia deixar
a biblioteca com flags diferentes das esperadas pelo outro, sem sua stamp local
mudar. Agora cada build copia o resultado do make -B para seu próprio
CMAKE_BINARY_DIR/libvitaGL.a e linka essa cópia. Os objetos vendor continuam
compartilhados durante compilação; executar builds de variantes sequencialmente.
Comparações de hardware exigem hash e flags do VPK efetivamente instalado.

101 efetivamente instalada: variante external, SHA
`06fb7b104dbc271a5b56d810795133d4728ec206a84e345fa5346f323e445d9b`.
VPK/ELF/flags preservados em `../_logs/build-101-external/`. Inclui calibração
independente dos dois sticks; próxima validação deve conferir os dois centros,
ativação e soltura após movimento/câmera. A build anterior de mesmo rótulo local
não instalada continua separada para evitar confusão de identidades.

101 reconheceu os dois centros e manteve os dois sticks inativos no HUD, mas
as comparações ainda dominaram janelas da introdução (~2,8 s para ~180 MiB de
reuso). Transferir o cache para heap external melhorou a taxa observada de
comparação, sem resolver o gargalo. Não é uma candidata final aceita.

102 compilada para testar igualdade booleana NEON, com leituras vetoriais exatas
de 64/16 bytes e cauda escalar, sem overread e sem hash probabilístico. A função
só determina igualdade; não substitui memcmp genérico do jogo. Um self-test
explícito no Vita verificará a ramificação ARM compilada em offsets 0..15 e
comprimentos nas bordas vetoriais, alterando primeiro/meio/último byte. O teste
host passou para todos os comprimentos 0..512 e offsets 0..15, mas o host usa
fallback escalar e não prova o código NEON. Mantém cache external e mesmos
limites espaciais/pool para isolar o comparador. SHA
`963aeed25047d624d9108bf79674c101519c7910ffd7047565fb33df182127c8`.
VPK/ELF preservados em `../_logs/build-102-neon/`; instalação e teste seguintes.

A skill também foi atualizada nos diretórios globais OpenCode e Codex (.agents),
para reutilização fora deste workspace. Regras globais mais novas de LiveArea
foram incorporadas sem apagar referências antigas; somente referências 13–16
são sincronizadas para globais. A nova cópia Codex passou em quick_validate.py.
O gerador confere hashes das referências após cada sincronização. O histórico
não ficou restrito a uma skill local ignorada pelo Git.

102: a assembly do self-test contém vld1.8/veor/vorr NEON. O comando
snapshot_selftest retornou sucesso (marcador `snapshot self-test=0`) no Vita.
Isso verifica a ramificação ARM real para buffers desalinhados e bordas de
16/64 bytes, além do teste host escalar. Performance ainda precisa da cena real.
Memória coletada ao fim de 101: newlib em uso 93851 KiB de 153600 mapeados,
RAM pool 0, VRAM 3042 KiB, PHYCONT 504 KiB. Retenção/aposentadoria de snapshots
é custo real; a margem de memória não está aprovada para streaming prolongado.

## 102 rejeitada e 103: compactação exata de geometria esparsa

102 passou no self-test NEON no console, mas ficou em ~6,5–6,9 FPS na vista
inicial detalhada. Comparar ~315–325 MiB por janela de 5 s custou ~3,5–3,7 s;
reutilizar bytes não compensou o custo da leitura. Registro estável:
`../_logs/tasm_102_neon_comparison.log`. Cache persistente e de um frame ficam
OFF na próxima candidata. As tentativas 098–102 são experimentos rejeitados
por desempenho, não otimizações aceitas. Mantêm-se os testes para referência.

103 testa outro mecanismo: em draws GLES2 U16 ordinários, com atributos client
intercalados e sem VBO, copiar apenas os registros citados pelos índices e
remapear os índices numa cópia própria. Preserva sequência de fetches, bytes,
ordem e winding; não altera os arrays de origem nem mantém cache entre frames.
Só entra se count*2 < span e todos os atributos habilitados cabem no stride
(1..256). Instancing, base-vertex, primitivas convertidas e shaders com
gl_VertexID não são escopo. Scratch geracional cobre todos os 65536 índices.
As cópias continuam no pool GPU com a vida útil normal da vitaGL.

Teste host `tests/client_compact_test.c` passou com -Wall -Wextra -Werror:
strides 12/16/24/28/32/40/48/64, índices repetidos, registros esparsos até 65535,
origem imutável, guarda da saída, wrap geracional e todos os 65536 vértices.
Foi acrescentado comando `compact_selftest` para conferir a cópia ARM NEON no
Vita, com offsets 0..15 e os mesmos strides. Compilar não comprova o hardware.

A recuperação após compactação de contexto verificou que não havia build
rodando. O primeiro VPK encontrado tinha hash 1e46eb7a... e o make incremental
concluiu com 73d14010...; nenhum desses foi instalado. Depois do self-test,
o build terminou com exit 0 e hash
`fa2be33e511607b6e803320a8020e975bc5fc1a7372ff4efc435105b5d6a3e65`.
VPK, ELF real `build/tasm`, checksums e flags arquivados em
`../_logs/build-103-compact/`. Este foi instalado e aberto para a execução 103.
Flags: compact ON, caches OFF, probes nativos OFF, pool total 72 MiB e perfil
espacial 096 ainda ativos como experiências. Aguardando testes visuais/FPS.

103: `compact self-test=0` confirmou no Vita os strides/offsets da cópia NEON.
Na vista inicial Press X to Jump, após uploads/capturas e carga assentarem,
várias janelas ficaram em 15,66–15,70 FPS (antes ~10,7–10,9 em 097 sem cache).
~125 draws/frame; submissão ~2,60 s/5 s, cópia ~345–348 MiB/5 s e compactação
evitou outros ~415–420 MiB/5 s. Nenhum cache ativo. É melhora parcial observada,
não benchmark de percurso completo nem prova de 30 FPS. A fachada à direita
ainda era uma caixa borrada na captura 3, frame 1292, e apareceu detalhada na
captura 4, frame 2496, sem deslocamento do personagem. Isso confirma atraso de
streaming mesmo parado com o perfil espacial maior, que continua não aprovado.
Capturas têm pausas de 3–13 s e são excluídas das janelas estáveis.

103 percorreu salto/web-sling até piscina; combate/esquiva/teia e mudança da
câmera funcionaram, personagem terminou no toldo verde. Geometria e reflexos
estavam coerentes nessas vistas, água continuou azul sólida, folhas pontilhadas
e texturas distantes ruins. Missão/save ainda não concluídos. Sticks em neutro
ficaram inativos em estados novos. Teste explícito na área da piscina: frame
3756, right 165/124, active=1, camera_down=1, dx=6; após release, frame 3764,
right 84/124, active=0, down=0, dx/dy=0. Isso comprova soltura da câmera sem
rotação presa nesse caso; a sensibilidade completa ainda requer regressão.
App fechado e log estável `../_logs/tasm_103_compact.log` preservado.

## 104: medir I/O antes de alterar streaming

Adicionados contadores de reads físicos dos arquivos compartilhados SHF:
chamadas, bytes, tempo lendo/abrindo, espera pelo lock, máximos, parcela do
main, reaberturas e erros. A aquisição do descriptor e sceIoPread continuam
protegidas pelo mesmo lock para impedir fechamento LRU durante uso. Não foram
alterados buffer, limites, eviction, prioridades ou dados. Watchdog usa trylock
para snapshot/reset; se ocupado, adia o intervalo. A primeira amostra inclui
o boot; amostras adiadas não devem ser interpretadas como throughput exato de
5 s. Tudo habilitado apenas em DEBUG_SOLOADER; release tem função vazia.

Build exit 0 (warning mktemp herdado), VPK
`16a7e6ef29648e7f8fbe09c9af58a9664d27bbb054efed176e1cc8d1c8a701a7`,
arquivado com ELF/flags em `../_logs/build-104-io/`, instalado e aberto como
execução 104. Menu alcançado em ~49,4 s do processo. Aguardando medição na
carga/percurso; pool/perfil/compactação permanecem iguais aos de 103.

104: carga inicial registrou reads/open de até 524/905 ms, com worker esperando
lock até 787 ms numa janela de carga. Carga de actors.gla leu ~35 MiB em ~3 s.
Depois, citys.gla consumiu ~100–540 ms de leitura por janela, main quase sem
espera; cena estabilizada só lia áudio (~136–194 ms/5 s), sem upload de texturas
e ainda ~14,5–15,7 FPS. Isso não aponta para lock de arquivo como gargalo
contínuo do telhado. O relatório não mede decode/integrar objetos e a primeira
amostra inclui boot. Próxima versão registra a janela real para evitar erro de
throughput quando trylock adia uma amostra. Sem mudança de política de I/O.
Log fechado estável `../_logs/tasm_104_io.log`. Memória: newlib ~68,5 MiB em uso,
RAM livre 0, PHYCONT 214 KiB e VRAM ~3 MiB; sem prova de margem em sessão longa.

REA MCP tentou objdump do MSYS e falhou arquitetura UNKNOWN, evidência
`ev_de56cb7a058974b206e8d6625896f6cb82685f842e2a7e75a31affb9f5b3a378`.
Fallback CLI com REA_OBJDUMP_COMMAND=VitaSDK passou e guardou:
`rea-stream-async.json` ev_5ea9c0a947ad9ffbebce02b1a8be729aeca8a6c9b7beed8ff803a937fee15ac9,
`rea-stream-yield.json` ev_e3ea8a882a46bf309518db33882feda0d2ac93d3b00723c0d98a9f96c3455bf3,
`rea-grid-process.json` ev_94341bfbb0ab5f8551fdb124f7e2c2b2dd25391accbec8b0e69c6137ab30b0b1.
ReadASyncThread::Run lê blocos até 16384 e chama glf::Thread::Yield entre eles;
Yield salta para sched_yield. O port mantém no-op herdado; não foi alterado.
CRegularGridStreaming::doYield retorna true. Não inferir que alterar yield ou
prioridades resolve streaming sem rastrear consumidores/callbacks. Comentário
single-core em sys.c foi corrigido, sem mudança comportamental. A análise do
alocador também encontrou vglMalloc/vglMemalign/vglCalloc já tentando external
antes de VRAM; não aplicar a suposição antiga de spill prioritário a esses três.

## 105 em preparação: cópia vetorial própria e localização da memória

O custo de submissão/cópia segue alto. Testa-se NEON só na cópia contígua para
snapshots client próprios, sem substituir memcpy do jogo e sem emprestar arrays
mutáveis à GPU. Novo helper respeita bordas de 64/16 bytes e cauda escalar.
Teste host passou tamanhos 0..2048 e offsets 0..15 com guardas antes/depois.
Self-test no console e microbenchmark CLIB/NEON foram acrescentados ao harness;
o benchmark exclui alocação/verificação e deve ser excluído do FPS de gameplay.
Instrumentação classifica origem/destino por pool, mede o tempo de cópia e o
pico real de solicitações ao pool circular. Arrays fora dos pools conhecidos
são classificados external pela API existente; essa categoria não prova posse.

O primeiro link 105 falhou porque vgl_circular_pool_global_peak só existe com
HAVE_DEBUG_INTERFACE. Nenhum VPK antigo foi instalado. Corrigido com contador
próprio vgl_client_pool_peak na reserva do pool, sem habilitar a UI de debug.
Rebuild em andamento; hash/teste no console ainda pendentes.

105 concluída/instalada com
`3c6f1c3e36813ad7fc4d13b5b64ae60153025dea0085b45744e36948b096bc55`.
O hash intermediário b0d34a51... não foi instalado: incluíram-se janela real de
I/O e correção do comentário antes do rebuild final. `build-105-copy/` contém
VPK/ELF/flags efetivos. O self-test real no Vita passou (client copy self-test=0).
A assembly confirma os loops vetoriais e as 32 repetições do benchmark.

105 REJEITADA por desempenho: 8 MiB EXTERNAL->RAM quente custaram CLIB 9553 us
contra NEON 19536 us, bytes iguais. Na vista inicial detalhada ficou ~10 FPS,
contra ~15,7 de 103/104, cópia contígua ~1,9 s/5 s. Origens medidas não eram
VRAM: ~65 MiB RAM, ~1 MiB PHYCONT e ~133 MiB external por janela; destino RAM.
Não atribuir esse gargalo a leitura de VRAM sem medida. Pico total solicitado
ao pool circular ~5278 KiB/frame. O microbenchmark cabe no cache e não representa
tráfego de gameplay. App fechado; log estável `tasm_105_copy_rejected.log`.

106 reverte NEON_CLIENT_COPY=OFF e testa pool total 36 MiB (12 por frame),
devolvendo 36 MiB ao allocator compartilhado, com mais de 2x o pico observado
com compactação. É uma hipótese de pressão de memória; a alocação própria de
fallback permanece e não se aceita o tamanho sem percurso/registros de pico.
O perfil espacial amplo continua ativo. A comparação relevante é com 103/104,
não com a tentativa NEON rejeitada. Compilação e hash ainda pendentes.

106 instalada SHA
`1aa6f0865c384d715e751a43ddcaccca6f60ebb87a4679b44da4ff59575bb14e`.
Arquivo `build-106-pool36/` contém ELF/VPK/flags. No telhado estabilizado ficou
~14,8–15 FPS, sem ganho sustentado de FPS contra 103/104. Newlib em uso caiu de
~68,5 MiB para ~36 MiB; RAM livre ~3,3 MiB, PHY ~1 MiB, VRAM ~3,2 MiB. Pico
no telhado ~5655 KiB, depois do web-sling/climb chegou a 8160 KiB, sob 12 MiB.
Nenhuma mensagem de overrun/falha nesse trecho. Isso melhora margem de memória,
não comprova orçamento em toda a cidade. Cópia contígua CLIB ~1,1 s/5 s entre
RAM/PHY; ainda há outros custos na submissão e no update. Log estável fechado:
`../_logs/tasm_106_pool36.log`.

Experimento sem restart: filter_base em 106 deixou fachadas mais nítidas mas
não resolveu os anúncios borrados, e trouxe de volta ruído severo no traje.
filter_original restaurava só os pedidos nativos, não o filtro trilinear
aplicado depois pela correção do traje. suit_mips reaplicou o filtro e a captura
6 voltou ao traje estável. Corrigido em fonte para invalidar o cache do traje
ao restaurar filtros, de modo a reaplicar o estado efetivo no próximo draw.
Essa correção ainda não estava no VPK 106; não chamar isso de regressão da
compactação. Capturas 4/5, antes do suit_mips, não são QA do filtro final.
Uma espera por log usou padrão errado (suit mip, em vez de suit sampler) e
deu timeout; captura/frames novos confirmaram vida do app e correção visual.

## 107: resolução dos atlases da cidade, mesmo VPK de 106

REA getMipmapCountToSkip em 0x26e018 verifica primeiro o substring CityAtlas e
consulta Texture_AtlasMipmapSkip. Evidência CLI `rea-city-mip-policy.json`,
`ev_627f26f1959d6236c60b3bb341baf8e92cc3e48dffe6dcc18194635b6f02cba2`.
Isso direciona o teste ao atlas da cidade, sem mexer em todos os mips/LOD.
O override 107 muda SOMENTE AtlasMipmapSkip 1->0 em relação ao perfil 096.
Gerador `../_logs/make_texture_config.py` valida a propriedade e preserva a
origem. XML `config_atlas0_107.xml`, SHA
`4b4ed9196215c49d9831f43c97c5a969beddb619b4edc095cce5ab880e623f11`.
Foi enviado ao override autorizado com app fechado e aberto para execução 107,
sem reinstalar: continua VPK 1aa6f086..., pool 36 MiB e cópia CLIB. Aguardar
qualidade, memória, tempo de carga e FPS antes de aceitar esse override.

Fonte também corrigiu comentário falso sobre 60 FPS/single-core no config e
ignorou build-release-qa/ para não misturar outputs de QA com mudanças de código.
Não se apagaram os binários antigos; seus hashes/flags continuam arquivados.

107 foi REJEITADA por falta de memória. AtlasMipmapSkip=0 provocou alocação
GPU de 12582912 bytes, tentativas de GC e malloc nativo de ~11 MiB falhando.
App foi encerrado e override 096 restaurado com app fechado. Log estável
`tasm_107_atlas0_rejected.log`. A captura pedida deu timeout e o BMP antigo da
execução 106 foi baixado; ele é explicitamente inválido para QA de 107.
Aumentar atlas RGB888 não é solução viável sem rever formato/memória.

## 108–111: teste sintético ETC1 de hardware

Adicionado source/utils/texture_selftest.c, executado apenas DEBUG + marcador
etc1_selftest.enable imediatamente depois de gl_init, antes de JNI. Usa dados
sintéticos, pequeno FBO e shaders próprios; override global vgl_etc1_native_probe
só fica ligado durante cada upload do teste. Normalmente é zero. O pipeline
normal mantém software ETC1 durante todos esses runs. Nenhum asset foi alterado.

108 D6999916B594A7B2FC3DEF0F7A4EA8FB5690AC1521CE1D3D26CFFAF2FE1D3EB4
produziu referência e saídas pretas, logo igualdade zero era INCONCLUSIVA.
109 D1524B57455238AE92604CC2E9A12A655FFA99EDC93D293D4DF1838F0252B935
acrescentou controle por clear/readback, localizações reais de atributos e
validação da variação da referência. Clear leu 64/128/191/255 corretamente,
mas draws client glDrawArrays ainda não mudaram a imagem. O teste abortou
corretamente como INVALID, sem aceitar igualdade de saídas uniformes.
Não se concluiu a causa desse draw de diagnóstico; não é prova de falha ETC1.

110 5C7876C37370B648D54E41C38C9441EF0C3DA271B1396ED73534F9C5516CF4EB
usou VBO próprio e glDrawElements de triângulos; a referência passou a variar
em 1008/1016 dos 1024 pixels. ETC1 original 16x16 e 32x16 deu erro RGB zero
contra software lido pelo mesmo shader. Transformações de byte/word order
propositadamente incorretas deram MAE ~92–94/255: o teste discrimina erros.
Isso valida swizzle/base para essas entradas, não todo o jogo nem todos os mips.

111 6C1AA369BD7A8EE19ED7D8C4FAF43F5935E0EEDE375271579A2FB8F820AE476E
ampliou para modo diferencial ETC1 válido e cadeia 32x16 -> 1x1: cada nível
isolado (offsets 0/256/320/336/344/352) deu erro RGB zero. Outra cadeia com
cores sólidas por nível e UV escalada testou seleção real de LOD 0..5: erro
zero em todos. O sampler nativo e os offsets dessa cadeia funcionaram. Logs
fechados preservados `tasm_108_black_probe_invalid.log`,
`tasm_109_uniform_probe_invalid.log`, `tasm_110_etc1_base_valid.log`,
`tasm_111_etc1_mips_valid.log`; ELF/VPK/flags de cada em build-108..111-*.
O pull 111 reportou EEXIST apesar de arquivo local existente com os resultados;
outro destino foi usado depois de fechar o app para evidência estável.
Marcador removido antes do próximo teste de gameplay.

Achado de código: SAFE_ETC1 expande ETC1 para RGB888, 6x os bytes da compressão,
e uploads de mips >0 chamam gpu_alloc_mipmaps em vez de consumir os pixels
decodificados. É comportamento observado nesta revisão, não prova isolada da
causa de todos os borrões. A API GXM já é inicializada com EXTENDED_FORMAT;
o teste não alterou firmware/plugins. README oficial confirma que SAFE_ETC1
desabilita o hardware: https://github.com/Rinnegatamante/vitaGL#compatibility-flags

112 em preparação: desativar software ETC1 só para permitir o caminho 2D nativo
já existente; cubemaps continuam decodificados por software com face stride 093.
Manter pool 36 MiB e override espacial 096 para isolar formato. Medir cidade,
traje, memória, carga e FPS antes de tentar atlas maior. Sem aprovação de release.

112 compilou exit 0, hash
`e7851353a32eba1b1b51885e9f0691cac56fa3eac06e2247f72b508b82c150cd`;
arquivado ELF/VPK/flags em build-112-native2d, instalado e aberto após remover
etc1_selftest.enable. Caches persistente/frame e cópia NEON OFF, compactação
ON, probes nativos OFF. TASM_SOFTWARE_ETC1=OFF é experimento ativo; o default
do CMake ainda fica ON até resultado de gameplay. App não modifica arquivos
originais do jogo nem firmware. Aguardando estado/captura nova da execução 112.

112: menu em ~41,4 s; calibração automática acompanhou a mudança dos neutros
físicos para (142,139)/(115,125), os dois inativos ao soltar. No telhado, traje,
fachadas e reflexos estavam coerentes nas capturas novas sem expansão 2D.
FPS estabilizado ~13,8–14,1 (não há ganho contra 106); submissão ~2,85 s/5 s,
cópia contígua ~1,52 s/5 s. Fonte de cópias RAM/PHY, sem VRAM. Pico do pool
~6,9 MiB no telhado e ~9,3 MiB ao atravessar/climb, dentro dos 12 reservados.
Newlib em uso ~171 KiB, VRAM livre 40984 KiB, RAM 2207 KiB, PHY 1023 KiB.
O ganho de memória é grande, mas falta percurso longo/mais materiais antes de
adotar nativo como default. Nenhuma mensagem de overrun/allocation/link/compile
failure no log coletado. O personagem ficou num canto durante web-sling/climb,
confirmado por captura, sem afirmar missão concluída. Água ainda não validada.
App fechado e log estável `tasm_112_native2d.log`.

Também foi verificado o arquivo histórico texture_probe.bin: 1.961.349 blocos,
1.715.462 em modo diferencial, zero com soma base+delta fora de 0..31. Isso
enfraquece a hipótese de conteúdo ETC2 inválido rotulado como ETC1 para esses
415 uploads, sem provar todos os assets/mips. Não foram alterados os dados.

113 repete AtlasMipmapSkip=0 com o mesmo VPK 112 e ETC1 2D nativo, mantendo
distâncias 096/pool 36/caches OFF. Override é exatamente config_atlas0_107.xml
4b4ed919..., enviado com app fechado. Medir a mesma imagem, memória e tempo
até detalhe. A falha 107 continua registrada; 113 é teste novo com formato
diferente, não uma repetição sem mudança causal.

113 completou carga com atlas nativo maior. Piso do telhado ganhou definição;
os anúncios distantes continuaram borrados. VRAM livre ~6497 KiB, newlib em uso
~11776 KiB, RAM livre 850 KiB; sem malloc/overrun no trecho. Cena inicial
estável ~14,3 FPS. Log fechado `tasm_113_native_atlas0.log`. Revisão do log
encontrou LINK FAILED/missing fragment shader em alguns programas, mesmo sem
crash: qualidade completa não está aprovada. 112 não tinha esses avisos.

114, mesmo VPK 112, mudou somente BuildingDistHigh 12000->30000 em relação a
113. XML `config_highlod_114.xml`, SHA
`1462b86db589d6a9b67070431bfe275c596107234433d281e2b2b87108b8d9ab`.
Primeiro comando Python inline falhou por quoting, sem gerar/push de arquivo;
substituído por make_highlod_config.py com assert e parse XML. Esse arquivo foi
enviado e app reaberto. A vista não resolveu os anúncios e não prova todos os
prédios/rota. Após Cross entrou na pose de salto, ~15,4 FPS, ~124 draws/frame;
comparação com a pose no chão não é benchmark idêntico. O perfil 114 fica em
teste para as próximas builds; não foi transformado em default. Log estável
`tasm_114_highlod.log`. Um estado novo tinha botão intercept 0x10000, que o
port já libera; não atribuir falta de input nesse instante a sensibilidade.

115 (73fd67a1e387f60e1bec958543bd24892c6e89eb0e93b712f2adf54c7a83d995)
testou compactação em duas etapas com perfil 114. Primeira varre índices e
mede vértices únicos; só reserva saídas se poupa >=25%, span>=16 KiB e
count<3*span. Segunda copia exatamente esses registros e remapeia índices
próprios. Isso evita reservar count registros quando unique é muito menor.
Teste host estendido passou (todos os índices, strides, guardas, wrap e fontes
imutáveis); `compact self-test=0` no Vita conferiu os dois caminhos ARM.
Pool peak inicial ~4430 KiB, cópia contígua ~610 ms/5 s e menos bytes, porém
FPS inicial ~13,5 e submissão ~2,86 s/5 s: não é ganho de gameplay aceito.
Aumentar varredura/gather pode consumir a economia de memcpy. Preparar reversão
para uma etapa ao isolar a correção seguinte. Log ainda ativo neste registro.

## 116: corrigir vida útil de nomes GL após falha de link

Auditoria identificou bug concreto no guard de glLinkProgram: PROG_INVALID=0
é o sentinela de slot livre usado por glCreateProgram, mas o guard o usava
quando faltava shader. Isso faz glIsProgram falso, permite reutilizar um nome
ainda possuído pelo jogo e bloqueia glAttachShader (exige PROG_UNLINKED).
Corrigido para manter PROG_UNLINKED e GL_LINK_STATUS=false. Validar nome antes
de acessar o array também evita acesso fora de limites em nome zero/inválido.
Isso corrige a semântica do guard, sem afirmar que explica o crash 080 ou todo
shader ausente. Será testado no Vita antes de concluir causalidade visual.

Novo program_selftest.enable opt-in de boot: link sem shaders deve falhar mas
preservar glIsProgram; outro CreateProgram deve ter nome distinto; anexar só
vertex continua falhando; anexar fragment deve recuperar o primeiro programa.
O teste usa shaders sintéticos e não desenha nem substitui materiais do jogo.
Build 116 ainda compilando; não instalado até exit 0/checksum e flags isoladas.

115 foi encerrada; log `tasm_115_two_pass_rejected.log`. O intermediário
116 5632a36e... foi compilado com duas etapas ainda ON e não foi instalado.
Após reversão isolada, 116 instalada é
`2667c28bcd7bfa45d443a00ad9a748abf63ef5233eb98b7f3937e78d41748765`,
com archive build-116-program-lifetime/ (ELF real/flags/VPK). Mantém 114 XML,
native ETC1, pool 36, compactação de uma etapa; nenhuma outra tentativa cache.
O self-test no Vita retornou `result=0 first=1 second=2 link=1`: duas falhas
de link preservaram o nome 1, CreateProgram deu 2 e anexar o fragment recuperou
o nome 1. Os dois avisos missing shaders anteriores ao resultado são induzidos
pelo teste. Isso valida a correção de vida útil, não o desaparecimento de todas
as falhas espontâneas. Remover marcador depois da coleta e medir o jogo.
Fonte primária de semântica consultada: OpenGL ES 2.0 registry da Khronos
https://registry.khronos.org/OpenGL-Refpages/es2.0/xhtml/ (LinkProgram/IsProgram).

116 gameplay: captura nova no telhado manteve traje/fachadas, ~15,5 FPS nessa
vista, sem avisos espontâneos de link/malloc/overrun no log coletado (só os dois
missing shaders induzidos pelo teste). Andou/saltou, caiu na fachada e escalou;
não concluiu a missão. Pico circular chegou a ~9348 KiB, abaixo de 12 MiB.
Memória na escalada: newlib ~12468 KiB, VRAM livre5750, RAM1598, PHY1023 KiB.
App fechado, log estável `tasm_116_program_lifetime.log`, marcador removido.
Um Cross de 160 ms no menu não iniciou a partida; estado confirmou o raw Cross
pressionado, novo ciclo de 500 ms entrou em HUD. Não foi tratado como falta de
injeção nem solucionado com reboot. Regressão de botões curtos ainda pendente.

117 `2dafb2eb0b214f3d80a75815f659a3c52787461f11acca823653e7a4389c3b60`
adicionou benchmark DMA somente em buffers próprios e caller/tid a LINK FAILED.
Sem mudar cópia real do jogo. Ambos pools RAM/PHY foram recusados pelo guard
herdado de endereço <0x81000000; nenhum DMA foi executado. Log estável
`tasm_117_dma_admission.log`, ELF/VPK/flags build-117-dma-bench/.

118 `e96819e23dd1c6d29e0bca85f5452b83a33ad950a32d5ad37172010b88291b17`
testou um DMA de 8192 bytes por alinhamento (offset0/1), em buffers próprios
cached/mapeados reais: origem RAM0x94a6e620/PHY0x95f00250 e destino0x8d147040.
API retornou0, bytes e guardas corretos; DMA89–102us versus CLIB7–8us. Isso
rejeita DMA para esses blocos pequenos. Não houve mudança do renderizador.
Log estável `tasm_118_cached_dma.log`, archive build-118-cached-dma/.

119 em preparação amplia só o benchmark para working set4MiB por pool,
blocos8/64/256KiB e offsets0/1, sem repetir só a mesma região quente. Verifica
4MiB inteiros e guardas fora do timing, alocação própria falha sem forçar GC.
São até8MiB de scratch no mesmo frame, abaixo dos12 disponíveis no menu;
excluir essa janela de qualquer FPS e nunca rodar no meio de uma carga pesada.

REA/read-only buffers: glDrawElements nativo passa por0x49df2c e glBufferData
tem chamada0x49f68c em caminho que usa IDs/flags próprios. Dumps estreitos
native-draw-elements.txt/native-buffer-upload.txt. Não reintroduzir zero-copy
nem patchar flags sem comprovar a vida útil/dirty paths. CustomColladaFactory
getVertexBufferConfig em0x268cd4 inspecionado viaREA, evidência
ev_00120bf89c7accecea3a9b465c05fa0328a96dd0cb1390d1dfb98f954c1f7580.
O GOT0xc3b5ec aponta para flag0xc43988, valor inicial1. ScanARM PC-relative
encontrou leituras nas factories de vertex/index; não prova todos os writes ou
o significado do flag. inspect_buffer_flags.py valida SHA do ELF e só lê.
Evidência native-buffer-policy-refs.json. Nenhum offset/flag nativo foi alterado.

119 instalada: SHA `2ac5607cf2ecdabfce9c3f4b0d1146b8f61601681802fe5c75035520aefc698b`.
Todos os 12 casos (RAM/PHY, 8/64/256 KiB, offsets0/1) retornaram0 com bytes e
guardas corretos. Para4MiB, CLIB16,5–20,2ms e DMA28,7–55,5ms: rejeitado para
cópia real. `tasm_119_dma_working_set_progress.log` preserva os resultados;
benchmark não autoriza DMA no renderizador. O endereço cached aceito pela API
não garante desempenho. Não extrapolar a taxa do menu para gameplay.

120 planejado: somente instrumentação da compactação/scan de índices e probes
nativos já verificados, para separar cópia/gather/preparação GL de física e
animação no perfil atual. Mesmo XML114, pool36, native ETC1, compactação de uma
etapa e cachesOFF. Medir telhado parado após streaming e fora das capturas.
Não desativar lógica do jogo nem reduzir distâncias para fabricar ganho.

Preparação120: bash -lc mudou cwd para/home/leand, então cmake não achou o
repo. bash -c preservou cwd, mas faltava/usr/bin no PATH. Com o mesmo PATH de
make.sh e bash -c, configuração funcionou. Nenhuma build dessas duas falhas foi
instalada. Log119 final estável: tasm_119_dma_working_set.log.

120 instalada `6d288cee361a2471d7a9bcbf0b58aec3a12d47134715fa78a3ef14a765354141`,
archive build-120-geometry-profile/. Telhado inicial confirmado em captura2,
telemetria pausada, sem input. Janelas estáveis:14,5–14,9FPS, ~125draws/frame,
copy contíguo1,38–1,39s/5,1s, compact0,78–0,79s, bounds0,16s, submit2,66–2,68s;
render mundo3,75–3,80s e update0,90–0,92s inclusivos. Física~0,05s, animação e
combate pequenos. Isso prioriza geometria, não cortes de física. Histograma
dominante stride32..47, compactados~51MiB versus~340MiB de cópia contígua.
Medições com probes têm overhead; cpu_main pode superar intervalo e não é taxa
confiável de utilização. I/O depois da carga somente áudio, ~140ms/5s, main0.
Capturas e trace_frame explícito excluídos. App fechado, log estável
tasm_120_geometry_profile.log. trace_frame coleta texturas, não layout dos
vértices; não reutilizar esse arquivo para concluir packing/vida útil.

121 adiciona geometry_trace sob comando: próximos128 drawsElements, metadados
de programa/índices e atributos ativos/enable/stride/pointer/VBO. Não copia
assets nem altera render. Verificar se layouts intercalados estão caindo em
cópias separadas por atributos desabilitados ou strides distintos; sem alterar
packing antes de obter amostra. Mesmas flags120, sem novo patch nativo.

121 instalada SHA `29b7c5a39739fea3d3a42553f6c87ab78e1cf1af015952fe7a31c047a2617575`,
archive build-121-geometry-trace/. geometry_trace_121.csv mostrou vários
desenhos do mundo com EBO e vértices intercalados cliente stride40, todos os
atributos ativos habilitados. A hipótese de atributos desabilitados duplicarem
essas cópias não se confirmou nesses desenhos. Os ponteiros de EBO são nomes
GL reais nesta vitaGL; coluna signed pode aparecer negativa. Não interpretá-la
como erro. A amostra opt-in não mapeia EBO para ler bounds, low/high0 nesses
casos são desconhecidos. App fechado, log tasm_121_geometry_trace.log.

122: descoberta no código: glDrawElements passava range_min/compact_indices
somente quando !gpu_buf (sem EBO). Mas os atributos cliente podem usar EBO, e
src já é ptr_do_EBO+offset. Permitir esse caminho U16/primitiva nativa elimina
o bloqueio artificial; o guard all_client permanece e os índices remapeados
continuam próprios. Não altera dados do EBO nem usa vertex zero-copy; VBOs de
vértices seguem normais. Teste novo opt-in indexed_client_selftest.enable no
boot compara FBO32x32 inteiro com referência VBO: EBO offset4 + prefixo64,
sparse64/1000/32000/65535, planar com strides8/12 e VBO+EBO normal. Referência
precisa cobrir>=1000pixels e variar>=900; verificar GLerror e binding EBO.
Compilando; resultado de pixel/performance ainda pendente.

122 instalada SHA `75339183b1c1bf8f7805d2a575d540230b000d0d4db6a5b28df49da4ac921dca`,
archive build-122-ebo-client-range/. Self-test passou os4 casos: drawn1024,
variation1023, diff_bytes0, GLerror0 e EBO_preserved1, result0. vita_wait não
encontrou o resultado porque a linha43–47 já estava fora do tail; pull completo
confirmou. Marcador removido após coleta. Fotos de telhado equivalentes120/122;
após carga e fora das capturas:15,1–15,2FPS vs14,5–14,9, ~125draws/frame,
contiguous_copy1,19s/5s, compact0,94s, bounds0,22s, submit2,58s. É ganho pequeno,
não resolver performance. Pool peak~5,4MiB no telhado e8,55MiB em combate.
Sem corte de distância/qualidade, XML114 ainda experimental.

Percurso122: Cross250ms do telhado, depois Cross+forward2400ms levou a salto
alto. Mais forward1000ms terminou na piscina e começou tutorial Web-Strike.
Circle250ms avançou para ranged; outro Circle levou à borda do terraço; ficou
agarrado na fachada. Cross+left-stick210,20 por1600ms subiu ao combate. Saúde
caiu durante os testes; missão ainda NÃO concluída/save NÃO validado.
Água continua azul sólida, anúncios distantes borrados. A sessão durou mais de
14min sem crash observado, com muitas capturas; isso não é benchmark contínuo.

Input122: telhado inicial recusou câmera apesar de right_active1; não concluir
sensibilidade nessa etapa bloqueada do tutorial. Na piscina frame5479, direito
parcial injetado182 -> raw169,124, active/down1 e dx4/dy0; soltura confirmada em
frame5491, down0 e deltas0. Full255 ->raw241,125, active/down1 em8344; soltura
enviada, mas matriz completa de sensibilidade/reversão/vertical ainda pendente.
Uma tentativa tinha rawbuttons0x10000/intercept e active0; nova tentativa normal
funcionou sem reboot. Estado antigo pode satisfazer soltura quando a tentativa
anterior foi interceptada; usar captura nova/contador e raw neutro.

Os8 ciclos de Square chegaram ao app (0x8000 no log), mas não fizeram melee.
REA confirmou ProcessMenuHUD2622f0 usa99 para GetSprayButton, a menos de
modificadores102/103. Cross23 com contexto de combate acessa GetCombatButtons
e produziu hits reais. Não é defeito de injeção nem causa para mexer no polling.
Start108 abriu mapa (captura22), Select109 abriu pause/options (captura23).
Os comentários herdados descreviam essas ações ao contrário. Corrigir as ações
na documentação e Start/Select na build123. Não alterar volume/opções de áudio.
REA: xperia Update ev_618640807b5932d60e1f15fbc7d32a562b6d4785831a5c8c4ba89d9ea2b6e17a,
HUD ev_d6a8c2a452a2f81400e468e2b1fb2e976c3d0e6769340ddbe8c5fedd4cecf0bd,
arquivos rea-xperia-*-122.json e xperia-*-122.txt. KeyToIndex confirmou
99->3,1004->2,23->1,100->4,108->10,109->9,102->11,103->12.

Ferramentas: cadeia40 comandos de input foi recusada Too many chained commands;
32 (8 ciclos press/wait/release/wait) funcionou. Várias buscas rg com wildcard
no argumento de caminho falharam no PowerShell; usar diretório e -g '*.c'/'*.h'.
Não inferir ausência do código de uma busca que retornou erro de caminho.

123 instalada `c171871c8ae7a898c8eae36ef1f50a615c5b38a4d3155a9a6f12ea196d373b64`,
archive build-123-start-select/. Start109 em gameplay abriu menu Pause, confirmado
em estado e captura com barra PAUSE/opções no rodapé. Select108 a partir de HUD
abriu o mapa. A transição pode exibir fundo de mapa antes do menu completo;
uma imagem isolada imediata122 não separava tudo. Circle em alguns casos foi
interceptado (0x10000), e sair de option/Pause exigiu novo ciclo; regressão de
navegação ainda pendente. Não usar auto-repetição em telas com opções de áudio.
O mesmo perfil114 continuou com~15FPS no telhado; nova travessia foi diferente,
passou pela parede/terraço e piscina. Missão/save continuam pendentes. Registro
122 estável final é tasm_122_final.log (o primeiro pull sofreu EEXIST no host).

Investigação de mips124, somente leitura: inspect_mip_policy.py agora usa os
PT_LOAD para VA->arquivo e valida SHA do ELF. Primeira execução encontrou dado
em BSS e recusou acesso; tratamento explícito de BSS registrou inicialzero,
sem confundir com estado runtime. getMipmapCountToSkip usa Texture_LowLodMipmapSkip
para nomes Map__, Atlas para CityAtlas, Lightmap para lightmap. Em vários casos,
valor configurado nãozero é aumentado em1 antes de retornar. GOT0xc3c544 aponta
para enableLowMemoryPhone0xc51a64 (símbolo exportado), inicialBSS0; se ativado,
o caminho força skip4. O port não chama nativeEnableGroupLowMemoryPhone. Isso
nÃO prova o valor runtime, não autoriza patchar o global nem culpar o flag.
Arquivo native-mip-policy-values.json preserva strings/endereços/limitações.

Plano124: mesmo VPK123, só LowLodMipmapSkip1->0 no XML114. Atlas0 não controla
texturas Map__; skip1 pode significar descarte de2 níveis. O lowphone original
também tem LowLodMipmapSkip0. Testar definição distante e memória no mesmo
telhado, sem mudar Sea/BuildingSkip/distâncias ao mesmo tempo. O experimento
fica reversível por config114, app fechado ao substituir.

124 usa VPK123 e XML `config_lowlod0_124.xml`, SHA
`08a1ee77577e4be4d779b45bb471dff91ce7b9127f8fec34fdfd9522a3e1501c`.
Captura2 mostrou anúncios/fachadas/faixas da rua muito mais definidos; é efeito
visual forte da política Map__, sem mudar distâncias/Atlas/BuildingSkip. O
Cross enviado como skip pegou o final da intro e saltou; imagem é pose de salto,
não comparação pixel a pixel do telhado parado. Vista~15,2–15,4FPS com~125draws/f,
comparável à pose antiga de salto; não afirmar ganho de FPS. Memória manual:
newlib51016KiB, freeBlk15031KiB, VRAM10697, RAM1184, PHY378KiB livres. Memória
aumentou; nenhuma falha de alocação/link apareceu no trecho. Sem prova de
estabilidade em sessão longa/cidade. Telemetria pausada após medição.
Wait pelo hito1080 falhou porque HUD já estava em1780; usar condição de frame
superior ao estado recente e captura fresca, não hito numérico fixo de outro boot.

124 travessia confirmou cidade mais definida até a piscina. Cross+forward2400ms
e depois forward800ms chegaram à piscina; Circle350ms Web-Strike, outro Circle
mais6Cross (250ms pressionado/500ms solto) entraram em combate com10hits. Mais
8Cross avançaram ao tutorial de esquiva: a imagem instruiu Triangle. Triangle
foi confirmado como esquiva/contexto de combate, não o web-shoot descrito antes.
Houve mais combos reais e novos inimigos, mas a saúde caiu. Sequência posterior
foi ao menu miss_failed (log) e Cross reiniciou no telhado. Isso é falha da
tentativa de gameplay, não crash nem missão concluída. Start abriu Pause.
Memória depois do retry: newlib53011KiB, freeBlk8200KiB, VRAM10082, RAM0,PHY0
livres. Nenhum malloc/link/overrun no trecho. Crescimento~2MiB não prova leak
nem aprova memória de cidade longa. Log parcial tasm_124_memory_after_retry.log.

REA buffers124: GL_OES_mapbuffer já é anunciado e import glMapBufferOES aponta
à função real; não é extensão simplesmente ausente. Os dois exports overloads
CStreamedBatchMesh::construct(0x30a1b8/0x30a3d4) são apenas bx lr neste ELF;
não inferir implementação do nome exportado. Evidências
ev_b4b11bdb32a12825baaa5ac3958fbff1c72dc64e736b387802efac880e405eb2 e
ev_64d5e2912ff6b269c28b1e21a6156616bdbc65450cd785c2c2b65d412c27cf5b.
Tabela alvo0xb32bd8 começa8892/8893; tabela usage0xb32bf0 começa88e4/88e8/88e0.
Vertex bind0x494f20 trata usage4 em branch próprio. native-buffer-enum-tables.json
e dumps preservados; comprimentos/enum completo e dirty paths ainda desconhecidos.
Não houve patch de política/VBO nativo, nem zero-copy.

125 em preparação: reduzir acessos de tabela na compactação esparsa juntando
tag16 e remap16 em um entry32. Scratch262148bytes contra o scratch original
com seen/remap/originals~512KiB. Na volta de geração65535, zerar a própria tabela;
não reutilizar fontes nem snapshots entre draws. Mesmos critérios de admissão,
ordem de vértices, índices próprios e cópia NEON de registros; duas etapasOFF.
TASM_COMPACT_TAG_TABLE experimental/defaultOFF, flag125ON para medir.

Teste host novo inicialmente falhou: o teste forçava generation0 sobre tabela
com tags1 antigas, simulando reset sem zerar a memória. Corrigido para zerar o
scratch no caso de inicialização, mantendo testes reais de wrap65535/UINT32_MAX
e mudança de fonte em draw seguinte. Recompilado com GCC -O2/Wall/Wextra/Werror,
exit0: fetches exatos, fontes imutáveis, guardas, strides12..64 e todos65536
vértices passaram. Build125 compilando. Ainda não aprovado/instalado; medir
compact_ms/FPS e executar testes ARM/FBO antes de manter o caminho novo.

125 instalada `12f25e4d39e3993ac8ffee0f2f902199077cb48d7adb42cfca6d4d98beaab855`,
archive build-125-compact-tag/. Mantém XML124, pool36 e demais flags123, acresce
COMPACT_TAG_TABLE1. FBO boot4casos passou novamente: diff_bytes0, drawn1024,
variation1023, GLerror0, binding1, result0. compact_selftest também0 no Vita,
incluindo offsets0..15 e rollover de tags. Marcador boot removido. Pull inicial
reportou EEXIST, mas o arquivo local resultante contém as linhas da execução;
coleta estável final ainda será feita. Banner __DATE__/TIME ficou o mesmo123
por relink de biblioteca; identificar por hash e log125, não horário isolado.
Captura2 confirma pose de salto e a definição Map__ restaurada124. Telemetria
pausada; medição estável desta cena ainda pendente neste registro.

125 medição inicial em pose de salto/XML124:15,56–15,77FPS, compact~0,90s/5s,
bounds~0,255s e cópia~1,27s. Comparação124~15,2–15,4 é ganho pequeno, sem afirmar
causa para cada diferença. O tag reduz scratch e passou equivalência. Ao retomar
depois de nova mensagem, app tinha~5016s/90193frames, ainda vivo (~83min desde
boot), final~20,8FPS em outra vista com~93draws/frame. Log inclui input adicional
após o período de medição; não atribuir a mesma cena nem tratar como teste
controlado de83min. Nenhum LINKFAILED/mallocfail/overrun encontrado no log125
completo. App fechado e evidência estável tasm_125_tag_final.log.

126 somente probe de metadados da factory nativa0x49c8dc, ativado por
buffer_probe.enable. REA revelou construção de objeto64bytes via base4b09a4 e
config18bytes: type+0,usage+4,size+8,data+12,flags+16,ring+17. Snapshot limitado
aos3 primeiros por type/usage, usando atomic counter; lê depois da função
original e registra caller/objeto/config. Hook valida export/base e prologue
e92d41f0/e1a05000, copia só8bytes sem PC-relative para trampoline ARM. Não
altera enum/flags nem buffers; necessário para relacionar a política CPU4 aos
callers reais. As disassemblies/scanvtable estão em native-buffer-*-126.txt e
native-buffer-vtable-refs.json. Não confiar nos nomes STL aproximados de objdump.

126 instalada `93faffa302c74292d330b635e99f36308e38528c8d72eccaf2a0c2ccc07c5112`,
archive build-126-native-buffer-probe/. Hook passou opcodes/base e foi instalado;
boot/menu e telhado foram renderizados. Probe revelou type0/1 e usages0/1/4,
mas limite3por(type,usage) foi esgotado nos menus, ocultando callers posteriores.
Não concluir que esses poucos callers geram todo custo da cidade. 127 corrige
só amostragem:128chaves(caller,type,usage),2linhas/chave, no máximo256linhas.
Tabela protegida por spinlock curto, sem logging/alocação dentro da região.

Input126: MainMenu vivo e raw0x10000 persistente; Cross retornava sucesso mas
não chegava ao app. Wake e um toque brevePS(80ms, sem menu rápido/volume)
deixaram state parado durante suspensão. `vita_launch_app(close_current=false,
force=true)` retomou a mesma execução (frame cresceu, sem novo log) e HUD;
input voltou a chegar. Não foi necessário reboot nem mexer em plugins. O
intercept voltou num estado posterior: não tratar estado raw como prova de
bug da sensibilidade. `tasm_126_factory_game.log` preserva probe e telhado.

127 instalada SHA `86e69d1cd1160932742b8caec3509696adac471b763478d121fef06b712c680b`,
archive build-127-buffer-callers/. Mostrou maiscallers e buffers CPU4; sem novo
comportamento de render. Injeção novamente falhou, desta vez raw0/128 apesar
de presshold e retomada sem fechar. App foi fechado, log estável
tasm_127_callers_input_unavailable.log, reboot normal viaMCP17s. Boot128 é o
mesmo VPK, sem nova instalação. EsperaMainMenu inicialmente leu state antigo;
aguardar log128 fresco+stateframe273; hardware retornou raw141,136/right100,125.
Crosshold confirmou0x4000 e soltura progrediuHUD. Sem mudanças de plugins/volume.

128 probe encontrou World::LoadWorldScene caller0x2f2ba0 criando buffertype0,
usage4,size26214400 (=25MiB, não26MiB), sourcePHY0x95e00250, flags1/ring1.
O constructor51db6c mantém source emobject+12 e campos dirty/lock; bind494f20
escolhe CPUbase parausage4, VBO parausages0/1. Lock51ebd4 e unlock51ecd8
registram faixa/dirty; modo gráfico usa upload49f540 e update49fcdc/49a824.
Esses caminhos existem no ELF e serão usados pelo experimento; ainda não
provam cobertura de todas as escritas diretas da cidade. Não usar zero-copy.

129 em preparação: opt-in native_city_vbo.enable muda SÓ a cópia local do
config desse caller/type0/usage4/size25MiB/source!=NULL/flags1/ring1 parausage1.
Usa o VBO nativo e shadowCPU já mantido pelo motor, com vida útil da vitaGL.
Arquivo ELF e source do jogo permanecem intactos; não muda todos os buffers,
não pula cópias de lifetime sem dono, não reduz distâncias. Acrescenta contador
glBufferSubData separado para observar updates nativos. Reverter removendo
marcador e reabrindo se houver OOM/imagem errada/regressão. Comparar telhado e
travessia, memória e falhas. Cobertura de writes e save continuam pendentes.

129 instalada SHA E58E1CDB5BFD19ABDD0399110740724D10001F200A264F84F2E77471FE998FDD,
archive build-129-native-city-vbo. Log confirmou a seleção exata usage4->1 e
upload inicial25MiB. A captura fresca2 mostrou regressão grande: prédios próximos
perderam a geometria detalhada e ficaram em blocos/coarseLOD. FPS maior nessa
execução não é ganho válido porque faltou imagem. Experimento REJEITADO; app
fechado, native_city_vbo.enable e buffer_probe.enable removidos. Log estável
tasm_129_city_vbo_rejected.log preservado. XML124 continua sem alteração.
Capturas temporárias local/remota removidas ao retomar a investigação.
Os contadores SubData gerais (~1/frame) não identificam quais buffers mudaram.
Não concluir só deles que a cidade recebeu seus updates. Causa ainda aberta.

130 em preparação: diagnóstico de comparação shadowCPU/VBO. Novo comando dev
city_shadow compara25MiB em páginas4096 e registra apenas número de páginas
diferentes/primeiro offset; sem dump de vértices. O helper vitaGL lê o storage
sem map/unmap: nesta implementação unmap muda last_frame para OBJ_NOT_USED e
contaminaria a análise de lifetime. Tracking somente do upload dinâmico25MiB
com marcador experimental ativo, invalidado em realloc/delete do nome GL.
Conta SubData especificamente desse buffer e primeiros8ranges. Comparação
pode observar escrita concorrente do motor; repetir para distinguir transiente.
Não muda política nova nem tenta reparar uploads; o teste opt-in será fechado
ao terminar e retirará o marcador novamente. Build/teste ainda pendentes.

130 instalada SHA B62127C120890CABE369E877899662C6DD7861FD7F31F929106E842E7F647008,
archive build-130-city-shadow-probe. Captura não necessária para a comparação.
Upload inicial: different_pages0/6400, bytes iguais, comparação517324us. Depois
de carregar HUD:6400/6400 diferentes, primeiro offset8, updates0 bytes0. Segunda
comparação posterior repetiu6400/6400, offset8, updates0. Tempo curto~2,5ms do
caso divergente deve-se ao early-out por página, não mede bandwidth de igualdade.
Isso confirma shadow alterado após upload e ausência de SubData desse buffer;
não prova ainda onde todas as escritas ocorreram. Log tasm_130_shadow_final.log,
app fechado e marcador removido. Último comando enviado logo antes do close
pode não ter sido consumido; conclusões usam as linhas efetivamente presentes.

131 em preparação: city_sync_once, comando diagnóstico explícito, espera
glFinish antes de map/escrever storage próprio, copia25MiB e unmap/restaura
binding. Sem cópia por frame, sem aliasing da origem. Hipótese: atualizar uma
vez após carga deve restaurar a geometria inicialmente ausente; não resolverá
escritas de streaming posteriores. Excluir sincronização/capturas de FPS.
O teste não certifica cache/VBO para uso final; lifetime depende da esperaGPU.

131 instalada SHA E391D496290ADBE90699E07B14CF8E87EB9A321002EBAABAEA6EA0CD33353C2E,
archive build-131-city-sync-once. Duas sincronizações executadas, mapped1/unmapped1,
143671/133029us. Depois da primeira houve4462/6400páginas divergentes, primeiro
offset0, updates0. A segunda foi seguida de comparação0/6400, bytes iguais,
536823us. Mesmo assim a captura de gameplay permaneceu incorreta, com blocos
escuros/coarseLOD em volta do personagem. Uma atualização isolada NÃO corrigiu
o caminhoVBO. A divergência está confirmada, mas não é explicação completa da
imagem errada; investigar layout/offsets dos atributos e integração nativa antes
de tentar VBO de novo. Não adotar fullsync por frame nem sincronização semGPUidle.
Captura1 era cinemática com íconeXskip apesar de menuHUD; capture2 era gameplay
em queda. HUD não prova que a cena já é jogável. Ambos comandos constam no log
tasm_131_sync_final.log; app fechado, marcador/imagens temporárias removidos.

REA131: primeira seleção de addSegment usou mangled substituiçãoS9 errada e
retornou exit0 com stdout204chars sem corpo. Não tratar como função vazia.
Resolvido com símbolo exato da tabela0030adbc, substituiçãoSA. Evidence
ev_00197ab45c891ec67175501487ce73d172e035357c4c669140ea0c5a5ea0634b,
stdout50545chars, SHAinalterado. Disassembly de addSegment/lockBatch/functor e
tail do upload nativo preservadas. Sem novo patch de streaming derivado disso.

132 é boot do binário131 com marcadorVBOausente. Capture1 preta ocorreu na
transição após skip; capture2 fresca confirmou prédios detalhados/fachadas no
salto, capture3 confirmou chegada à piscina. Água ainda azul sólida. Desempenho
em travessia~11,4FPS numa janela, poolpeak9473KiB; não usar como comparação de
telhado. Input na piscina: parcialMCP200,128 -> raw172,125/down1/dx4/dy0;
soltar -> raw100,126/down0/dx0/dy0 em frame2252. MáximoMCP255,128 -> raw227,125,
down1/dx14/dy0 frame2668. Log estável tasm_132_cpu_restored.log. App fechado.
Waitframe1150deu timeout45s comframe1032, pois o caminhoCPU era mais lento;
próximas esperas devem partir do frame recente e usar progresso/contexto real.

133 modifica só defaults da câmera: horizontal0.65, vertical0.75 (antes1/1),
mantendo curva por tempo, compensação de ganho nativo, calibração, deadzone e
cfgopcional0.25..2. README atualizado. Tentativa de pull do cfgantes132 retornou
550 (arquivo ausente); portanto o teste usou defaults e não sobrescreveu cfg do
usuário. Build SHA5E84896D2F2C3788FD69D0443C39F26D183938F9CE3E72A6C37EBE55F7EF2A1D,
archive build-133-camera-defaults. Instalada sem marcadores; regressão prática
de parcial/pleno/reversão/vertical/soltura/pausa em andamento. A redução dos
defaults não significa que a sensibilidade ideal foi aprovada pelo usuário.

133 log confirmou horizontal65%/vertical75%. GCCcamera_input_test -O2/Wall/
Wextra/Werror passou. Em gameplay: parcialMCP200 -> raw171,125/down1/dx4/dy0;
pleno255 -> raw227,126/down1/dx10/dy0 (antes14). Reversão diretaMCP0 -> raw0,126,
dx-6/dy0; verticalup -> raw100,0 dx0/dy-6. Soltar frame2999 retornou down0,
deltas0. Verticaldown+leftstickdireita: raw215,134/100,252, ambosactive1,
dx0/dy6. Start com ambospressionados abriuPause, ambosactive0/down0. Soltar e
Circle retornouHUD frame3446 com neutro/down0/deltas0. Log estável
tasm_133_camera_validation.log, app fechado/captura temporária removida.
São verificações dos gestos/deltas, não medida angular nem aprovação subjetiva.

134 reprodução negativa do offsetVBO grande: FBO quad colorido32x32, referência
drawn1024/variation1023. Casos originais0..3 passaram exatos. Caso4 armazena o
mesmo quad num VBO com prefixo0x2000c (>64KiB):diff_bytes4064/error0/result-14.
Shader/dados/offsetsGL válidos, mas vitaGL gravava offsetsGL32 num campoGXM16.
SHA0AC53A2C8F028BB17E60113AB3EA4B5C05169A6B5400311042988B2859D0E1B8,
archive build-134-vbo-offset-negative; tasm_134_vbo_offset_negative.log.
vita_waittail não encontrou resultado porque saiu no início do log e ficou
fora do tail; pull completo após fechar confirmou. Não repetir wait pelo hito
antigo. Marcador indexed_client_selftest continua só para a correção135.

135 em preparação: rebase do prefixo comum de atributos VBO para o ponteiro
do stream antes de converter o offset para16bits; manter stateGL e lifetime.
Aplicar DrawElements/DrawArrays/MultiDrawArrays. Se a separação entre atributos
não cabe16bits (planar distante), usar streams individuais já existentes.
FBO estendido cobre offset>64K, first3, instancing, planar com stride igual,
atributos em ordem invertida e multidraw. Primeira compilação falhou porque
helper precedia typedefprogram; corrigido passando attr_num/attr_map ao helper.
Ainda aguarda build e prova no Vita. Não reativar VBO nativo até passar o teste.

135 instalada SHA E62FE6FE57B35BA359207E63571D95C825A73D17DB0583CF8AACA1D28C80A1D1,
archive build-135-vbo-offset-rebase. TODOS10casos passaram:drawn1024/variation1023,
diff_bytes0,error0,EBO_preserved1,result0. Inclui prefixo0x2000c, DrawArraysfirst3,
instância1, planarVBOigualstride20 separado0x40000, posição/cor invertidas e
MultiDrawArraysfirst3. Evidência tasm_135_offset_final.log. Correção de endereçamento
confirmada; não é prova isolada de streaming/performance. MarcadorFBOremovido.

136 mesmo binário135, VBOcidade opt-in. Upload inicialigual358287us; três
syncsGPUidle144554/127718/122679us. Capture1preta em transição; capture2cinemática
comXskip; capture3 mostrou telhado com prédios detalhados restaurados após rebase
e sync. Geometry_trace_136 revelou atributos reais doVBO9493b610 em offsets
0x1033408/0x1033424/0x1033414/0x103341c e0x12ee808, confirmando que a cidade usa
prefixos de muitosMiB. O erro16bit era pertinente ao cenário, não só sintético.
Sem sync automático:3613páginas divergentes antes de terceiro sync; depois de
travessia384páginas divergentes, primeirobyte1032, updates0. Capture4 em queda
mostrou detalhe, mas isso não prova que todas as partes novas estavam corretas.
Últimas janelas~17,9–19,2FPS em vistas variadas; não atribuir ganho controlado de
telhado. Janela8,35incluiu captura/diagnóstico. Log estável
tasm_136_vbo_offset_synced.log; fechado, marcador/imagens removidos.
Pull CSV reportouEEXIST mas arquivo local válido foi inspecionado.

REA136 localizou memcpy@plt emCVertexDataAllocator::alloc (caller0x30fdb4),
destino derivado de seu buffer+12 e offsets de blocos. O tailaddSegment chama
51e3a4, mas a disassembly indica resize(size,data,ownership), NÃO notificação de
dirty. Dumps native-city-allocator-136 e native-buffer-notify-136 preservados.
Não hookar uma função com ABI presumida pelo endereço/chamada.

137 em preparação: probe de escritas por memcpy/memmove/memset normais e
aeabi_memcpy[4/8], preservando as funções de cópia originais. Somente no intervalo
CPUshadow25MiB doVBOexperimental, marca páginas4KiB em bitmap800bytes atômico;
workers não chamamGL, não alocam nem imprimem por cópia. city_shadow cruza bitmap
de divergências com escritas observadas e informa uncovered. sync_once inicia
novo período de observação antes da cópia, preservando eventos concorrentes.
É cobertura por página, não prova de que cada byte/write futuro está coberto;
aeabimemset/memmove e escritasinline ainda podem não passar pelos wrappers.
Nenhuma sincronização automática será ativada só com esse contador. Build em
andamento, ainda sem resultado/instalação137. Próximo passo: medir cobertura em
carga/travessia e decidir o hook correto com base nos offsets/callers reais.

137/138 correção operacional: primeiro build do probe finalizou21EAF5A09C6426E1
C78107AF1DC4FA9DADDF3B950309F903A651A9B010A0DEF7. Foram acrescentados load/store
atômicos da origem compartilhada e reset de período no sync. Segunda compilação
ainda retornava session15807 quando a instalação foi disparada indevidamente;
o boot137 foi abortado/fechado, marcador removido, log preservado em
tasm_137_aborted_build_race.log. Nenhuma conclusão de regressão desse boot.
Não instalar se write_stdin ainda retorna session_id: esperar exit0 é obrigatório.
Segunda compilação terminouexit0, VPKA19475B6E2953B948D42D9F6C77B89A95D4C98C60F
751F578703D16ABCA5B928. Arquivado build-138-city-write-epochs e só então reinstalado
para boot138. Comparar SHA do pacote/log efetivo, não assumir que o arquivo já
representa o último source enquanto o build está em andamento.138 em teste.

138 resultado: comparação inicial0/6400, depois6400/6400 e267writes/6400páginas
marcadas/uncovered0; mais tarde34557writes ainda6400divergentes antes do sync.
Sync_once122501us iniciou novo período; logo após0páginas/0writes/0uncovered.
Travessia curta também não mudou dados. HoldCross+forward6500ms alcançou outro
telhado (captura2):1023páginas divergentes,1520writes,430páginas marcadas e
593divergentes sem notificação. Portanto wrappers atuais são INSUFICIENTES para
sincronização automática. Sem atribuir a escrita restante a uma rotina sem prova.
Log estável tasm_138_write_coverage_rejected.log. App fechado, marcador/capturas
removidos. Waitframe500timeout20s terminou490 durantecarga: não foi crash.

139 amplia probe às rotinasARM __aeabi_memset[4/8], memclr[4], memmove[4/8],
chamando helpers originais antes da notificação; ordemAEABImemset é(dest,size,c),
distinta daC(dest,c,size). Sem substituição do algoritmo de cópia. Próximos
suspeitos só se a cobertura continuar insuficiente: inflate/uncompress, leitura
direta e stores inline; nenhum já marcado como causa. Acrescenta request_id e
command_count ao state do harness; comandos agora podem ser enviados com id e
aguardados por request_id==N antes do próximo. Isso confirma consumo e evita
sobrescrever cmd.json por usar frame/meta antigos; não é confirmação de sucesso
de comando desconhecido. IDs são locais ao boot (inicial0).
Buildexit0 SHA AADB9BC9506057B0C6D8AD947AE62AFE58A1A140DA7CDFEF114848DAD7A7141F,
archive build-139-eabi-write-ack, instalado depois do término.139 em teste;
VBOexperimental não será mantido habilitado como candidata sem cobertura/correção.

139 execução: request_ids1..8 confirmados em state, command_count correspondente;
city_shadow1 frame1516, sync2 frame1533, capture3 frame1556 (aindaLOADING),
shadow4 frame2083. Esse período teve3800páginas diferentes/3797marcadas,
19940writes/uncovered3. Capture5 frame3621 mostrou personagem abaixo da cidade,
imagem inválida no caminhoexperimental sem sincronizar durante o carregamento.
Shadow6 repetiu mesma divergência. Sync7 frame4126 (127123us) e travessia6500ms:
shadow8 frame4274 teve884divergentes/645marcadas/2420writes/uncovered275.
Portanto AEABI também não cobre tudo. Não usar só o caso3páginas como conclusão
de travessia longa. Log estável tasm_139_eabi_final.log; app fechado, marcador e
capturas removidos. A imagem ruim não conta como candidato nem ganho deFPS.

140 em preparação/teste: preservar semântica de I/O e notificar saídas de
fread_soloader nos caminhosarchive/cache/libc, readreal e inflate/uncompress
importados, pois cópias feitas DENTRO do port não passam pela tabela de imports
do ELF. Marca bytes reportados pelas funções; freadsize>1 com itemparcial pode
deixar bytes sem notificação e permanece limite a testar. Bitmap distingue
contadorescopy/eabi/io/zlib. Nenhum GL emworker, nenhuma mudança de prioridade
ou algoritmo de leitura/descompressão. Só a comparação dirá se cobriu as páginas.
Buildexit0 SHAD6F71F96A26D153EE6021988CE74CA99C745665553BAA9553059C10ECB807482,
archive build-140-io-write-probe. Instalado após exit0, boot140 em andamento.

140 resultado: início203páginas divergentes,217marcadas/779copy/uncovered0.
Depois do carregamento3725divergentes/3725marcadas/18370copy/uncovered0. Contadores
eabi/io/zlib0 nesse trecho; não atribuir melhora de cobertura aos novos wrappers.
Memóriadevmem: newlib73558KiB/freeBlk12293, VRAM7938/RAM1386/PHY1023livres.
Capture1 mostrou cenário incorreto (VBOsemupdates, coarseLOD). Sync7idexecutado
128258us; travessia6500ms ->751divergentes/511marcadas/1902copy/uncovered240,
demaiskinds0. Resultado insuficiente continua. Log estável
tasm_140_io_coverage_rejected.log, app fechado, marcador/captura removidos.

REA140: glf::Memcpy export6429b4 é somente branchpara memcpy@plt1aa1ac;
portanto JÁ passa pelo wrapper e NÃO é cópia SIMD interna nova. Memset16/32
(6429c4/642a6c) têm loopspróprios, mas não há evidência de uso no buffercidade.
Não instalar hook só por existir função. SInPlaceMetaDataPolicy pode escrever
cabeçalhos inline; primeiro byte4 após travessia é compatível, não prova.

141 em preparação: segundo passo diagnóstico lê só páginas semnotificação e
conta bytes divergentes/primeiros8offsets. Sem registrar bytes dos assets,
sem alterar mapped/last_frame/bindings/GPU. Objetivo: separar diferenças de
cabeçalhos do alocador de dados de vértices. Nenhuma sincronização automática.
Acrescentou também memclr8 que faltava no mapa do probe; ainda não se observou
chamadaEABI nesses períodos. Build em andamento; hash/instalação pendentes.

141 instalada apósexit0 SHA480549527A1344D89EDE33FB1C26835F3B8F0D56472D264899B72919DECCD5D0,
archive build-141-unobserved-bytes. Primeiro período3735páginas divergentes,
3728marcadas,uncovered10 (variação concorrente explica não ser simples subtração).
Segundo passo encontrou219bytes nos10pages, primeiros offsets4b7400,4b7401,
4b7403,4b7404,4b7408..b. São poucas escritas inline, compatíveis com cabeçalhos;
não dizer que todas219foram classificadas individualmente. Após segundo sync,
2078divergentes/2078marcadas,uncovered0; período final semnovoswrites teve0diff.
Sem travessia/carga nova no período final não se prova cobertura geral.
Log estável tasm_141_unobserved_final.log. App fechado/marcadorremovido.

REA141 freeCVertexDataAllocator: evidence
ev_cea87108f5b0243d5d6dcced79753abafab7e7235b86c832e3fa46240da2ff0f.
Funções5aeca8 e5aed34 do policy usam base=self[0], chunk_size=self[1], índice
chunkemr1. Escrevem diretamente next/prev/size em headers0/4/8 dentro do bloco;
free tem argumento5hint na pilha e retorna próximo hint. O prefixo5aeca8 é
e92d07f0/e5904004;5aed34 e92d0ff0/e5907004, semPC-relative nos8bytes. Não hookar
glfMemcpy, já encaminhado ao import. Dumps native-allocator-*-141/142 preservados.

142 só probe: hooks desses dois policy methods chamam primeiro o original,
preservam argumentos (incluihint5) e retornor0, depois notificam metadados.
Só admite policybase==shadow25MiBativo, chunksizeválido e índice que cabe todo
no buffer. Marca conservadoramente o bloco inteiro; isso NÃO significa que todos
os bytes mudaram. SemGL/alocação/log por worker; não altera listas nem cópias.
Instalaçãoopt-in juntoaoVBOcity, checa2prologues antes de instalar qualquerhook.
Contador native/chunk_size mede custo/cobertura antes de sincronizarGPU.
Buildexit0 SHAFC0CE9EAC0E50432B062FD5AD02997D1D60B6189D516C2C931E957015087F58D,
archive build-142-native-arena-probe inclui source-current.diff dos arquivos
envolvidos (não inclui novos arquivos ignorados nem é snapshot do histórico066).
Instalada, boot142em andamento. Nenhuma sincronização automática aprovada.

142 resultado: hooks confirmados em985aeca8/985aed34; chunk_size REAL1024bytes.
Primeiro período874divergentes/963marcadas,3774copy+3774native,uncovered0.
Segundo período após sync130281us e travessia6500ms:1052divergentes/1065marcadas,
3373copy+5890native,uncovered0.13páginas extras são coerentes com marcação
conservadora. Esses cenários cobriram os headers inline que faltavam nos probes
anteriores; continuam sendo testes limitados. Nenhuma falhaLINKFAILED/alloc no
trecho. Log estável tasm_142_native_arena_final.log, app fechado/marcadorremovido.

143 experimento: segundo marcador native_city_sync.enable, além doVBOcity,
ativa atualização de páginas4KiB marcadas, no máximo uma vez por frame antes do
primeiro draw. Só habilita após confirmar instalação dosdoishooks nativos.
Mainthread retira bitmap atomicamente; workers publicam dirty depois da escrita,
semGL. glFinish garante que leiturasGPUanteriores terminaram antes de map/escrever
storagepróprio. Copia runs contíguos das páginas marcadas, unmap/restaura binding.
Não usa zero-copy nem sourcealias, não renomeia25MiBporSubData a cada frame.
Falha map/unmap mantémdirtypara retry e emite erro; agente deve parar/reverter
se aparecer. Primeira ativação faz sync completo para cobrir inicialização.
perf-city-sync separa bytes/tempo/esperaGPU; fullinit e cargas fora da comparação
deFPS. Publicação da origem ocorre após zerar contadores para não apagar eventos.
Buildexit0 SHA43DC3ED9C6A7AA7E061F8EE9DC91BEFE2A700ACBFAB537C3A58AE4DD74DE2804,
archive build-143-auto-city-sync; instalado com dois marcadores para teste.
Ainda não aprovado: conferir imagem, divergências pendentes, custoGPU, memória,
travessias e falhas. Default e candidata final não foram alterados paraVBOON.

143 parcial: log confirmou automatic1. Primeiro sync25MiB110945us; durantecarga
houve29updates192KiB/67999us,13updates1092KiB/53277us e umajanela44updates6116KiB/
704605us, max28853us. Não omitir esse custo de carga usando só a janela estável.
city_shadowid3 durantecarga tinha151páginas diferentes e168pendentes,uncovered0:
worker pode terminar depois do primeiro draw; testar convergência quando para.
Capture1 aindaLOADING; main_actor main>0ms em log foi só heurística e não provou
fim de carga. Captura2 fresca mostrou personagem/telhado e cidade detalhada
corretos, sem sync_once manual. Id5 depoisestabilizado:0/6400diferenças,
0páginaspendentes. Janelas24,1–25,5FPS (~126draws/f); vista próxima da inicial
anterior15–16, mas não comparação controlada pixelapixel nem ganho do jogo todo.

143 travessia Cross+forward2400ms,release,forward1000ms: captura3mostrou swing
e edifíciosdetalhados. Id7 terminou0diff/0pendentes. Últimas janelas estáveis
26,6FPS/121draws, depois movimento24,7FPS, não atribuir cada diferença àmesmacena.
Atualizações nessa passagem4calls644KiB/7732us(max3264),9calls1724KiB/19823us
(max6475), errors0. Janela8,44FPS incluiu captura e não é benchmark de gameplay.
Id8 memória: newlib73887KiB/freeBlk8192, VRAM10696/RAM2984/PHY1023livres.
Inclui ~25MiBextra doVBO; não declarar memória longa aprovada.

143 Circle400ms e captura4: personagem em pé na borda/jardineira do telhado da
piscina, geometria coerente, sem chegar a combate. Medições em outra orientação
~20,2–20,8FPS (93–94draws/f), sync0porque semwritesnovos. Persiste gargalo fora
da cópiaestática. Start abriuPause frame5407 para análise sem deixar personagem
exposto; app aindaABERTO/PAUSADO e dois marcadores ativos somente para teste.
Logs parciais tasm_143_auto_initial/pose/settled/traverse/after_traverse.log;
coletaestável ao encerrar ainda pendente. Captura temporária ainda eminspeção.
Próximo: combate/novas áreas, medir lags/update/GPU, compararcoerência e memória;
retirar marcadores ao encerrar ou se houver regressão. Ainda não jogo100%.

143 encerramento: câmera direita1300ms mudou a orientação de forma limitada e
parou ao soltar. TentativasCircle,8Cross,Triangle e movimento mostraram o herói
na borda/parede do átrio, com inimigos no deck; NÃO houve combo nem combate
confirmado. Não interpretar8Cross como8ataques executados. Up3000ms mostrou
movimento na parede, mas imagem sozinha não determina delta de altura. Pausa
entre inspeções. FPS nessa posição tinha WorldUpdate~2,9s/5s contraWorldRender
~1,7s/5s; física~0,74s/5s (tempos inclusivos). Próximo gargalo éupdate, não somente
cópias. Capturas6..9 confirmaram cenário coerente, água da piscina ainda plana.
Snapshotid15 emPause/frame52633 terminou0diff/0pendentes, sem falhas de map/link/
alloc/overrun encontradas no log completo. Id16heap73894KiB/freeBlk8185, VRAM10111/
RAM949/PHY1023livres; +7KiBdesdeamostraanterior não prova estabilidade longa.
Maioria dosframesextras foiPause~60FPS: não contar como teste longo emgameplay.
App fechado; ambos marcadores removidos e BMPs temporários apagados. Logfinal
tasm_143_auto_city_final.log (pull EEXIST, arquivo existente válido contém última
amostra). Nenhuma conclusão de missão/save nem release100%.

144 diagnóstico para navegação eCPU: CGameObject::GetBodyPosition37abfc retorna
vec3 via sret(r0dest,r1self) e lêposição base+16/+20/+24, somando altura+0x180emY.
Não confundir structreturn com ponteiro. Snapshot somente deXYZbase/slotcombate
0x338 na ENTRADA deCMainCharacter::Update, checando vtable0x98c13e28; devplayer
imprime cópia/idade emframes, nunca lê personagem antigo depois de outroload.
GetCurCombatState3ccc20 indexa vetor+0x350porindex+0x338 semguard; não chamá-lo
arbitrariamente fora do contexto. Dumps native-player-position/combat-144.

144 state_profile: janela alvo5s, sóthreadprincipal,128chaves(vtable,definition,
caller), top16por tempo inclusivo. REAStateAutomat::Update2b4590 usa current+24,
pending+28; definition=current[1], nomeemdefinition+12 no branch deconstructor
std::string. Probe só segue current quando pending0 e copia nomes ASCII apenas
se constchar*está dentro de segmentoSOmapeado. Não dereferencia nomesheap nem
usa nomes como instruções. Campo state_profile_active emstate permite esperar
fim; reporta sampled_span real e dropped, não porcentagem deCPU. Excluir esse
intervalo de benchmark. Jogo/posições/listas/IA não são alterados pelo probe.
Primeiro buildF849A59D32B4260949038D17A1D470DEFA6758738FC1532790A36F30411E74AB não
foi instalado; acrescentadosflagatômico e spanreal, segundo buildexit0
SHA161081C9BE5A6ECCE3D7E235C07CA26A0587E825461B255184304CE46CF0FDE4.
Archive build-144-player-state-profile inclui novoheader/patch.c e diffcorrente.
Instalado comVBO+autosync143opt-in para comparar mesmo caminho.144emteste.

144 resultado: crash antes do primeiro swap/menu; log parou em478linhas. Dump
psp2core-1793818259-0x0001372065-eboot.bin.psp2dmp preservado em _logs/crash-144,
bundle original C:/Users/leand/vita-mcp/crashes/2026-11-04T18-50-59-000Z.
PC9847ca18 em std::map<string,intrusive_ptr<collada::CResFile>>::find47c9e4,
R3=1 usado como ponteiro de chave, DFARfffffff5. LR67 era scratch, NÃO caller.
Decodificação do prologue e pilha recuperou retorno98470418 e próximo9848eb18.
O nó8df63f00 não está nos PT_LOAD do core; conteúdo não recuperável desse dump.
Não afirmar mesma causa do crash080 nem corrigir convertendo ponteiro1 emnull.
O core bruto é gzip1583856B; ELF descomprimido15460712B. Script readonly
inspect_crash144.py corrigiu a suposição inicial de ELF semgzip. SHA bruto
5e33f92c098ab8c1ae47e4551c8b3b8ef6c991602f5b182b477951c065c22d94,
decodificado ddc8b2825eecf5fa56d3dfd54543921fee8bae24b940d0b4f3cbe4325b144c41.
Logs native-crash-144/native-resource-lookup-144 registram disassembly; nomes
coarse do addr2line semDWARF não identificam funções ocultas com segurança.

Controle145: restaurado binário143, ambos marcadoresOFF, chegou MainMenu.
Controle146: mesmo143, VBO+syncON, chegou MainMenu. Reinstalado144, marcadoresOFF,
boot147 chegou MainMenu e HUD. Falha é intermitente; esses controles não isolam
causa nem aprovam estabilidade. malloc_soloader usa vglMalloc/SonyMspace;
thread safety do SonyMspace não foi estabelecida, não declarar bug de mutex.
HAVE_CUSTOM_HEAP alternativo existe mas NÃO foi habilitado; qualquer experimento
exige evidência própria. Nenhuma mudança em atomics/kuser foi feita nesta etapa.

147: playerid1 durantecarga valid0; id3 apóscarga valid1, frame13695 age1,
vtable98c13e28, XYZbase(-18944.2,9536.3,-8712.6), combat_index0. Não é estado de
combate decodificado. state_profileid2: sampled_span4951096us,dropped0, maior
chamada1584107us durante transição. Nomes ficaram vazios pois estão noheap,
excluído pelo guard atual. NÃO usar essa chamada como custo de frame estável.
Nova janelaid4 apósestabilizar: span4998321us,dropped0; maior contexto8038us
acumulados/1216calls,max40us. Outro6919us/2432calls,max21us. Não repetiu pausa
de1,58s; StateAutomat não explica sozinho o gargalo contínuo dessa cena.
Log estável tasm_147_cpu_final.log após fechar; nenhuma travessia nesse boot.

Limite descoberto no diagnóstico144: report é chamado pelo watchdog/perf_report
e pode ler a tabela enquanto main retorna de uma amostra. Mover report para
tasm_dev_frame e adiar enquanto houver chamadas amostradas em voo; swaps podem
acontecer dentro de StateAutomat durante load. Não confundir ajuste de probe
com correção de corrupção do jogo. Ainda não implementado neste registro.

148 controle em andamento: mesmo binário144, sem nova instalação/compilação,
ambos marcadoresON. Esperar log fresco/MainMenu antes de inputs. Script anterior
continuou após timeout144; corrigir orquestração para parar ações dependentes
ao falhar espera, sem usar estado herdado como confirmação. Resultado pendente.

148 resultado: mesmo144+marcadoresON chegou MainMenu/HUD semcrash. Capture1
estava em cinematic, embora menuHUD; HUD sozinho NÃO prova gameplay pronto.
Cross pulou cinematic; captura2 confirmou tutorial inicial, prédios detalhados.
Percurso: Cross250ms, depois Cross+forward(128,0)1800ms e soltar ambos. Ao
estabilizar chegou ao deck e inimigos (tempo menor que2400ms usado antes evita
terminar fora da borda neste cenário). Player logo apósmovimento estava emvoo,
XYZ(-18749.4,9783.9,-800.7); não interpretar como altura final do deck.
Circle250ms +3Cross180/350ms registrou4HITS reais, inimigos derrubados e prompts
de ataque/ranged. Outras sequências de Cross, Circle, Triangle e Square mostram
golpes/dodge/reação e inimigos adicionais; missão NÃO concluída, semsaveaprovado.
Saúde caiu muito enquanto capturas/análise deixavam herói exposto. Captura16
confirmou combate perto da borda com saúde crítica. Não declarar24Cross como
24hits: HUD/contexto/animação precisam confirmar. Antes de retomar ataques da
pausa esperar menuHUD; Circle e próximo comando imediato podem pegar transição.

148 emcombate: janela semcaptura com89frames/5113ms (~17,4FPS), WorldUpdate2455ms,
WorldRender1993ms, objects1589ms, physics756ms, state137ms. Tempos inclusivos,
não somáveis como fatias de CPU. Persistem gargalos de física/objetos/render.
Capturas ~1,5MiB bloqueiam vários segundos; janelas contaminadas excluídas.
city_shadowid13 emPause retornou0/6400diferenças e0pendentes apóscombate,
92732notificações(35294copy/57438native), chunk1024B. Água azul permanece.
App fechado, log estável tasm_148_vbo_diag_final.log; BMP temporário ainda será
apagado ao fim da inspeção. Marcadores mantidos apenas para próximo controle149.

149: corrigido relatório de state_profile: sai de perf_report/watchdog e roda
em tasm_dev_frame/main. Contador de amostras emvoo impede report/rearm no meio
de chamadas externas/recursivas, inclusive swaps dentro de load. Flag offmain
sóconsulta atômica. Sem mudança na lógica nativa/heap/streaming/qualidade.
Buildexit0 SHA0D6417ADC9EA2A480EE3DB4E881D6E210DF28ED6BB441202056075A511166646,
archive build-149-main-profile-report; instalado após encerrar148. MesmoXML124
e dois marcadoresON. Inicialização excedeu primeira espera25s, mas log mostrou
I/O/compilação progredindo (não crash); espera seguinte confirmou MainMenu.
id1armado antes de WorldLoad expirou semamostras, span0: não é custo zero do
load. id2 na transição/rooftop: span4972161us,dropped0; top6189us/944calls,max13us,
segundo5906us/1888calls,max25us; relatório coerente emhardware, nomesheapexcluídos.

REA149 readonly: CPhysicalComponent::Update e CGameObjectManager::Update via
CLI/provider VitaSDK objdump, ELFhashinalterado. Evidências
ev_4e991a98ee68333d54b9bd742180653d68f04ed496ede98b6ecd2cc56bc035d5 e
ev_731ead67e184057cc9618515fdebccf3d1de60187f6e95ac534ce89cae95b661.
Física chama preprocess, calGravity e CollisionEllipsoidWithWorld; presença
estática das chamadas não mede qual é quente. Primeiro parse foi prematuro
(CLI aindaexecutava), falhou JSONvazio; aguardada saída0 antes da leitura válida.
Não instalar/mudar motorREA nem alocador. Próximo: testecontrolado das distâncias
de streaming/LOD, mantendo mipmaps e câmera para separar carga de qualidade.

149 estabilizado: janelas25,4–26,0FPS (~124–125draws/f), WorldUpdate~1,46s/5s,
WorldRender~2,82s/5s; imagem do telhado inicial conferida. Nenhuma travessia neste
boot. Log final tasm_149_profile_final.log após fechar. Header de build mantém
15:06 pois main.c não recompilou: horário do banner não identifica o VPK sozinho;
usar hash/arquivo de boot e request_id fresco. Config remoto coletado antes do
teste150: config_before_radius150.xml, SHAigual124 (08a1ee...1501c).

150 experimento, MESMO binário149 sem rebuild/reinstall: só Streaming.RadiusAdd
15000->10000 e RadiusSafe18000->12000; pares originais do perfilVita herdado.
Mantidos CameraNear/Far, distânciasLOD, mipmaps/shaders/câmera, ambosmarcadoresON.
XML config_radius_150.xml SHAfd3e5a9d89cbab5711d523256b665fa37a422b2d26ca1bda140da4acef34ef77.
Objetivo: separar custo de prefetchar área maior do detalhe geométrico/textura.
Não aceitar FPS maior com prédios ausentes ou streaming tardio. Chegou MainMenu,
Cross entrou HUD, outroCross pulou cinematic. Primeiro pull ainda carregava:
21draws/f, citysync121calls/13840KiB/340487us e programa73 sendo criado; isso
NÃO é baseline estável. Resultado e restauração ainda pendentes.

Auditoria de cache durante150: HAVE_SHADER_CACHE está ativo, path é definido
ANTES de vglInitExtended; não é bug de ordem de inicialização. Cachehit pode
ainda custar leitura/alocação/patcher, portanto compile END4–60ms não prova
recompilação do GLSL. Cacheparser/GLbinary legados têm checks limitados e
vglGetShaderBinary ignora bufSize, porém DUMP_COMPILED_SHADERS estáOFF no build
atual. Nenhuma mudança de cache nem limpeza indiscriminada foi feita aqui.

150 resultado: telhado~26,1FPS/124draws, custoWorldUpdate~11,2ms/f semelhante149;
imagem inicial equivalente visualmente. Travessia capturada aindaemvoo, cenário
coerente. Ganho pequeno dentro da variação, sem testeextenso de pop-in; raio
menor NÃO adotado. Appfechado, log tasm_150_radius_final.log.151/152 restauram
os raios15000/18000 do124; não sacrificar antecipação por ganho não comprovado.

151 testa só Shader.LowRes_Sea1->0 sobreXML124, mesmo binário149 eVBOsyncON.
Arquivo config_sea0_151.xml SHA d9c7916a5cb7aacc26261b11ef8f1b8c67f7d48cb56d122d3b370346956646ad.
Boot ultrapassou primeiraespera30s, logprogredia/nenhumdumpnovo, depoisMainMenu.
Primeiracaptura era totalmente preta durante carga/transição, próxima mostrou
normalmente telhado inicial. Não chamar framepreto de crashnem shadercorrigido.
Capturas4..8: travessia atéborda doátrio, depois Cross+diagonal210,20/1600ms subiu
ao deck. Recuar1700ms e girarcâmera1800+1400ms abriu vista da piscina aindaazul
sólida. LowRes_Sea0 NÃO eliminou o sintoma; restaurado XML124 apósfechar151.
Log tasm_151_sea0_final.log; sem nova causa de crash comprovada. Ataques após
subida não geraramcombo visível, não contar como novo combate aprovado.

Automação: releaseCross imediatamente seguido de pressCross no mesmo comando
pode não ter intervalo observado pelo poll do jogo. O percurso de250ms+jump
1800ms nem sempre termina no mesmo ponto; precisa release-gap>=umframe lento,
por exemplo120–200ms, antes da nova pressão. Isso é hipótese de reprodução da
automação, não prova defeito de input físico. Giro limitado e parada da câmera
observados ao soltar; valoresangulares ainda não medidos.

Criado tools/make_runtime_config.py: gera novo XML grupo/prop com --set
GRUPO.PROPRIEDADE=VALOR, imprime diffsemântico+hashessource/output, rejeita
propriedade inexistente/repetida, source==output e outputjáexistente. SaídaUTF8
comLF determinístico. Positivo151passou; negativapropriedadeinexistente deu
exit2semcriararquivo; negativooutputexistente deu exit2semsobrescrever.
Primeiro py-c comXPathfalhou porquotingPowerShell antes de qualquerpush; helper
evita repetição. Reproinicial sobrescreveu localconfig150 comLF (mesmosvalores,
hashdiferente); bytesCRLF implantados foram recuperados eSHAfd3e...ef77conferido.
Guardoutputexistente adicionado; reproagoraemconfig_radius_150_reproduction.xml,
SHAe8c1927f7ed48ff57ffdf74abc94d2a73d0881a5cab001eef7c38f042a2d1cc9.
update_port_knowledge.py passou a distribuir helper para scripts/da skill local,
parent e duasglobais, validando hashes. Sync/validação final ainda a executar.

152 planejado/emboot: XML124 restaurado, mesmo149, VBO+syncON, material_probe
opt-inON. Objetivo: mapear programas/texturas e identificar shader da água em
uma frame real, em vez de continuar alternando flags sem causa. Probenão altera
shader, mas grava fontes/mapa: excluir esse boot de comparação de cargaeFPS.
Remover material_probe e capturas ao fim; manter investigação COLLADA aberta.

152 resultado: boot e telhado normais. Traceid1 terminou trace_frames1,125draws;
program37(64/65) fez2draws309índices, program68(128/129) fez1draw54índices.
Fontesoriginais dessesmateriais de água usam global_SeaColor(0,0.2,0.67), cubemap
com fator0.2 e distorção animada. Samplers68 DistortMap=unit0 (tex99,64x64ETC1),
EnvTexture=unit1(cube101,32x32,cubemapligado). Payloadcube10118432B preservado.
Cor medida no BMP151 da piscina: modosRGB(7,62,190)/(8,61,191), compatíveis com
baseazul+reflexo fraco do shader. A aparência azul sozinha NÃO prova bug noport;
falta comparativoAndroid ou isolação de reflexo/tempo para julgar fidelidade.
Não alterar artisticamente a água para mascarar problema. Os dois branches
LowRes_Sea diferem pela aplicação de LightMap sob USE_LIGHTMAP, não por um
reflexo totalmente diferente. Logs/material-152-* contêm mapa, draw e fontes.
Fechado152; material_probe removido, BMPremoto/local apagados. XML124 mantido.

153 contagem de sched_yield: probe DEBUG opt-in sched_probe.enable distingue
main/workers e primeirocaller; perf_report mostra contagensdelta. Segundo
marcador sched_yield_delay.enable permite experimentoDelayThread1us somente
comprobeON, mas NÃO foi ativado; arquivo ausente(rm550). Prioridades e sleeps
sub1ms mantidos. Buildexit0 SHA A31AF69E686330376D9BFF2C29456C909C29672133D5FF219F29E632BBF8C421,
archive build-153-scheduler-probe. Warnings herdadosvolatile/mktemp; semerrobuild.
Em todos os relatórios observados do menu, WorldLoad e telhado: yieldmain0/
worker0, erros0. Logo, não há evidência para adotar mudança de sched_yield nesse
cenário. REA glf::Thread::Yield6407b8 é branch para sched_yield@plt; evidência
ev_68f51fdf303b76f293b8b3b3d06a0975821774dbd49a375e8a01541988310cbe.
Existência estática do import não prova uso. Fechado153, logfinal
tasm_153_sched_final.log; sched_probe removido, delaynuncaativado.

154 correção concreta dos logs: utils/logger.c e FalsoJNI_Logger.c passavam o
texto JÁformatado diretamente a sceClibPrintf(texto). Percentuais e conteúdo
shader/JNI podiam ser reinterpretados como novos especificadores variádicos.
Ambos agora usam sceClibPrintf("%s",texto). Não atribuir crash144 a isso sem
reprodução. Logger também limita write_len ao buffer ao acrescentar sufixo de
repetição: snprintf retorna tamanho pretendido, que pode exceder espaço restante.
Devcommand log_percent_test exerce os dois sinks com literal100% %s %d %n %%,
depois registra retorno. Só rodar na build corrigida, não provocar UB na antiga.
Primeiro build083D50EE544B20C23852CC6895284E5D0774E7A1A5043B2D76E04E33C8DE4EC8
não instalado; adicionada chamada direta ao sinkFalsoJNI no selftest, buildexit0
finalSHA61BD7D60B6A8283D97455BD38FBC7D4A17378857709043F2F53CBC41DA14DFBC.
Archive build-154-log-percent-fix inclui logger/dev/FalsoJNI/diff. Instalado,
boot154 emandamento, XML124 e VBO+syncON mantidos; demaismarkersOFF. Selftest e
gameplay ainda pendentes. Skillvalidada globalmente em152; sync final pendente.

154 hardware: log_percent_testid1 confirmou as duaslinhas literais, incluindo
100% %s %d %n %%, e retornou/continuou atéHUD. Chamada direta FalsoJNIwarn está
ativa(DEBUGLEVEL1<=WARN2). Não foi feito teste negativo provocando crash antigo.
gitdiff--check e py_compile dosdoishelpers passaram. Banner16:55:34 identifica
main.c recompilado em153, não substitui SHAfinal154.

154 percurso com release-gap: jump250ms, release200ms, jump+forward1800ms,
soltarambos800ms aindaemvoo; depois nova pressãoforward1000ms,release800ms
chegou diretamente ao Web-Strike no deck. Sem concluir que temposfixos servem
em toda carga; o prompt direcional pode aparecer só depois do primeiro gesto.
PrimeiroCircle+6Cross250/500 não mostroucombo, houve hits recebidos. Log mostrou
Circle e Cross down/up chegando emHUD; não foi comprovada perda deedge nesse
caso. RetomarPause comCircle220ms/release250ms e esperarHUD; depoisTriangle180,
Circle250/650 eCross120/250 em lotes confirmou13hits, depois16hits. Novasondas
de inimigos apareceram e foram vencidas. Pausa apóscaptura para não exporherói.
~21golpes+esquivas porlote não significam21hits: contagens sóquandoHUDmostrou.

154 primeiro combate FOIconcluído: inimigos desapareceram, XP+20 e diálogo
postcombate. Próximo objetivo marcouNPC. Aproximação à janela reproduziu
enquadramento da câmera dentro/colada à parede, parte do trajeclipada. Ainda
precisa separar câmera nativa, interação e possíveldefeito de port; não declarar
resolvido. UmCross300ms iniciou closeup da NPC; novoCross180 avançou interação
e personagem passou a carregá-la, objetivo74m. Nenhuma teleportação/edição de
posição/vida/script foi usada. Câmera girou por gestos e ficoudown0/deltas0 ao
soltar; há autocâmera/contextos nativos a distinguir da sensibilidade do port.

154 resgate: rightstickleft500ms, jump+forward1800ms chegou à borda(65m).
Novo jump+forward2400ms, soltarjump e manterforward800ms deixou emvoo(6mXZ).
Pausa; retomar e5s semmovimento (janela state_profileid26) pousou na rua(13m).
Down1800ms reduziu para9m, Left2400ms para3m, Right800ms entrou na área e NPC
foi posta no chão automaticamente. Cross180ms apóscena mostrouXP+500/tutorial
domapa; emseguida overlaylevel_up confirmou LEVEL2, recompensa20/25. Primeira
missão/resgate concluídos emhardware. NÃO é aprovação de todojogo nem de save.
Estado atual nesse ponto: binário154, menu level_up aguardando, request34.
Select220 enviado durantelevel_up foi ignorado, não teste negativo do mapa.

154 city_shadowid15 apósluta e33 apósrua/resgate:0/6400diferenças,0pendentes,
74134notificações(30171copy/43963native), chunk1024B. Id32 memóriaemlevel_up:
RAM344KiB/VRAM6348/PHY268livres, newlibused72895/freeBlk7816KiB. Houve muitas
pausas (frame~70mil); NÃO chamar isso70milframes de gameplay contínuo. Logs
tasm_154_literal_test/first_combat/postfight/rescue_map.log; final estável pendente.

Auditoria de persistência: vita_savedata_list lista só5arquivos sce_sys. Backup
genérico de04:17 NÃO cobriu os saves Android emux0:data/tasm. Lá existem
login.sav,glcloud.sav,tempSaveFile.dat; tamanhos reais/checksums ainda a coletar
com appfechado antes de qualquer mudança. Nenhum save foi editado nesta etapa.
I/O remapeia fopen/remove/unlink, mas rename aindaimporta libc diretamente:
inconsistência de namespace/cache a investigar com caminhos/retornos reais,
sem afirmar que explica perda deprogresso antes do teste de reload normal.

REA154/155readonly: SaveAllBytesOfSaveFile24aa28 somente copia para arraysRAM,
não grava disco. CombineAllSaveFile24b2bc serializa e chamaGaiaManager::SaveCloudSave
2408dc, que usaSaveGameManager BeginSave/SaveBuffer/EndSave. GetSaveFilePath80b688
constrói std::string; AcceptSaveRestore80f694 eEndSave têmrename@plt. Evidências
ev_629b9ea6295b2837cb7ac18adf6ab6875c3153b4cbfdae34a6cabd5eead47325,
ev_653a1139405c7223ec92de80f8ce5ed6d1a4bd769d98c8b0e3f8d452a3483218,
ev_34b5ac1c6773870f0f0f4ae7c8782bcbb6b6ac6e509d248a9125b8bea01d8249,
ev_abfa670aeb8ef472e052b038068af75c146dcedfdf34f4fa9633f08648a195c4.
StickWalk::touchEnded34bfa8 checa down+26 eID+36; não checa coordenada deUP.
Limpa down, shorts+56/+58 e floats+92/+96. Hipótese deUPfora do retângulo
ignorado NÃO é suportada por essa implementação. GetOffset34c050 sólê shorts
e escreve resultado do caller. Evidências ev_bf6ad30670dd718a27c9391e62e959c5eda1af08b69c06685188fefeeaf825c0 e
ev_639cf3f9f8501b78307b152fa67c2bf6804043ead830177316957c0dcfb3fab0.
Nenhum probe/mudança155 foi implementado atéaqui: nomesdosarquivos indicam
investigação planejada, não build implantada. L1key102chamaBtnHeadReleased,
R1key103usaGetEarthquake; comentárioCamera/WebRush anterior precisa correção.

Próximos passos autorizados: fecharLevelUp com açãoOK/back(segura, semShare),
testarSelect/mapa, sair pelo menu normal para permitirsave, fechar app e fazer
backup explícito dos pequenos arquivosAndroid/config. Reabrir mesmo154 e
comparar level/objetivo persistidos. Se falhar, registrarrename com seus caminhos
e testar contrato fopen-remapeado/rename usando arquivos descartáveis próprios.
Depois travessia maislonga/streaming, perfil de física/objetos/render, câmera ao
encostarparedes e texturas próximas. Crash144 e estabilidade longa continuamabertos.

154 conclusão e saída: Circle fechou level_up (semShare); Select abriu mapa do
tutorial. Circle nele levou aPause, nãoHUD; esperaHUD10s expirou e orquestração
parou oStartdependente. Captura confirmouPause. Right3vezes selecionouMAINMENU;
Cross abriu confirmação de saída, YESselecionado; outroCross voltou MainMenu.
Assim houve saída normal do gameplay antes de fechar processo, não só kill.
Backup Android feito antes dessa confirmação e novamente apósfechar154:
_logs/save-154-before-menu-exit e save-154-after-menu-exit. Arquivos reais:
login.sav276B SHA11B6C799B2C4AE68B8AB0B7973A7513E1D72F2F4DB903BFED72C29CDDE1F023A;
glcloud.sav0B(SHAe3b0c442... vazio); tempSaveFile.dat948B
SHA3FED3D3A78143DF46FAFACCAF034B7E8D988B2ED7E1E3B4263AC019E963A0A6A;
override2980B SHA08A1EE...1501C. Conteúdo de login não foi exposto; índice sóhashes.
Log estável tasm_154_level2_final.log (~930KiB), semmatchesFAILED/overrun/NULL/map
encontrados pela busca (rgexit1 significou nenhum match, não erro de build).

155 controle de persistência: MESMObinário154, semrebuild/reinstall, relançado.
Chegou MainMenu ePlay/HUD, captura frescaconfirmouLv1/telhado inicial61m/PressX.
Logo, progressoLv2 NÃO persistiu, mesmo com saída pelo menu. Fechado155, log
tasm_155_reload_lost.log. Backupsave-155-after-reload: login276 eglcloud0iguais
154, tempagora796B SHA49b67dbe7604fd741e269f9aa4ea3b3a6c346578754b56d8a10a563efc586844.
Backup948BLv2 preservado ehashconferido antes de instalar nova versão; ainda
nenhum restore/cópia manual dele paraarquivo principal. É necessário descobrir
transação exata antes de declarar recuperação.

156 correção/teste de namespace: novo rename_soloader remapeia origem e destino
como fopen/remove/unlink; importnativo passa a usar esse wrapper. Invalidação
de cache após sucesso, errno preservado apóslog, logfs-rename inclui caminhos,
retorno/errno/caller. Sem fallback de substituir destino ainda: primeiro medir
semântica real da libc. Devio_rename_selftest cria somente doisarquivos próprios
logs/_tasm_rename_selftest_{source,target}.tmp, faz baseline renameAndroidraw,
renomeia aliasAndroid->Vita, compara32bytes porSceIOdireto, tenta substituir
destinoexistente e testa sourceinexistente semmudar destino; limpa ambos.
Resultado6 identificará falha na substituição; não usar success de cmd.json
como success do teste. Nenhum arquivo real de save é alterado pelo selftest.
ComentáriosL1/R1 corrigidos paraBtnHeadReleased/GetEarthquake; sem remap de teclas.
Buildexit0 SHA8B69099DA35B339C424B9FAB6BFA46F1E745F98594BD235182AF15C0BDD8DDD3,
archive build-156-rename-namespace, instalado emboot156. XML124+VBO/syncON,
outrosmarcadoresOFF. Hardwaretest e retorno de save pendentes.

Auditoria adicional: há código legado de fcache_invalidate que remove entries
por swap e não mantém lifetime de handles abertos. Porém FCACHE_ENABLED=0 neste
source/build: não atribuir a ele crash144 nem ativá-lo como otimização. Nenhuma
mudança/teste foi feito nesse caminho inativo. Conferir flags antes de perseguir
código suspeito. Cache compartilhado de archivesé outro caminho, continuaativo.

156 resultado: boot ficou longo período em MainMenu entre continuações do usuário;
~737milframes são de MENU, não gameplay. O rename corrigido foi usado de fato:
EndSavecaller9880ff24 renomeou '/data/data/.../glcloud.sav' para backupSaveFile.dat;
caller9880ff70 renomeou tempSaveFile.dat para glcloud.sav, mappedux0:data/tasm,
ret0. Isso confirma o namespaceAndroid usado pela transação que antes falhava.
Pulltasm_156_rename_selftest.log relatouEEXIST mas arquivoresultanteválido mostrou
resultado8: alias/remapeamento/substituição passaram, testeorigeminexistente não
preservou o destino. SóACKrequest1 não teria revelado essa falha.
Appfechado, logestável tasm_156_rename_final.log3,7MiB. Backup save-156-before-recovery:
login276B SHAigual154; glcloud140B SHA2b980ad66225335641ff968cc249941bb582280c1dbdc1137f2aac3dfa54954c;
backupSaveFile144B SHA805cec84cd2caf992ca9bbf6963eaa4a69c1a8a325169c92a052e08a523fdbe6.
O arquivo principal deixou de ser vazio; são defaults doMainMenu, nãoLv2recuperado.

157 REAdo SDKlinkado: _rename_r doELF156 resolvepaths, chama sceIoRemove(destino)
ANTES de sceIoRename(origem,destino), semstat de existência daorigem. Assim pode
apagar a versão anterior e depois falharENOENT. Evidência
ev_2e35e69893577fd7bec1064353930adb34e4a5b81ae28e8b3e9c3d183f22f76e,
arquivo rea-sdk-rename-157.json; ELF156 preservado emseuarquivo de build.
Novo wrapper serializa chamadasrename com mutex e fazstat(origem) antes dessa
libc. Isso protege o caso medido de origem ausente e renames concorrentes pelo
wrapper, não constitui garantia geral de transação durável/powerloss. Status,
errno ecaller passaram ao INÍCIOdalinha, pois deduperlegado compara prefixo128B
e podia ocultar retorno diferente depois dos longoscaminhos.

157 buildexit0 SHA02B7BF4E2008B8450B424373DF74E3C695E64B048BA0CAEA1842F558B566346F,
archive build-157-rename-source-guard, instalado/boot157. Selftestrequest1 passou
result0, legacy_ret-1/errno2, missing_ret-1/errno2, previous_preserved1. Também
testou mesmo arquivo por aliasrelativo/Vita, além de substituir destinoexistente
e comparar32bytes viaSceIOindependente. Só arquivos descartáveis foram usados.
Log tasm_157_rename_guard_test.log; fechado com tasm_157_guard_final.log.

158 recuperação autorizada do checkpoint: appfechado; arquivosatuais preservados
emsave-157-before-level2-restore e identidade login276B conferida igual154.
Os renames nativos confirmam que tempSaveFile.dat e glcloud.sav/backupSaveFile.dat
usam os mesmos bytes/contêiner. Copiado backup948Bdo154 para principal ebackup,
sem editarpayload. Readback dosdois emsave-158-readback confirmou exatamente
SHA3fed3d...a0a6a. Sem reinstalar: lançado MESMObinário157 emboot158. Chegou
MainMenu/Play/HUD; primeiroBMP estava preto durantecarga. Recuperação emjogo
ainda NÃO confirmada nessa primeira imagem. Original948Bcontinua preservado.
Nenhuma evidência de que arquivoRPCack equivale a savedataload aprovado.

158 resultado posterior: imagem fresca confirmou Lv2/BREAKING NEWS, sem tutorial
inicial. Posição do checkpoint (-24835.5,9140.9,-1047.7), diferente da inicial.
EndSave nativo voltou a gravar pelos renames corrigidos; principal e backup
passaram de948 para952B. Cópias privadas emsave-158-level2-native e logs
tasm_158_level2_recovered.log/tasm_158_recovery_final.log. Original948B preservado.

159 segunda abertura do MESMO157, sem nenhuma cópia manual intermediária:
Lv2/BREAKING NEWS novamente. Persistência nativa do checkpoint confirmada,
não apenas restore visual. Fechado159 antes da instalação seguinte e preservados
login/principal/backup emsave-159-level2-native. Pull do log tentou primeiro
path incorreto data/tasm/tasm.log (não existe); path correto logs/tasm_159.log
encontrou erros FTP426/PASV, a coleta estável desse arquivo precisa nova tentativa.

160 experimento em andamento: runtime native_profile_on/off desliga apenas os
cronômetros dos16 wrappers nativos; todas as chamadas originais continuam uma
vez, snapshot do jogador permanece e state_profile explícito continua possível.
DefaultON preserva controle. JSON expõe native_profile_enabled; comandos têm
request_id e ACK individual, seguido de verificação do efeito. Compilação
pendente foi reencontrada sem processos ativos; make repetido terminou exit0,
VPK SHA F34CEBBD6C3B9A17D12EB89F541F1FB57190E8DE684419D427869F3164C0E679.
ELF/VPK/flags/diff/fontes específicas em build-160-profile-toggle. Instalado160.
git diff --check passou; uma busca PowerShell com logger.* falhou por glob não
expandido e foi refeita com filename explícito. Não houve alteração por essa busca.
MainMenu apareceu cerca de57s de processo; primeira espera35s expirou ainda
compilando/carregando, depois chegou. Play/HUD apareceu em~70s, primeira captura
~74s ainda preta; nova captura~99s confirmou Lv2/cidade. HUD não certifica carga
concluída. Pedido player_position era comando não suportado, corrigido para player.
Teste ON/OFF/ON/OFF na mesma pose; descartar os primeiros5s após cada transição
e janelas com upload/captura/save. Medição FPS independente permanece ativa.
Nenhuma redução de qualidade, distância ou ajuste novo de sensibilidade.

Próximo: concluir A/B160, reavaliar gargalos pelo custo medido, depois ampliar
percurso ativo/streaming/combat. Crash144 e clipping da câmera perto da parede
continuam abertos. Ajuste de patch de documentação anterior falhou uma vez por
contexto e foi refeito sem alteração parcial; planos não são resultados.

160 resultado A/B: mesma posição antes/depois (-24835.5,9141.7,-1047.7), câmera
neutra, city-sync0 nas janelas estáveis. ON/OFF/ON/OFF confirmado no estado.
Starts time_us/frame: ON117747126/1824, OFF153557765/3118,
ON189751999/4452, OFF226365866/5751; final261548923/7027.
analyze_profile160.py preserva todas as janelas e exclui duas primeiras após
cada troca (limite conservador>5s), uploads textura/buffer e frame max>=100ms.
Mediana ON35.90FPS(n8), OFF37.05(n7), ~3.2% nesta cena,39–40draws/f. Não extrapolar
para combate/cidade inteira. Há drift temporal: OFF1~37.1 eOFF2~36.5; medianas
não são ganho causal exato. Sem capturas/movimento nos blocos. Log original
tasm_160_profile_ab.log e profile160-analysis.json mantidos. Upload4MiB e
autosaves ocorreram em outras janelas; pausas mesmo semupload também existem.
Pull159 foi enfim concluído emtasm_159_final_retry.log; tentativa anterior
deixou um diretório com nome .log e nextdownload usou nome novo, sem apagar nada.

160 travessia: ~9s de três impulsosCross+forward; captura/pausa no fim mostrou
personagem no ar e cidade presente. Janela antes da captura18.52FPS/85draws/f,
frame max84ms, city sync1360KiB/50.6ms/5s errors0, cópia de vertices~246MiB/669ms.
Apósresumir/aterrissar, nova imagem mostrou fachada próxima ainda muito borrada,
questão aberta. Não recolorir nem reduzirqualidade para ocultar o defeito.
Geometrytrace128draws distingue malhasCPU do VBOcity; comparar somente basecity
não cobre todos os buffers. tasm_160_profile_traversal_final.log estável após
fechar; backupsave-160-before-mesh-test: principal/backup968B,login276B.
ImagensBMP temporárias removidas local/remoto apósinspeção.

161 preparo de próximo A/B: timingnativo defaultOFF após160, continua ativável.
Novo native_mesh_vbo.enable é OFF por ausência. Somente callerSO+0x4760f4,
type0/usage4/data!=0/flags0/ring1 pode clonarconfig parausage1; teto cumulativo
8MiB por processo, sem devolução dequota quando objetosmorrem. Outrascriações
inalteradas. Binário mostra essecaller construindo config sobre dadosCOLLADA;
não há ainda prova de imutabilidade/lifetime de todos os dados, portanto NÃO
adotar nem declararcorreto sem imagem, updates, memória e travessia. Nenhumalias
de ponteiroGPU: usa caminhoVBOoriginal do motor. Disassemblies readonly em
native-mesh-buffer-caller-161.txt e native-mip-policy-tail-161.txt, complementam
evidênciaREA preservada do motor. Leituras de dois filenames inexistentes
vertex_array.c/get.c foram corrigidas por inventário; código não alterado.
Buildexit0 SHA0C0A7B39465EC8B26E4FFEA4EB8022C89C0E2FFA53A32FB071240083A401FD1F,
archivebuild-161-mesh-policy-optin, instalado/boot161 com marcador aindaOFF.
Próximo: controle161 vs mesmo binário com marcador162, excluir carga/capturas;
reverter imediatamente se faltar geometria/updates ou memória insuficiente.

161 controle: defaulttimingOFF confirmado; Lv2/pose idêntica ao160. Apóscaptura,
váriasjanelas~37FPS/39–40draws/f e~1.2MiB de cópia devertices/f. Memóriarequest3
coletada antes defechar, logtasm_161_mesh_control_final.log. Só hashes de três
regiões de imagem guardados; BMPtemporário removido. Skill/docs sincronizados
com1062arquivos de evidência e conferênciabyte-a-byte.

162 MESMObinário161, somente native_mesh_vbo.enable ON. Código de fato admitiu
váriosbuffers (primeiros36800/19024B etc), logo resultado negativo não foi teste
sem ativação. Captura Lv2 preservou cidade; região da fachada esquerda teve
SHA exatamenteigual161, duas regiões com reflexo/névoa diferiram. Isso não é
equivalência da imagem inteira. FPS~36–37 e cópia porframe permaneceu~1.2MiB:
sem ganho material neste teste. Desligado/removido marcador, app fechado e
tasm_162_mesh_candidate_final.log preservado; não adotado. Código fica OFF
por ausência do marcador para reproduzir experimento, não é recomendação.

163 próximo teste independente: Native driver helper SO+4a240c encaminha criação
e caller4a2428 retorna sret; não é identificação semântica de toda malha. Dados
runtime anteriores mostram criação type0/usage4/3MiB/sourceNULL/flags1/ring1
nesse ponto. Novo native_batch_vbo.enable altera SOMENTE esse tuple parausage1,
no máximo duas alocações por processo (6MiB). EBO1MiB, demaisbuffersemeshes,
cityarena25MiB continuam conformecontrole. É hipótese de evitar cópias dos lotes,
não prova de ownership/imutabilidade; testar imagem, atualizações e métricas.
Binário163 exit0 SHA4BD248D7FC0072016A582761B0B0F0DB4DC9837CE68F8CA675D16180A3FA420C,
archivebuild-163-batch-policy-optin. MarcadormeshOFF, batchON, cityVBO/syncON;
mesmoXML124, clock444 e câmera133. Instalado/lançado163; resultado pendente.

163 falhou antes de qualquerframe/estado de dev e ANTES de admitir tuplebatch.
Log473linhas termina apósshader/program20 e buffersCOLLADAiniciais. Novo dump
ux0:/data/psp2core-1793841525-0x0002313fb7-eboot.bin.psp2dmp,
bundle C:/Users/leand/vita-mcp/crashes/2026-11-05T01-18-45-000Z,
reportscopiados para_logs/crash-163. PC9847ca18,DFARfffffff5,SP819009f8,
R3=1,R4=8df63f00,LR67: MESMAassinatura/nó de144 (outrosheapendereços variam).
Não é prova de que batchVBO causou a falha: novo caminho não executou. Binário
preservado para simbolização, logtasm_163_boot_failure.log. Marcadorbatch removido
e appfechado. Boot164 MESMObinário163, batchOFF, chegou MainMenu e ficouativo;
controle tasm_164_batch_off_menu_control.log. Batchexperimento continuaSEMresultado
de imagem/perf e não adotado. TimingOFF, meshOFF, cityVBO/syncON permanecem.

165 prioridade mudou para recorrência do crash. Não alteraratomics/heapglobal
por suposição: comparar primeiro alocador em arenaPRIVADA descartável. Novo
heap_selftest explícito sóMainMenu, nunca no startup. SceClibMspaceCreate sobre
blocoUSER_RW1MiB separado, 1worker depois4,256rounds*16slots porworker, tamanhos
32..2079B. Barreiraacquire/release separa alocação/preenchimento, verificação de
sobreposição/canários e free. Workersfixados aoscores0/1/2 (quarta compartilha0),
prazo6s, joinslimitados. Ao falhar não libera ponteiros possivelmente sobrepostos;
se todosjoinam, descarta arena inteira; timeoutmantém contextoestático/arena e
exige fecharprocesso antes de continuar. Não modifica heap/save do jogo, não
altera prioridades dos workers existentes. Passar só refuta falhas cobertas pelo
teste; não prova lifetime nativo nem causageral. PrimeirobuildC13B...não instalado;
apósfixaraffinity, exit0 SHA6A4F8CA5AF2BFCC85197256557C55C14A9025AE005CAA49F1C7EB5D4BA2031C3,
archivebuild-165-private-heap-stress, instalado/lançado165. Resultado pendente.

165 resultado4 no teste SERIAL: CreateThread falhou, nenhumteste deheap ocorreu;
concurrent=-1 significou nãoexecutado, não corrupção. Prioridade0x100 substituída
por0xA0, dentro da faixa jáusada no port; logs passaram a mostrar erroCriação/Start.
166 buildexit0 SHA64C64D89E2595C992691D59702F74BE4AD25CE53CDB24D775A9542A2F0E7BDB7,
archivebuild-166-heap-test-priority. Hardwarerequest1 PASSOU: serial4096allocs
result0 em93824us/affinity10000; concurrent16384allocs result0 em602625us/
affinity70000, todosjoined, semoverlap/canárioalterado. Não confirma integridade
dequalquer objeto do motor; nãojustifica trocarheapglobal ou culparSonyMspace.
Logs tasm_165_heap_test_creation_failed.log, tasm_166_heap_test_pass.log e final.
Tanto165como166chegaramMainMenu, semnovo crash. Semgameplay nessesboots.

REA SpinLock::Lock (_ZN3glf8SpinLock4LockEv) ev_dc16f32436c877124cf946ad249f31bccd7f880a824c1d3d91c75360914a6e4b,
arquivo rea-spinlock-166.json. Helpera83954 retorna valorantigo depois de cmpxchg
e o chamador compara comzero; o literal kuser está no helper verificado. Nada
medido aqui autorizou mudar atomics/barreiras. Dump163 desassemblado como find
de std::map<string,intrusive_ptr<CResFile>>; r0map/r1key, retorna iteradornor0,
sem sret oculto. native-collada-find-163.txt/native-spinlock-atomic-166.txt.

167 coleta limitada para crash: resource_watch.enable ativa hookfind somente
após conferir prologue47c9e4 e duasinstruções nãoPCrelativas. Chama original1vez.
Endereço8df63f00 é apenas candidato que repetiu144/163 neste layout; não deve ser
copiado para outroport. Callbacks de malloc/calloc/realloc/memalign/free anotam
alocaçãoválida que englobe24B nesseendereço; wrappersC/EABImem anotamwrites
sobrepostos comcaller, sem bytes deassets. Find lê seiswords somente enquanto
callbackconhece alocaçãoviva que cobre essesbytes; não percorre ponteiros arbitrários,
não corrige nó, não mascara strings ruins nem remove consultas originais.
As primeiras32consultas e chave<1000 geram metadados; concorrência pode alterar
aswordsduranteobservação, e inline/nativecustomallocations podem não ser vistas.
DefaultOFF; mesh/batchOFF, cityVBO/syncON. Custo diagnóstico não deve virar FPS.
Retenção de arena doheapselftest emqualquerfalhacorrupção/timeout melhorada:
mantém contexto/arena até fecharprocesso; falhaCreate/Start semalloc limpa mspace.
Buildexit0 SHA62C7499A292CB2BC90F1A2CE8D0302FDE48D6160DA5125ACA3DD4FEB11D44D30,
archivebuild-167-resource-watch, instalado/lançado167 comresourcewatchON.
Resultado pendente. Novas buscasglobPowerShell falharam empaths wildcard; foram
refeitas com diretório/nomesexatos, sem mutações ou resultados inferidos desseserros.

167 hardware: chegouMainMenu (~66s), semcrash. Endereço8df63f00 foi coberto por
GlitchAlloc44B iniciado8df63ef0; wordsobservadaszero, NÃOidentificado como nó de
mapa nesseboot. O teste é dependente dolayout, não um rastreador semântico.
Logtasm_167_resource_watch_final.log292KiB; condiçãozero gerou maislinhas que
necessário. REA GlitchAlloc ev_d6fef91ded6e6b1c90dc5fb60e69078701bd6183160c01f27556c681652562ed
mostrou seleção microallocator seconfigurado, senão malloc seguido memset0.
No observado, caller244ba0 era somentewrapper genérico, não criadordoinobjeto.

168 adicionou hookexternoGlitchAlloc (ABIunsignedsize/inthint, retorno ponteiro)
apósvalidar prologue244b5c. Preserva original e registra chamadorreal se allocation
cobreendereçocandidato. Find agora imprimeheaderdo map nas16primeirasconsultas,
e wordsdocandidato só nas16primeiras ouquandohashmuda. Isso evita spamdezeros.
Buildexit0 SHA699F4A9EB1EA57AA4B5665B962D1CBAEBE9C21334AEAF64E8FA91A485EC9C47D,
archivebuild-168-resource-allocation-caller. MainMenu em~57.5s, semcrash. Endereço
candidato pertenceu a outros tamanhos/dados; header do map8dc91268 evoluiu de1
para8entradas normalmente. Logs tasm_168_resource_map_menu/final. Ainda não é
evidênciadefix. A função4703d4 chamafind470414 diretamente, semlocklocal visível;
isso não prova corrida, pois callersmaisexternos podem serializar.

169 retomou candidato batch com MESMO168, resourcewatchOFF/meshOFF/batchON.
Admissão3MiBcount1 confirmada, mas parou antesMainMenu/primeiroswap. Dump
ux0:/data/psp2core-1793842877-0x00025b3ecb-eboot.bin.psp2dmp,
bundle C:/Users/leand/vita-mcp/crashes/2026-11-05T01-41-17-000Z; reportsem_logs/crash-169.
PCe00004aa,SceLibKernel, LR8100ff9b dentro de tasm_city_memcpy. R0dest91b53280,
R1sourceNULL, tamanhooriginalR4=0x1d70(7536B). É assinaturaDIFERENTE de144/163.
NativebatchCPUdata não permaneceu disponível com promoção; não ocultarNULL
nem tratar sucesso decriação como ownershipcompatível. ExperimentoREJEITADO,
marcador removido e appfechado. Logtasm_169_batch_policy_failure.log preservado.
Coreaddr2linecity_note_write eraambiguidade deinlining: abortemmemcpy/sourceNULL,
não prova de falha da bitmapcity. SourceCPUprecisaria lifetime próprio e dados
corretos antes de reconsiderar; nenhum dessespatchesfoiaplicado.

170 controle: MESMO168, batchOFF/resourcewatchOFF/meshOFF, cityVBO/syncON,
chegouMainMenu eframesavançaram>5400. Não confundir FPS58deMENUcomgameplay.
Próximo: confirmarHUDcontrole e melhorar coleta dosnós/callersreaisdocache para
recorrência144/163. Auditoria readonly achou importslegados glFinish,
glCompressedTexSubImage2D e glIsBuffer ainda ret0; glTexSubImage2D é wrapperreal.
Isto sozinho NÃO prova uso oucausa deartefatos. Contar runtimeantes de outra
mudança; glFinish temcomentário deotimizaçãoherdada, não validaçãoatual.

170 entrouPlay/HUD e captura fresca mostrouLEVEL2/cidade/poseesperada. Câmera
neutraactive0/down0/deltas0. Logestáveltasm_170_baseline_level2_final.log; appfechado
eBMPtemporárioremovido local/remoto antesdainstalação171.

171 aprimora recurso diagnóstico: espelha APENAS comparações de caminho de find,
até64nós/strings<=4096B, com sceKernelFindMemBlockByAddr antes de ler spans.
Não corrige ponteiros nem substitui resultado: originalé chamado1vez depois.
Registra seiswords do nó inválido ANTESdo abort esperado, mesmo se o core não
capturar heap. Spansmapeados não provam lifetime; concorrência pode mudardados
entre observação/calloriginal. Também mede entradasfindmain/worker e overlap;
isso NÃO cobre todos os inserts/erases nem comprova corrida se só há ausência.
MarcadorresourceON, batch/meshOFF. Buildexit0
SHAAF9590B1F2A9A620ECCED9257BDA7CC852C02CDEB1537AD73E40908794458B74,
archivebuild-171-resource-path-probe. Instalado/lançado171; resultado pendente.
gitdiff--check e py_compile dosdoishelpers passaram. Próximo: conferir paths
observados e threads, repetir boot/carga até colher assinatura sem mascará-la.

171 resultado limitado: chegouMainMenu, mas nenhumresource-path apareceu apesar
deheaderdorecurso/callshookados. Portanto espelhamentoNÃOfoidemonstrado; não
apresentar bootbom como teste de árvore aprovado. Guardadeintervalomapeado será
auditadaporretornosAPI. Logs tasm_171_resource_paths_menu.log e
tasm_171_mapped_probe_skipped_final.log. Fechado antesdenovainstalação.

172 agora registra oitoqueriesiniciais: FindMemBlockByAddr(pointer,bytes) e
GetMemBlockInfoByRange. Autoriza leitura só com retorno>=0, accessR e intervalos
contidos na mappedBase/mappedSize. Isso audita o guardareal, emvez de supor que
sucesso de compilação prova APIsemântica. Nenhuma mudança da lógica do jogo.
Buildexit0 SHA8019AAC47578CFB2BA4E11A43462F457065DB5B66AB1D27E738BF7D2222803CB,
archivebuild-172-mapped-span-queries, instalado/lançado172, resourceON/outros
experimentosOFF. Resultado pendente. Skillganhouassets/mspace-selftest com os
fontes testados, integração e limitações; updater copia fontescanônicos e verifica
hashes nosglobais. py_compile e diff--check passaram; sincronização finalpendente.

172 resultado: GetMemBlockInfoByRange retornou0/access6 e intervalos reais,
FindMemBlockByAddr com bytes!=0 retornou80020001. Pathlogs agora exibem nós,
keypointers e comprimentos34/33/39, logo espelhamento foi executado nesta versão.
Nenhum nóinválido/workerfind/overlap visto. Pullmenu reportouEEXIST mas arquivo
resultante válido foiinspecionado; logestáveltasm_172_resource_paths_final.log.
173 MESMO172 semrebuild/reinstall repetiuMainMenu ePlay/HUD, semnovaassinatura,
semresource-thread/invalid/unmapped observados. Isso não excluioutros acessos
concorrentes ou timingalteradopelaprobe. Logestáveltasm_173_resource_paths_final.

AuditoriaSDK172: malloc->_malloc_r e chamadasdiretas __malloc_lock; a função
__malloc_lock chama sceKernelLockLwMutex e unlockchamaUnlock. Arquivo
sdk-newlib-malloc-lock-172.txt. Portanto não tratarnewlibcomoallocatorsemqualquer
lock, nem dizerque testeprivadoSonyMspace exercitou todaaheapnewlib. Source
_newlib_heap_size_user=150MiB; intervalo retornadopeloSysmem tinha170MiB, oque
não equivale ao espaço utilizável/livre doallocator. Nenhuma mudança deheap.

174 nova coletaoptin sync_probe.enable, inicializadaantesdeentrynativo:
sem_wait temlegado timeout2s e retorna0mesmocomerro; sem_trywaitespera1ms;
sem_timedwait temcondiçãodeadlineinvertida eexpressãotv_sec*1000*1000 semcast
prévio64bit (auditar tipoantesdeatribuir overflow). Registrarcontagens/caller,
kernelret/tempousados e deadlinewide, PRESERVANDOsemânticalegada. Ausência de
chamadas não provarácausa nem autoriza mudaragendamento. Não há correçãoainda.
Primeiracompilação falhou porqueinitializerfoiantesdosheadersutils/logger;
reordenado eadicionado sys/time.h para declaraçãoGettimeofday, buildposterior
exit0 SHA EA04B1DCE19AED5FECEC271BAC3F7DB6604D61A7E0EB9E6AC7A52A38844818AC.
Nenhum VPK falho foi instalado. Archivebuild-174-semaphore-observation; instalado
e lançado174. ResourcewatchON, syncprobeON, batch/meshOFF, demaisbaseline.
Próximo: lerresultadosruntimeantes de intervirnasesperas, conferirLv2/carregamento,
sincronizardocs/skillcomas evidências completas e manterpendências explícitas.

174 resultado: nenhumsem_wait/timedwait/trywait observadono boot, Play/HUD e
travessia~6s deCross+forward. Só linhaenable do syncprobe; não atribuirlags/
recursosinacabados a essesimports nessecenário. Semalterarsemântica. Resourceprobe
também não observouworkerfind/overlap/nóinválido. Captura confirmouLv2emmovimento
próximoàfachada, seguidaPause; muitasframesanterioresparadas, não sessãoativa10min.
Logestáveltasm_174_sync_probe_final.log277KiB. Desligados/removidos syncprobe e
resourcewatch; BMPsremovidos. Cópias persistentes/caches antigos seguemOFF.
Auditoriamem confirmou vglUseCachedMemTRUE; não repetir uma hipótese de RAM
uncached sem flagreal. Revisão de persistentcache antigo não levou a reativação:
NEON já existia, resultados102 negativos permanecem documentados.

175 coleta dequalidade, aindaSEMmudançaXML: novo texture_policy.enable hooka
CustomTexturePolicy::getMipmapCountToSkip em26e018, verifica push/movPCindependentes
e preserva resultado/chamada1vez. Name é std::stringdata emITexture+12 (mesma
leituraverificada noELF); validaRep/mappedrangeantesdecopiar<=96Bdebasename/path.
Até512eventosnome/skip e mudanças de enableLowMemoryPhone, cujoELFdeclara4B
GLOBALc51a64. Lerflag emruntime, não assumirBSSinicial. Contadores/últimaflag
atômicos para admitir chamadasporworkerssem corrida introduzida pelo diagnóstico.
Removidaconsulta redundante FindMemBlockByAddr com tamanho!=0 doguarda:
GetMemBlockInfoByRange/accessR/extent já validado172 permanece. Primeirobuild
9802D8...nãoinstalado, corrigidaatomicidadeantesdainstalação. Finalexit0
SHAC8DF89940DFB31FD579E0C810C60358107041711E8B68A3D1FE97C7B6735EF61,
archivebuild-175-texture-mip-policy. Instalado/lançado175; somente texturepolicyON,
outrosprobes/mesh/batchOFF, cityVBO/syncON, XML124. Resultado pendente.
Próximo: identificar mips realmente escolhidosparaasfachadas, testar sómudança
comproporcionalidadevisual/memória, sem relaxar ownershipnem reduziqualidade.

175 resultado: low_memory_phone=0 foi confirmado durante boot/carga. Menu_font_sm,
efeitos e building_port_low retornaram skip2; building_port_02_lod retornou0.
A quota única de512 eventos esgotou em assets de UI antes de ver Map__/CityAtlas.
A ausência desses nomes não foi interpretada como caminho inativo. App fechado;
log tasm_175_mip_policy_quota_final.log,167KiB. O script somente de leitura
inspect_mip_policy_strings175.py valida SHA/PT_LOAD e resolve literais PC-relative:
vehicle e a grafia vheicle usam VehicleSkip; _lod vem antes de building_ e usa
HighLodSkip; building_ usa BuildingSkip, prop_ usa PropSkip. Resultado completo
em mip-policy-all-strings-175.json. Nenhuma alteração de XML foi feita ainda.

176 separa a quota:32 amostras gerais e256 dedicadas a Map__/CityAtlas/building_/
spiderman_. Chamadas e resultados nativos permanecem iguais. Build exit0,
SHA14220706B5947B645698391B5F617F81049EA885AC457AB82FBA79D0399287D5,
archive build-176-mip-world-quota, instalado/lançado com os mesmos XML/marcadores.
Dois patches de documentação falharam por contexto e foram refeitos; não houve
alteração parcial. O updater sincronizou1166 evidências antes do resultado175;
a skill passou na validação. Próximo: controle176 e177 mudando somente
BuildingMipmapSkip1->0, com confirmação visual, memória e FPS. Ainda não implantado.

176 controle coletado: CityAtlas1/2/6/9/10 skip0, mas building_retro_08_wall,
building_retro_05_{door,windows}_01, building_modern_windows_03,
building_abcstudios_{01,02,05}_d, building_squaretower_01 e outros retornaram2.
LowMemoryPhone continuou0. A quota por família finalmente alcançou o mundo.
Captura fresca confirmou Lv2/pose padrão. Métricas irreversíveis de três regiões
salvas em frame176-facade-metrics.json; o SHA da fachada esquerda continua igual
ao161/162. Não é comparação da imagem inteira nem prova de qualidade de todos
os assets. analyze_facade_regions.py gerou aviso Pillow sobre getdata deprecated;
o resultado foi produzido e não foram guardados recortes/BMPs permanentes.
Snapshot player=(-24835.5,9141.7,-1047.7), memória depois da captura excluída deFPS:
RAM4045KiB/VRAM938KiB/PHY1023KiB, newlib usado66731KiB/freeBlk5424KiB (não toda
a capacidade restante). Log estável tasm_176_mip_world_final.log.

177 experimento somente XML, mesmo VPK176 SHA142207...: helper alterou apenas
Texture.BuildingMipmapSkip1->0. Arquivo config_building0_177.xml,
SHA12069c940bd17282ce3121ecf11dfece45ac057dcf497a4cb68ed192013ddb05.
Config remoto anterior preservado em config_before_building177.xml e SHA08A1EE...
1501C conferido; saves reais preservados em save-176-before-quality-test.
App fechado antes da troca; push e readback confirmaram exatamente os bytes177.
Marcador texture_policyON para demonstrar o retorno nativo; demais probesOFF,
cityVBO/syncON, câmera133/clock444. Lançado177, resultados ainda pendentes.
Não aceitar mais resolução sem conferir memória/carga/FPS e sem percurso ativo.

177 resultado visual: mesmas texturas de fachada retornaram skip0, com
LowMemoryPhone0. A grade de janelas próxima ficou visivelmente mais definida.
Métricas da região esquerda: borda horizontal3.149->5.437, vertical0.999->1.995;
são métricas locais de uma cena, não certificação de todos os assets. Imagens
temporárias removidas; frame176/177-facade-metrics.json guardam só hashes/métricas.
Mesma posição (-24835.5,9141.7,-1047.7). Parser analyze_mip_fps177.py excluiu
início/captura por frame>=2800, uploads e maxframe>=100ms, draws39..41/f:
controle176 mediana37.17FPS(n10),17737.27(n7), dentro da variação. Considerar
também janela adjacente a capturas: latência pode ser publicada no próximo
latido. Não usar janela com captura em curso mesmo se maxframe ainda for baixo.
Memória177 antes de percorrer: RAM278KiB/VRAM619KiB/PHY340KiB, newlib70065KiB,
contra1764045/938/1023 e66731KiB. Maior uso, sem falha observada.

177 percurso: primeira série~12s deCross+forward, depois pouso em outro telhado.
GPUcity continuou0/6400 páginas diferentes, errors0. O FPS12.81 de uma janela
que terminou durante captura é INVÁLIDO para desempenho do percurso. Muitos
frames foram Pause durante análise, não teste ativo prolongado.
Segundo percurso cronometrado, SEM captura: time_us1051802275/frame50810 até
1071435616/frame51198,388frames/19.633341s(~19.76FPS), contador de capturas3
antes/depois. Posição (-27099.4,5095.6,-5680.3)->(-2809.0,3204.0,-21088.4).
Janelas ativas completas~23.43/21.66/16.25FPS com streaming real; não excluir
uploads normais para disfarçar stutters. Dados travel177-timing.json e log
tasm_177_continuous_travel.log. Depois pousou na rua (captura fresca) e Pause.
Snapshot final emPause é antigo/agealto; não confundir com posição atual lida
ao vivo. Novo city_shadow0/6400. Newlib64991KiB/VRAM1378KiB, sem crescimento
monotônico nesses pontos. Ainda não é teste de toda a cidade/30min ativo.

177 perfil na rua: native_profileON por comando11, sem amostragem de estados.
Trecho17s sem capturas manteve~18–19.6FPS. Janelas estáveis: WorldUpdate~26ms/f,
WorldRender~17ms/f, MainCharacterUpdate~11ms/f; física de todos~1.6ms/f, I/O
principal22–38ms/5s, city-sync0. Tempos inclusivos, não somar pais e filhos.
Logo disco não foi o gargalo principal nesse trecho. native_profileOFF comando13.
BMPs apagados; app fechado e log estável tasm_177_building0_final.log666KiB.
Saves reais preservados em save-177-before-dispatch-probe. BuildingSkip0 continua
candidato preferido após melhora visual, FPS estável na pose e percurso semerro.

178 REA: MainCharacter::Update ev_4d6ac5ad53668a5cfaf0b817a84dd2c117f2043b31b5e8128868f0d9f21f2e29,
919 linhas. Nomes STL/acosf+offset eram símbolos mais próximos, não identidade
dos helpers. Contagem de blxregister0 no script NÃO exclui chamadas virtuais
MOVLR,PC/LDRPC. CGameObject::Update em37ba9c, evidência
ev_e34d30c675e2011783154da2c69284306495f43f771ad7dcd777a183691c6621,
tem actor-vtable+A0, actor+74(this,delta,force1) e componentes+10(this,delta).
Loop de componentes usa vector emself+124/+128 e r4 contém parent vivo.

Novo actor_dispatch.enable (default ausente) mede essas três chamadas por5s,
somente personagem principal/vtablec13e28/mainthread. Todas as chamadas originais
continuam uma vez. Confere todos os três pares MOVLR,PC/LDRPC antes de editar.
Veneers ARM carregam alvo original emr3; componente encaminha parentr4 emr2
somente à wrapper, que chama alvo original com dois argumentos. Sync mantém
force original; LR volta a site+8. Nenhuma vtable/objeto é alterada. Checagem
offline actor178-bridge-check.bin/txt confirmou literais/retornos. Amostragem
explícita actor_profile sóHUD; epoch e active aparecem em state.json para evitar
wait antigo. Relatório na main, adiado até não haver chamada amostrada em voo.
32 contextos guardam apenas function/vtable/kind/calls/us; sem patch de gameplay.
Builds intermediários não instalados; final exit0
SHA75F1E9F592F272C84EA140A9ADEF44476546AB113942AA4B3794C4015DC54121,
archivebuild-178-actor-dispatch-profile. Instalado/lançado178, XML177/Building0,
texturepolicyON e actorDispatchON; outrosprobes/mesh/batchOFF; native timingOFF
por default. Próximo: validar passthrough e amostrar telhado/rua antes de otimizar.
178 hardware: boot/MainMenu/HUD/Lv2 confirmados. Quatro capturas não foram
necessárias: foram três, todas removidas após inspeção. A primeira mostrou
fachada ainda em resolução baixa; a segunda, sem deslocamento, mostrou a grade
nítida. É evidência de carregamento tardio ainda aberto, não correção completa.
actor_profile epoch1: 190 atualizações/5s; física69778us, sync29743us,
UpdateEnemy4798us, combate1608us, blend590us; total~0.56ms/atualização.
Percurso até rua: seis pulsos Cross com forward, câmera700ms e queda8s.
O primeiro envio foi rejeitado integralmente por sintaxe dos sticks; usar
press left-stick/right-stick X Y e release, não left/right X Y. Refeito.
Imagem fresca confirmou rua/NPCs/Lv2/HP; três capturas no total, não quatro.
epoch2:127 atualizações, física46216us/sync20229/Enemy3341/combate699/blend386;
novamente~0.56ms/atualização. Sem dropped. Chamadas originais preservadas.
Perfil nativo nesta OUTRA rua (~28FPS) mediu MainCharacter397ms/144 (~2.76ms),
WorldUpdate2059/144 (~14.3ms), Render2161/144 (~15ms). Não comparar como ganho
com os18–19FPS de177: posições/contextos diferentes. O custo restante do
personagem está fora das três chamadas. Native timing desligado7, Pause e
close; log estável tasm_178_actor_final.log. Nenhum crash observado neste boot.

179 diagnóstico: adicionada a quarta chamada virtual interna, em3b9f4c, dentro
de MainCharacterUpdate: objeto this+38c, slot4C, flag original0, personagem vivo
emr4. Mesmo veneer ARM do componente, parent extra sópara wrapper, original
recebe this/flag. Verifica os quatro pares de opcodes antes de alterar qualquer
um. Atribuição ainda sóao personagem principal/main; sem mudar gameplay.
O helpera83324 observado é incremento atômico por kusercmpxchg, não divisão
nem mutex por causa do nome do símbolo mais próximo. Sem mudança de atomics.
Build exit0 SHA CBC335F0C330B29EC0B9872BF9150B0E5FFB162525A1C44ADFE95D07B5F08B20.
Resultado de hardware179 pendente. XML177 e demais políticas preservados.
179 hardware: primeiro wait MainMenu40s expirou durante boot; logs ainda
avançavam. Novo wait encontrou MainMenu com frame2885/time94.8s; esse é tempo
até a consulta, não duração exata do boot. Sem outro launch/input durante timeout.
Lv2/HUD carregou. Epoch1 ainda teve uploads de texturas, não é FPS estável.
Quarta chamada (9859e354/vtable98c18298)159us/144 no telhado,182us/140 no
percurso final: ~1–2us/chamada, não explica a lentidão. Física nesta outra pose
399156us/140 (~2.85ms), demonstrando dependência do local. Não comparar como
regressão com178 sem mesma pose. Captura3 foi por engano emPause; refeita4HUD,
inspecionadas e ambas removidas. Imagem fresca mostrou personagem transparente
com câmera muito perto da parede, caso ainda aberto. Sem crash observado.
Log fechado tasm_179_scene_final.log. Skill atualizada e validada (1221evidências).

180 diagnóstico: REA analisou oito helpers nomeados chamados por MainUpdate;
rea-actor-helper180-0..7.json guardam evidências completas. SwingReward é apenas
BX LR e não foi patchado. Sete prologues restantes (14palavras) não têm referência
PC e foram conferidos antes de qualquer hook. Trampolins executam os originais
uma vez e preservam até3 argumentos inteiros/ponteiros. Mede UpdateEffect,
LiftingCar, Challenge, HistoryRegular, GPSPointer, CBelt e PointLastPosition.
Amostra sódentro da chamada viva do personagem principal, somente mainthread;
contexto anterior é restaurado, relatório espera chamadas emvoo e fim da chamada
externa, proteção contra reentrada de carga. Metadata kind4/function, sem assets.
A primeira compilação DFBC... não instalada; revisão evitou leitura do contexto
emworker e custo de thread-query emhelpers com amostragemOFF.
Final exit0 SHA54C7316AA754DFEBD52801E238DF8E838D7A4A6BCBC652553CA7D83CA2869C3A.
Archivebuild-180-character-helper-profile; hardware pendente, sem otimização de
lógica adotada nesta etapa.
180 hardware: MainMenu47.36s no state, não duração atéessa consulta apenas81f;
Lv2/HUD e todos os sete hooks confirmados. Primeira amostra epoch1 coincidiu
com carga (só28updates/5s); não serve para FPS. Epoch2 estabilizado193updates:
UpdateEffect329520us (~1.707ms/update), física71548us/sync31079, histórico8318,
Enemy4344, combate1556; demaishelpers pequenos. CBelt772calls/588us. Nenhum
PointLastPosition chamado nessa pose (ausência de amostra, não custo zero geral).
Epoch3 após percurso:145updates, Effect72538us(~0.50ms), física52361us(~0.36ms).
Logo o custo varia muito com local; não extrapolar os11ms de177 a todas as ruas.
Log fechado tasm_180_helpers_final.log. Sem crash observado nesse boot.

181 REA: UpdateBrightness ev_6766e67f01de731693c478340ac1991e6d36b8732dbcbb0b3901ad04d78c25cb,
chamado por UpdateEffect, sómain character, lança raio na direção de luz e
interpola brightness+self158. FastCollideRayWithWorld ev_caa4eab691b5442f2365625823d3862b860b6ebea4a7546e1fd62d95e2f126c7
recebe this,line24B,quatroints (dois na pilha), retorna inteiro/pointer emr0;
brightness sócompara zero/nãozero em3844ac. O manager+4 é passado como saída
ao raycast; nenhuma otimização pode ser generalizada aos outros callers.
Novo módulo brightness_ray: opt-in brightness_ray_probe.enable, prologue e
BL/CMP do caller conferidos. Todas as outras chamadas/threads passam originais
uma vez, seis argumentos. Cache começaOFF; primeiro medir calls/tempo isolado.
Experimento adicional implementado mas NÃOadotado: cache de apenas iluminação,
chave exata do manager+24bytes de linha+4ints, TTL100ms após cálculo, mudançade
qualquer byte força original. Pode atrasar reação da iluminação a geometria
que muda sem mudar o raio ematé100ms; não é equivalência perfeita. Não aplicar
em colisão de movimento/combate. Comandos brightness_cache_on/off e stats;
state mostra flag para A/B na mesma pose. Hardware e benefício ainda pendentes.
Build181 exit0 confirmado após session1105, SHA2DC157C031AF1CBF820B188C4A57B8B02F6B75FDD7A3DDA3510ACD4CAAAF11D3.
181 hardware: MainMenu apareceu antes de aceitarCross; primeiro waitHUD35s
expirou ainda emMainMenu. Sem enviar comandos dependentes; segundoCross com
menuestabilizado abriuLv2/HUD. Epoch1 coincidiu com carga (12updates), inválido
paraFPS. Brightness-ray realmente foi chamado:32originais/51641us (~1.61ms),
com cacheOFF. ARMwrapper compilada inspecionada embrightness181-wrapper-arm.txt;
recupera argumentos5/6 da pilha e os encaminha ao trampoline.
Captura1HUD confirmou fachada nítida e traje; removida de ambos os lados.
A/B sem imagens: OFF1frame6202/time181787218 atéON17036/203444436; ON1até7872/
225534577; OFF2começo7931/227102061 até8733/249065054; ON28762/249852603 até
9584/271796868. JSONs brightness181-state-ab/repeat preservam estados completos.
OFF1ainda35–36draws/f ehouveupload4096KiB emON1, seguido de40draws/f; não usar
comparação de primeirafase como ganho/regressão causal. Leituraactors.gla naquele
upload não comprova atraso de fachadas: pode ser outro ator/asset. Novos blocos
estáveisOFF3:817frames/22.45432s(~36.38FPS), ON3:831/22.065974s(~37.66FPS),
pequeno ganho local (~3.5%), com variação temporal. Cache foi exercitado:
contadores totais3711originais/2002reusos/3mudanças/628expirações, originais
5969361us. Não resolve streaming nem desempenho de travessia. Foi desligado14.
State13/14buttons65536 = SCE_CTRL_INTERCEPTED segundoSDK; inputStart não pausou
naquele instante. WaitPauseexpirou, novo stateHUD/buttons0 e novoStart confirmou
Pause. Não éVOLUP (00100000); nenhum volume alterado. Esse evento limita teste.
Log fechado tasm_181_brightness_closed.log; removidos markersbrightness/actor/
texturepolicy. Candidato de cache NÃOadotado por padrão, diferença de iluminação
potencial100ms e benefício restrito a raio estacionário. Cidade/câmera/XML intactos.

182: REA Task::Wait ev_413503cc5adfe55b3bc87cb4c5742645ba3ab3441f77afdfc59947387c8d519c
confirmou os dois callers de pthread_cond_timedwait em6575c8/657678. Calcula
segundos +ms/1000 e grava tv_usec+(ms%1000)*1000 diretamente na segunda palavra,
sem normalizar. A heurística herdada trata isso como ns quando muda o segundo e
pode também confundir deadlinesPOSIX válidos curtos. Novo cálculo explícito só
seleciona microssegundos pelos dois retornos verificados6575cc/65767c; demais
callers usam ns. Preserva clamps1ms/10s nesta etapa. Sem tocar sinalização/join.
Helper cond_deadline.h e teste de10limites passaram com GCC -Wall -Wextra -Werror:
1s emvárias frações, carryus, virada de segundo, passado, nsPOSIX parecidos e64bit.
Implementado opt-in cond_deadline_fix.enable, ainda AUSENTE para controle182.
Novo sync_probe também mede condtimedwait:24logs no máximo, contadores atômicos,
caller/thread/deadlinecalculado/kernelret/tempo. Log apósreobter mutex; diagnóstico
pode alterar timing. Não afirmar que causa pop-in sem observar chamadas reais.
Build182exit0 SHA AF62706D64278D097998CED3E3921730B6B9640EC30C7E561D1CEA9FF7BF5DD2.
Instalado182 com sync_probeON e fixOFF; hardwarependente. Remanescewarningmktemp
herdado, sem novowarning de prazo/ABI. Sem clock/plugin/firmware alterado.
182 hardware: waitMainMenu45s expirou com frame12, logscontinuaram; novo wait
achouMenu eCross abriuHUD/Lv2. sync_stats emMenu/HUD e após~27s percurso
confirmou ZEROcond_timedwait (e sem_wait/try/timed sem chamadas observadas).
Logo a falha aritmética latente não explica este atraso: fix permaneceuOFF,
nenhum teste183comfixON foi feito. Clocks de workers entreduas consultas:
main+43.07s, workerativo+8.56s, outros~0.90/0.85/0.02s. Um worker condsem
teve0CPU emambas; isso não comprova deadlock se não recebeu tarefa. Não concluir
starvation de uma fotografia BLOQUEADO. Log fechado tasm_182_cond_closed.log.
Marcador sync removido; demaisprobes actor/texture/brightness continuam ausentes.

183 investigação de LOD: REA processosgrid/Lod emrea-stream-process-*183.json;
griddoYield ev_e3ea8a882a46bf309518db33882feda0d2ac93d3b00723c0d98a9f96c3455bf3
retorna1constantemente, não é prova de quantas vezes o process cede nem pede
sched_yield importado. Não alterar para0 sem entender o contrato. WorldLoadScene
rea-world-load-stream-183 confirmou leitura das trêsdistâncias, vetor float
paraCLODStreaming e raiosadd/safe emgrid+b0/b4. Os nomes sozinhos não são unidades.
Scripts read-only inspect_world_literals183.py/inspect_stream_config_refs183.py
conferemSHA/PT_LOAD, decodificam literaisARM e guardam JSON; referências são
candidatas verificadas no disassembly correspondente, não decompilação de alto
nível. CameraNearDist é lido porCameraManager::Init/SetCameraNearValueToDefault;
CameraFarDist também porWorld::LoadLevel. Atributos podem separar passes de
câmera; não assumir nearclip de100m apenas pelo nome. Mudarconfig sóapósleitura
ativa nohardware. CampoXMLBuildingScale não teve referência neste parser; isso
não prova ausência absoluta de uso indireto.
Novocomando streaming_stats sólê seis exports estáticos int32 conferidos
readelf (far/mid/near/overlap/radius_add/radius_safe), semponteiros de cena.
VPK183SHA85475BB2328E82FF4091B1BCC55D93927D6443ACD7D24563D728803E398A6F44,
exit0; XML177 preservado para controle. Próximos: coletarvalores reais e planejar
uma variável de distância; nenhuma alteração deconfig183 aplicada ainda.
183 hardware: globals confirmados emMenu/HUD, far600000/mid10000; raios saíram
16000/17000emMenu para15000/18000emHUD conformeXML. Near foi impresso como
int1092616192: isso é float10.0 (41200000). OBJECT4 noELF não prova tipoC.
Snapshot apenasleu bits; não alterou o valor. Captura fresca de Lv2/fachada,
regiãoesquerda idêntica177/hash41156e...355aba. Métricas183 guardadas, BMPs
removidos. Logfechado tasm_183_baseline_closed.log. Saves econfig reais fechados
preservados em save-183-before-far-test; XMLSHA12069C...DDB05 verificado.

184: SetCameraRenderPass REA ev_0c07bcedf3ab55dd39cced3cbafe3634459538fbc5cd3cfac2f9b79cb0fea0d0
confirmou nearfloat, mid/farinteiros convertidosfloat e sobreposição; passa
perto/longe aoSetCamNear/SetCamFar. PortantoCameraNearDist é plano intermediário,
não nearclip10. Não assumir todos os consumidores/LOD sem medição. Novo logger
imprime near como float+bits, demaisint. O cache de iluminação continuaOFF.
Revisão do fixoptindecond também identificou domínio de relógio: TaskWait chama
gettimeofday importado (processotime emsys.c), mas pthr.c chamavaSDKwalltime.
Código local corrigiu também o domínio para os dois callers verificados; outros
callersPOSIX continuamrealtime. Isso está no184, OFFe NÃOexercitado pois zero
chamadas em182. Não chamar182 uma correção completa de clock; testes eram sóde
aritmética. Nenhuma prova de novo gargalo atual, condattrclock ainda não validado.
Build184exit0 SHA874DF8260E9A348B8976B11535444BABE8C3AA7B318FD85BB9F354B5DBFF2279.
Experimento únicoXML184:CameraFarDist600000->120000, helpersemanticdiff ehash
FE718585FF847DEB0B32BA6620546CB5D43576E218D361F2F882AD188ECD0FAB,
readbackexato; sem mexerMid/LOD/texturas. Instalado/lançado184, sem probesextras,
cityVBO/syncON. Benefício/qualidade ainda pendentes; reverter se a distância
cortar cenário visível ou não reduzir custo. FogEnd60000 é contexto, não prova
antecipada de equivalência visual. PlanoMidmaior ainda não executado.
Criado CURRENT_STATE.md para retomar rapidamente; updater agora o inclui antes
no registro15 completo. Todo histórico anterior preservado, sem sumir falhas.
184 resultado: globals confirmaramfar120000/mid10000/nearfloat10/radii15k/18k.
Captura1preservou regiãoesquerda idêntica183/177/hash41156e...355aba. FarTower
teve métricas diferentes e não se provou equivalência do horizonte. FPSroof
estável~38.66–38.90 com36draws/f, semelhanteao183. newlib68063KiB contra183
67898KiB (outrosassets/fragmentação podem mudar, não efeito isolado de far).
Percurso semcaptura start227520245/frame8568 até247600841/9033:465quadros/
20.080596s=23.1567FPS, dadosfar184-travel-timing.json. Captura2depois da medição
mostrou mergulho e horizonte com possíveis cortes; não certificá-lo semcontrole.
BMPs removidos. Log fechado tasm_184_far120000_closed.log. Request1timedout5s,
mas foi processado depois. Cross foi enviado antesdoACK: falha de orquestração
registrada, não perda do comando. Waits seguintes aguardaramACK; usar15s para
comandos quandoháI/O, não sobrescrever cmd.json nem avançar entradasdependentes.

185 controle: mesmoVPK184, XML177/far600000 restaurado e readbackSHA12069...DDB05.
Lv2/globals corretos, regiãoesquerda idêntica, farTowerdiferente porcontexto e
projeção. newlib73900KiB (atores extras/BREAKINGNEWS jáativos), portanto não
comparação de consumo exclusivo da distância. PrimeiroCircle dePause não foi
aceito comSCE_CTRL_INTERCEPTED; sequência dependente interrompida. State9depois
buttons0 confirmouPause; novoCircle abriuHUD e sóentão inicioupercurso.
Start249554236/frame9717 até269509006/10159:442quadros/19.95477s=22.1501FPS,
contadorescapture1/1 (far185-travel-timing.json). Captura2final mostrou outra
trajetória, perto de torreescura; não atribuir os~4.5% de diferença àfarplane.
Amostras iniciais de câmerae streaming não fixam totalmente rota/RNG/autofollow.
DistânciareduzidaNÃOadotada: sem ganhoconvincente e horizonte nãoequivalente.
XMLoriginal permanece, appfechado, logtasm_185_far600000_closed.log; BMPs removidos.

Próximo186: testar sóCameraNearDist10000->20000 (planointermediário), mantendo
far600000, demaisLOD/texturas/buffer25MiB e cityVBO/sync. Isso é hipótese de
detalhe maisdistante, nãofixadotado; medir imagem/FPS/memória, reverter se piorar.
Não aumentarBufferSize sem adaptar toda a políticaVBO/dirtybitmap de25MiB.
186 resultado: globalsfar600000/mid20000/near10/radii15k/18k, playerna mesma
posição inicial. Fachadaesquerda conservouhash41156e...355aba; maisgeometria
na parte próxima, mas ganho de nitidez distante não convincente nesta pose.
Draws51início e54–55/f depois, contra36–40; FPSestável34.9–35.2 vs~37–39.
newlib77505KiB/VRAM4119KiB. ComparaçãoCPU/GPUcity0/6400 em358msfora da medição.
SCE_CTRL_INTERCEPTED repetiu e impediuCircle; sequência de travessia abortou sem
entradasdependentes. launch_app(close_current:false,force:true) retomou o MESMO
processo: frame18080/time416224918/request6, sem reset; botão0. Não é novo boot.
NovoCircle abriuHUD epermitiu percurso:frame19727/time444820132 até20141/
465117885,414quadros/20.297753s=20.39635FPS, capturas1/1. Imagemfinalmostrou
TimesSquare/torredorelógio, rotadiferente doscontroles; não afirmar queda causal
exata deFPSdotrecho. Candidato186nãoadotado por custo parado e ganho incerto.
Appfechado, logtasm_186_mid20000_closed.log199KiB, BMPs removidos. XML177
restaurado/readbackSHA12069C...DDB05. RGcomglobna posiçãodepathfalhou efoi
refeito comdiretório+ -g '*.c'; pad/touch usamPeek, não bloqueiam esperandoamostra.

187 integração: detectadoque o perfil embutido ainda tinha Atlas/LowLod/Building
skip1 e distâncias antigas, embora ooverride177testado usasse outrosvalores.
Atualizado fallbackpara61propriedades semanticamente idênticas177; override do
usuário continua prioritário. check_default_profile187.py extrai stringC/parseXML,
confere duplicados/igualdade completa egera esperado2844B/SHA5B3CB9B93C2B3853443959F89694C5B5E61CA844D4B61FDF8E6A480349A81337.
CMakeclean default também divergiam do cachetestado: softwareETC1ON ecompaction/
tagOFF. NovosdefaultsOFF/ON/ON; configure fresco em_logs/cmake-default187 deu
flags exatamenteiguais ao buildhardware. Caches existentes preservamsuasopções.
Nenhum novo backend foi introduzido. Compile187exit0, VPKSHA24E3ABA646C637B73C97ADFE5AADF367AB4F67044EE55FF74B7E2C97CF15C0C9.
DocsREADME/INSTALL corrigidos: Crossincluimelee/contexto, CircleWebStrike, Square
webspray, TriangleDodge/contexto; Start109/Pause, Select108/Map. Shoulderactions
não recebem nomes nãoverificados. TextoSoftwareETC1foiatualizadopara2Dnativo.
BUILDING registra flags e limitação: cityVBO/sync ainda dependemmarkers/debug;
validação sem markers e release continua pendente, não alegarinstalação100%.
Próximohardware187: preservaroverride177 como backupremoto, removersóoativo,
abrir comfallback, conferirhashde logs/vita_config.xml, globals/Lv2/imagem/percurso.

187 hardware: sem override ativo, logs/vita_config.xml2844B bateu SHA5B3CB9...81337
do esperado. Lv2/fachada hash41156e...355aba persistiram; roof37,5–38,94FPS.
Override177 preservado como backup remoto vita_config_override.profile177.xml.
Logtasm_187_builtin_closed.log, BMPs apagados.

188: TASM_NATIVE_CITY_STREAMING defaultON independente deDEBUG. Predicado atômico
no boot reúne VBO/sync; disable markers native_city/native_city_vbo/native_city_sync
têm prioridade. Legacy enable requer par. Alloc/free verificados antes dafactory;
falha impede promoção25MiB. Tracking copy/I/O/zlib/arena/GL e dirtysync compila
semDEBUG; mesh/batch e probes resource/texture mantidos debug/optin. Check automático
25MiB só com city_shadow_boot_check.enable, ausente. VPKdebug188SHA
7E70224D05BC9258676F56B28C2F5336107CF800515FB360F129A0A6539E94E6.
Enable markers removidos. Bootdefault1/request1/hooks, HUD/Lv2/fachada iguais.
Roof36,33–38,99FPS; check0/6400 em358377us, errors0. Log188_markerless_closed189KiB,
BMPs removidos. Controle disable ainda não foi feito.

188/189 release: primeiro compile1889C5A1A...4471F2 mostrou declarações implícitas
resource/texture por escopoDEBUG expandido. Corrigido DEBUG interno; sócitycore
fora. Primeiro VPK temporário sobrescrito, apenas metadata preservada. Novo
TASM_DEV_HARNESS independente, default=${DEBUG_SOLOADER}. Comparador city retorna
-1indisponível/erro negativo,0igual,1divergência. JSON city_check_request/result
evita tratar ACK como sucesso. Estado1536B; bannerboot direto mesmo semINFO.
Primeiro link release+harness falhou: DMA benchmark só existia noDEBUG. Guardados
extern/dispatch; segundo compile temporário exit0 SHA90189C2B...3AB645, não instalado.
Root reconfigurado DEBUGOFF/nativeprofileOFF/harnessON/cityON, rebuildexit0,
VPK189final283DF9B0872293C58655645D8894B7481A25787849A43F3B90EBDF0F1D39BC06,
1462635B. ELF/fontes/cache/flags arquivados. nm confirmou corecity/harness presentes.
Warnings herdados mktemp/constvolatile separados. Não misturar diretórios/ELFs.

189 instalado/lançado. Retomada de contexto: cell608 não existia mais, state/log
diretos confirmaram bootfresh/MainMenu/buildNov5 01:41:07/frame14044/time275406136.
Cross180ms abriuHUD/Lv2. XMLgeradoSHA5B3...81337 igual esperado, sem override.
Request1city_shadow durante streaming retornou1; request2 após estabilizar
frame15531/time318764751 retornou0. Não omitir primeiro resultado: workersCPU e
dirtypages podem estar concorrentes/pendentes. Não prova falha persistente nem
tracking completo no percurso. Cobertura/pending ainda não publicados no quiet.
Capture3preservou fachada hash41156e...355aba; métricasframe189 eBMPs apagados.
Pause/retornoHUD funcionaram, neutrosem drag/delta. Ainda há perf/watchdog/heartbeat
sempre ativos. PERF_DEVFILES inclui comparação/captura, não só escrita deestado.
ABBAsemcapturas, mesma pose12–13s: ON37,6533/OFF38,2183/OFF38,2042/ON36,2670FPS.
quiet189-telemetry-abba.json comestados completos. Sugere custo deI/O com deriva,
não ganho exato de percurso; telemetry_pause ainda poll4Hz. DefaultDEBUG aindaON.
Leituras locais paths supostos glutil.c/BUILDING/ref14 falharam, corrigidas via
rg --files. Patch delete/add do mesmo arquivo em uma operação foi rejeitado antes
de alterar; refeito sequencial. Nenhum arquivo de jogo afetado por essas falhas.

189 complemento: percursosemcaptura comtelemetria pausada, poll4Hz mantido:
frame27271/time634350052 até27516/645192802,245quadros/10,84275s=22,59574FPS.
quiet189-travel-timing.json; captura posterior mostrou rua comfachadas presentes,
mas não certifica ausência de pop-in durante o movimento. Pause foi aplicado com
atraso após captura/I/O, confirmado no state seguinte antes de continuar. App
fechado; logtasm_189_quiet_closed.log120KiB, BMPs apagados. Saves reais fechados
login/glcloud/backup preservados em save-189-before-cpu-control, sem editar conteúdo.

190 controle CPU, MESMO VPK189/SHA283DF...BC06, apenas marker native_city.disable.
Bootnovo/contextoMCP eLv2/HUD. city_shadowrequest1=-1indisponível, como esperado.
RooftelemetryOFF:frame1869/time91173432 até2462/107299741,593/16,126309s=36,7722FPS.
Imagemfresca preservoufachada/hash41156e...355aba, perfil177 embutido. Não houve
ganho para substituir GPU. Logtasm_190_cpu_control_closed25KiB; appfechado, marker
removido, BMPs apagados. Artefatoscpu190-roof-timing/frame190-cpu-metrics. Sóesse
disable foi testado; os dois aliases de disable têm a mesma expressão no código.

REA189 estática: processosLOD305120 egrid306b90 extraídos viaCLI existente,
evidências ev_1a93e144d0b178551ca7f0d4a85ddabe9cfd3abbd15ac8cd5ea35da7e5ae78df
e ev_94341bfbb0ab5f8551fdb124f7e2c2b2dd25391accbec8b0e69c6137ab30b0b1.
Há chamadas virtuais de arquivo, allocadores e locks; não adicionar timing/yield
sem identificar interfaces/semântica. Nenhuma mudança nativa adotada dessas leituras.
RevisãoSHF: lock global abrangePread;188teve espera267/627/372ms porjanelas, mas
main_wait0/16/6ms nesses trechos, contra váriossegundos de leitura principal.
Não atribuir todo atraso ao lock. Aumentar buffer/prefetch ou liberarFD sem lease
traz tradeoffs e não foi implementado. Busca de headersSDKdecompress não achou
API nos paths filtrados; não prova inexistência nem autoriza trocar zlib.

191 mudança: DEBUG_SOLOADER defaultOFF, harness/nativeprofile herdamOFF emfresh
configure. Buildroot mantémharnessON explícito paraMCP. SemDEBUG, novo cadence
poll1s/state5s; DEBUG mantém250ms/1s. telemetry_fast/slow permitemABBA nomesexatos.
Menu novo e comando forçamestado; telemetry_pause ainda permiteACK/menu novo.
Salvar published_menu sóapósrename bemsucedido evita perder mudança se falhar.
JSON2048B publica dev_io_calls/us acumulados (exclui ciclo atual aindaemexecução),
cityrequested/active/pending, páginasdoúltimocheck e erros de sync totais que
o watchdog nãozera. Getterbarato não varrebitmap a cada frame; compareexplícito
calcula cobertura. Escritas concorrentes impedem transformar cobertura em prova
absoluta de memória estável. Sem mudar cópia, GPUwait, dirtysync ou regrasLOD.
Compileexit0, VPK1462646B/SHA9BB0178F8BA3D19830D59D161D6D957D81E07DB6212E7BC8ADFBB98D9A8A13AE,
arquivado build-191-slow-diagnostics. Instalado eMainMenubuildNov5 02:00:33.
Configurefreshcmake-default191 comprovouDEBUG/harness/nativeprofileOFF,
city/compaction/tagON/softwareETC1OFF. Variante semharness compilouexit0 emtemp,
SHA04C35645F7C87DBF0AC4FA9C5C45B0BC6A70FDB38AF636AB5457262338F96901,
arquivobuild-191-release-no-harness; NÃOinstalado. RootVPK9BB permanece. Warnings
herdados mktemp/constvolatile continuam, nenhum erro novo de link/declarações.

191hardware: GPUrestaurada semmarkers, XMLfallback mantido/Lv2/fachadaigual.
city_shadowrequest1 logoapósHUD:10páginasdiferentes/12marcadas, uncovered0,
pending1/errors0; request2 após estabilizar:0/6400/pending0/errors0. Estados
slow191-hardware-checks.json. Camposdifferent/marked/uncovered referem ao ÚLTIMO
check, pending élive; não misturar epochs. Resultado1 pode refletir atualizações
jáobservadas ainda pendentes; nãofoi mascarado paraPASS.
ABBAfast/slow/slow/fast semcapturas/checks, mesmo191/pose:
36,91508 /38,07193 /38,08732 /36,63001FPS; I/O26,153 /6,912 /7,011 /26,297ms/s.
Redução~73%deI/O diagnóstico e ganho local~3,6%; sem garantia emtodojogo.
slow191-telemetry-abba.jsoncomestados. Restauradotelemetry_slow request11.
Capture12roofconfirmouLv2/hash41156e...355aba; foi lenta(~14s), totalmente fora
doABBA. Ambasjanelas/adjacentes excluídas antesde percurso. BMP removido.
Percurso1Cross+forward10s,frame9963/time306337425 até10199/317606497:
236/11,269072s=20,94227FPS. Confiraestados slow191-travel-timing.json para valores
exatos, trajetórias/RNGnãofixos; não atribuir diferença de189 sóao cadence.
Request15apóspercurso:200diferentes/218marcadas/uncovered0/pending1/errors0;
repetição16apósestabilizar zerou. Importante: principal gargalo permanece.

Teste17câmera foienviado apesar de state16játerbuttons65536, errodeorquestração:
raw234recebido, mas right_active/down0; NÃOvale como validação. Releaseemfinally
executou. Retomado MESMOprocesso via launch(close_current:false,force:true),
frame/time/requestcontinuaram. Novo state19HUD/buttons0 antes deinjetar.
Teste20válido raw235/141:right_active/down1/dx6/dy0; release21raw108/141/down0/
delta0. Nãohá camera_sensitivity.cfg na listagem126linhas; defaultsH.65/V.75.
Xperia divideYpor2; fator-2noenvio compensa isso, não éduplicação arbitrária de
sensibilidade; não removê-lo baseado sóna leitura isolada do helper.
Novo percurso2 (guardHUD/INTERCEPTED explícito) com pan700ms, forward e6Cross
2scom250msrelease:322frames/15,398596s=20,9110FPS. slow191-travel2-timing.json.
Request24checkzerou/pending0/errors0. Capture25mostrouSpidey juntoafachadadevidro,
visível, semcertificar todo clipping; statePause confirmado, métricasguardadas,
BMPs apagados. Logtasm_191_slow_travel.log87KiB; sem sessão30minATIVA certificada.
Os links6/7/11 semfragmentshader repetem no boot; próximoREA deve conferir o
fallback nativo antesde chamá-los decausa gráfica. Nãohouve fix disso ainda.

192/193: comparação188DEBUG mostrou relinks6/7/11 válidos na carga de cidade,
enquanto189–191quiet falharam. NÃOaceitar quiet como release sópela fachadaigual.
LeituraARMno4b3ca0 confirmou glLink→GL_LINK_STATUS; falha retorna0 e construtores
4b38a4/4b3984 fazemcleanup/glDeleteProgram ezeramobjeto+3c. Isso prova tratamento
deerro, não um fallbackmaterial válido. RótuloSTLnearest não é identidade real.
Busca completa não achou chamada glGetAttachedShaders@plt noSO alvo; stubherdado
comcommentKRM não é prova de seu uso. Não o trocaràscegas para resolver esse caso.

192instrumentação optin TASM_DEV_HARNESS: ringestático4096x32B, semalocação nem
I/O por evento após leituraúnica do marker. Wrappers createShader/createProgram/
attachShader eeventos nos wrappers jáexistentes de delete/link. Registra tipo,
IDs,frame,TID eattachments via querydireta max2. CSV sóem comando explícito,
JSONshader_trace_request/result/events separaACK/despejo. Efeitos de consulta/
layout/tempo ainda podem mudar reprodução; isso NÃOéfix do programa ausente.
Compile192exit0 SHA E4B11C052A2E007E715672C6A39BCA9C8480183B9134B386018649437B296C1D,
1463821B, arquivo build-192-shader-lifecycle-quiet.191fechado/log167KiB preservado.
Marker shader_lifecycle.enable enviado após fechar; instalado192, MainMenu.
Comando1dumpmenu445eventos, comando2HUD531eventos; ambosresult0. CSVs
shader192-menu.csv e shader192-hud.csv.123shadercreates/79programcreates/
13programdeletes/158attaches/79linksbefore/79after, ZEROshaderdelete nativo.
TodosTID40010003. Program6(V8/F10),7(V8/F11),11(V8/F16) passaraminício/recriação;
asdeleções foramframe20e reconstruções2531. Todos79linksresult1. Nenhum efeito
visualperfeito inferido; tracealtera layout/tempos efoidumpadolentamente foraFPS.
Appfechado, logtasm_192_trace_closed44KiB. Markerremovido. Boot193 éoMESMOVPK,
shader_events0 confirmoutraceOFF; MainMenu→HUDframe3503/time103644097. Log193
coletado não mostroumissing/LINKFAILED. vita_pull reportouEEXIST apesar de arquivo
localdisponível: conferir bytes/banner; não repetir entrada nem trocarbinário.
Este resultado enfraquece atribuição exclusiva a DEBUG e exigeisolar wrappers.
Próximopasso193/194planejado: restaurarbindings create/attachdiretos mantendo
demaistrace/layout, novoSHA/ELF, medirreprodução. Ainda não executado.
Registro191percurso1 continhanúmeros de fronteira digitados incorretamente;
corrigidos a partir doJSON:9963/306337425→10199/317606497. Diferenças/FPSjáeram
corretos; artefato éautoridade. Não inventar fronteiras ausentes emresumo.

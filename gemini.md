# Documentação da Arquitetura: Metal Slug - Demake

Este documento descreve a arquitetura do projeto após as refatorações guiadas por princípios de **Clean Code**, com o objetivo principal de facilitar a escalabilidade de novas fases e realizar o port para a placa FPGA **DE1-SoC (ARM v7, Linux)**.

---

## 1. Princípios de Clean Code Aplicados

Para garantir que o código seja legível, modular e escalável, dividimos um monolito (o antigo `main.c`) em múltiplos módulos especialistas (Princípio da Responsabilidade Única - SRP).

- **`src/main.c`**: Atua apenas como orquestrador inicial. Não possui implementação de física ou de desenhos. Apenas inicia a tela, o jogador, e invoca o loop da fase atual.
- **`src/jogador.c` e `include/personagem/jogador.h`**: Encapsula tudo que diz respeito ao MegaMan. A física de plataforma, recebimento de dano, pulos, tiros, estados e leitura isolada de botões são tratados exclusivamente aqui.
- **`src/ui.c` e `include/ui/ui.h`**: Módulo responsável unicamente pela HUD. Contém as primitivas de renderização em tela e métodos especializados (ex: `desenhar_vida`, `desenhar_barras`, ícones flutuantes e números).
- **`src/fase1_logic.c`**: O loop da fase 1. Guarda toda a lógica de spawn de inimigos (soldados com paraquedas), checagem de finalização (matar 50 inimigos) e background.
- **`src/fase2_logic.c`**: O loop da fase 2. Spawn de Spider Jockeys (ver seção 4). Mira em ciano para diferenciar da fase 1. Meta: matar 20 Spider Jockeys.
- **`src/fase3_logic.c`**: Fase 3 (tank boss). Ainda a ser implementada.

---

## 2. Escalabilidade: Sistema de Fases

O projeto foi configurado para suportar facilmente até 3 (ou infinitas) fases:

1. **Separação de Contexto**: Cada fase tem seu arquivo próprio (ex: `fase1_logic.c`, `fase2_logic.c`).
2. **Continuidade de Estado**: O loop de fase é chamado a partir do `main.c` passando o ponteiro da struct `jogador_t` (`rodar_fase_1(&jogador);`).
3. **Preservação**: Como o ponteiro do jogador trafega de uma fase para outra, todos os atributos sensíveis como Vida, Munição Normal (e seu superaquecimento) e Tiros Carregados (espingarda) são **automaticamente** levados da Fase 1 para a Fase 2.

---

## 3. Port para DE1-SoC (VGA, Displays de 7-Seg e LEDs)

Para facilitar a integração com os Periféricos Memory-Mapped da DE1-SoC (FPGA + Processador HPS), foram criadas interfaces que abstêm a engine do hardware:

### A) Variáveis de Comunicação com Hardware (`hardware_state.h`)
O arquivo `include/hardware/hardware_state.h` possui globais dedicadas:
- `hw_display_inimigos_restantes`: Deve ser conectado no driver dos displays de 7-segmentos, enviando a contagem de quantos inimigos faltam para finalizar a fase atual.
- `hw_leds_vida_personagem`: Recebe valores de 0 a 10. Pode ser capturado diretamente para ligar 10 LEDs dispostos fisicamente na DE1-SoC (LEDs acesos = barra de vida).

### B) Transição do SDL para VGA (DE1-SoC)
O sistema gráfico está abstraído em `framebuffer.h`. A transição entre desenvolvimento no PC e execução na Placa exige apenas **MUDAR UMA FLAG**:
- Vá no `CMakeLists.txt` e mude `option(USE_SDL_BACKEND ... ON)` para `OFF`.
- Ao fazer isso, o compilador ignora a biblioteca do PC (`framebuffer_sdl.c`) e compila o driver cru: `framebuffer_de1soc.c`.
- **Renderização VGA**: No `framebuffer_de1soc.c`, os frames são despejados via `/dev/mem` (Mapeamento de Memória) no endereço físico `0x08000000` (Padrão para Pixel Buffer no Linux HPS).

### C) Input: Mouse e Teclado via USB
Dentro de `framebuffer_de1soc.c`, as funções de controle do jogo solicitam o estado do teclado (`fb_key_down`) e do mouse (`fb_mouse_pos`). Para injetar movimentos físicos no Linux Embarcado, o sistema deve ser configurado assim:

1. **PThread Background Worker**: Na inicialização do jogo para DE1 (`fb_init`), levante uma *thread* separada em C (`pthread_create`).
2. **Eventos USB Linux**: Esta thread faz a leitura constante bloqueante nos dispositivos Character:
   - **Teclado**: Leia `/dev/input/eventX` extraindo dados na formatação padrão da `struct input_event` (`<linux/input.h>`).
   - **Mouse**: Leia `/dev/input/mice` capturando blocos de 3 bytes, atualizando deltas locais (x, y e estado dos botões).
3. **Ponte de Dados**: A thread salvará os estados decodificados em variáveis estáticas isoladas. As funções que a Engine chama (`fb_mouse_pos` e `fb_key_down`) apenas retornarão instantaneamente os valores destas variáveis assíncronas.

---

## 4. Novos Inimigos: Sistema de Sprites e Conversão

### A) Pipeline de Conversão de Sprites (`novos/convert_to_rgb565.py`)

O script suporta dois modos:

- **MODO 1 — CENÁRIOS** (`SCENARIOS`): Converte backgrounds com múltiplas camadas (bg, fg, colisão), zoom e crop. Formato de entrada: `AARRGGBB`.
- **MODO 2 — SPRITES SIMPLES** (`SPRITES`): Converte sprites de personagens/inimigos sem redimensionamento. **Formato de entrada: `RRGGBBAA`** (padrão do Piskel para sprites — diferente dos backgrounds!). Usa `rrggbbaa_to_rgb565()` e `sprite_is_transparent()`.

> **ATENÇÃO**: O formato Piskel para sprites é `RRGGBBAA` (alpha no byte menos significativo). Confundir com `AARRGGBB` inverte os canais R e B, fazendo elementos vermelhos aparecerem azuis.

Após rodar o script, os `_CNV.c` gerados devem ser **copiados como `.h`** para `include/inimigos/<nome>/`. Após copiar, **reaplicar manualmente**:
1. Include guards (`#ifndef ... #define ... #endif`)
2. Prefixo `static` no array `sprite_frame_t`

### B) Spider Jockey (Fase 2) — `inimigos/spider_jockey/spider_jockey.c`

**Arquivos criados:**
- `include/inimigos/spider_jockey/spider_jockey.h` — struct e API pública
- `inimigos/spider_jockey/spider_jockey.c` — lógica completa
- `include/inimigos/spider_jockey/spider_jockey_anm.h` — frames do esqueleto (58×50, 3 frames)
- `include/inimigos/spider_jockey/spider_jockey_arrow.h` — frame da flecha (13×9, 1 frame)
- `include/inimigos/spider_jockey/spider_jockey_sprites.h` — umbrella header

**Layout de frames:**
- Frames 0–1: idle e andar (passados com `frame_count=2`)
- Frame 2: ataque (passado como `&spider_jockey_frames[2]` com `count=1`)

**Comportamento:**
- Velocidade: 3 px/frame (3× o soldado)
- Dispara flechas parabólicas **tanto no chão quanto caindo de paraquedas**
- Pool de 8 flechas próprias por inimigo (`sj_flecha_t flechas[SJ_FLECHAS_MAX]`)
- Morte: sprite com tint vermelho (sem explosão separada)

**Física das flechas (balística sem resistência do ar):**
- Tempo de voo fixo `T = 25 frames`, gravidade `g = 0.25 px/frame²`
- Ângulo resultante: ~21° para alvo a 200px horizontal na mesma altura
- Fórmula: `vx0 = dx/T`, `vy0 = (dy - 0.5*g*T²)/T`
- Dano proporcional à velocidade relativa herói-flecha (mecânica Minecraft): mínimo 1, máximo 3

**Constantes ajustáveis (em `spider_jockey.h`):**
```c
#define SJ_VELOCIDADE          3
#define SJ_ALCANCE_TIRO        400
#define SJ_INTERVALO_TIRO      130   // ~2s a 60fps
#define SJ_FLECHA_TEMPO_VOO    25
#define SJ_FLECHA_GRAVIDADE    0.25f
#define SJ_FLECHA_VIDA         80
```

### C) Assets de Sprite Prontos (em `novos/`)

| Arquivo CNV gerado | Prefix C | Dimensão | Frames |
|---|---|---|---|
| `spider_jockey_CNV.c` → `spider_jockey_anm.h` | `spider_jockey` | 58×50 | 3 |
| `spider_jockey_arrow_CNV.c` → `spider_jockey_arrow.h` | `spider_jockey_arrow` | 13×9 | 1 |
| `goomba_CNV.c` | `goomba` | 18×18 | 5 (0-3=walk, 4=stomped) |
| `tank_idle_CNV.c` | `tank_idle` | 76×87 | 3 |
| `tank_death_CNV.c` | `tank_death` | 76×87 | 1 |
| `tank_shooting_CNV.c` | `tank_shooting` | 79×87 | 11 (9-10=fogo) |

---

## 5. Roadmap de Implementação

### ✅ Concluído
- [x] Spider Jockey completo (código + sprites + integração na Fase 2)
- [x] Converter de sprites corrigido (formato RRGGBBAA)
- [x] Todos os assets novos convertidos (goomba, tank idle/death/shooting)

### 🔲 Próximas Etapas (em ordem)
1. **Fase 1.5 — Copycat Boss**: Sala fechada (background cano Mario), 1 inimigo que espelha os movimentos do herói com delay, mesma skin com tint roxo, mesma vida do herói. Vencer = passar pelo cano para a Fase 2. Arquivos de arte: `novos/salaEspelhada.c`, `novos/bitmapEspelhada.png`.
2. **Fase 3 — Tank Boss**: Tanque atira projéteis a 45° (fogo nos frames 9-10 de `tank_shooting`). Invocar goombas (18×18, 5 frames). Vencer = pisar 20 vezes no goomba. Mecânica de morte do goomba: frame 4 quando pisado.
3. **Integração DE1-SoC**: Substituir `framebuffer_sdl.c` por `framebuffer_de1soc.c`, configurar pthreads para USB, conectar displays de 7-seg e LEDs.

---

## Resumo das Dependências
```text
[Hardware DE1-SoC] <=== Comunicação ===> [InterfaceFramebuffer]
       |                                           |
  (VGA / I/O USB)                                  |
                                           [Engine Principal]
                                           - src/main.c
                                           - src/jogador.c
                                           - src/ui.c
                                           - src/faseX_logic.c
                                           - inimigos/spider_jockey/spider_jockey.c
                                           (+ fase3 tank, fase1.5 copycat — pendente)
```

# Arquitetura

## Estrutura de pastas

```
.
├── CMakeLists.txt
├── CLAUDE.md
├── docs/                                  <- você está aqui
├── include/
│   ├── framebuffer/framebuffer.h          <- API de desenho/input (teclado + mouse)
│   ├── sprite/sprite.h                    <- tipo de frame + desenhar sprite
│   ├── animacao/animacao.h                <- máquina de animação (troca de frame no tempo)
│   ├── ataques/tiros/tiro.h               <- motor de projéteis (genérico, todo personagem usa)
│   └── personagem/achilles/movimentos/
│       ├── achilles.h                     <- inclui todos os estados abaixo
│       ├── achilles_idle.h
│       ├── achilles_andar.h
│       ├── achilles_pulo.h
│       ├── achilles_atirar.h
│       └── achilles_morrer.h
├── framebuffer/
│   ├── framebuffer_sdl.c                  <- backend de preview (SDL2, roda no PC)
│   └── framebuffer_de1soc.c               <- backend real (VGA da placa, só compila em Linux)
├── sprite/
│   └── sprite.c
├── animacao/
│   └── animacao.c
├── ataques/tiros/
│   └── tiro.c
└── src/
    ├── main.c                             <- ponto de entrada / lógica do jogo
    └── output/
```

Regra de organização: **cada módulo tem seu header dentro de `include/<nome-do-modulo>/`**, e seus `.c` numa pasta de mesmo nome na raiz do projeto. Ex: `framebuffer/` (código) ↔ `include/framebuffer/` (header). Personagens seguem o mesmo padrão, um nível abaixo: `include/personagem/<nome>/`.

## Por que existe uma abstração de framebuffer

O jogo vai rodar em dois lugares muito diferentes:
- **No PC**, durante o desenvolvimento: sem VGA de verdade, então usamos uma janela SDL2 só pra visualizar.
- **Na DE1-SoC**, no final: sem janela, sem sistema de janelas — os pixels são escritos direto na memória do controlador de vídeo VGA (320x240, RGB565, endereço físico `0x08000000`).

Pra ninguém precisar reescrever a lógica do jogo (movimento, colisão, personagens) quando trocar de ambiente, toda a parte de "desenhar pixel" e "ler input" fica isolada atrás de uma única interface: [`include/framebuffer/framebuffer.h`](../include/framebuffer/framebuffer.h). Em hardware, a base do VGA Pixel Buffer na DE1-SoC é acessada no endereço `0xC8000000` ou `0xC0000000` (quando Double Buffering está ativo).

### A API do framebuffer

```c
void fb_init(void);
void fb_shutdown(void);
void fb_clear(fb_color_t color);
void fb_put_pixel(int x, int y, fb_color_t color);
void fb_present(void);
int  fb_poll_quit(void);
int  fb_key_down(fb_key_t key);   // FB_KEY_UP/DOWN/LEFT/RIGHT/FIRE/JUMP/ACTION
void fb_mouse_pos(int *x, int *y); // posicao do mouse em coordenadas do framebuffer
fb_color_t fb_rgb(uint8_t r, uint8_t g, uint8_t b);  // monta RGB565
```

**Regra de ouro:** código de personagem/jogo só pode incluir `framebuffer.h` (e `sprite.h`/`animacao.h`, que também são backend-agnósticos). Nunca `#include <SDL2/SDL.h>` direto, nunca `mmap`/`/dev/mem` direto. Isso é o que garante que o mesmo `main.c` compila e funciona nos dois backends.

### Os dois backends do framebuffer

Selecionados em tempo de configuração do CMake, via `USE_SDL_BACKEND` (`ON` por padrão):

- **`framebuffer_sdl.c`** — cria uma janela SDL2 (320x240 escalado 3x), copia o buffer de pixels pra uma `SDL_Texture` a cada `fb_present()`, lê teclado (setas/WASD + Space/Shift/K) via `SDL_GetKeyboardState`, e o mouse via `SDL_GetMouseState` + `SDL_RenderWindowToLogical` (converte coordenada da janela, que está escalada 3x, pra coordenada real do framebuffer 320x240). `FB_KEY_FIRE` aceita `Space` **ou** o botão esquerdo do mouse.
- **`framebuffer_de1soc.c`** — abre `/dev/mem` e faz `mmap` no endereço físico `0xC8000000` (512x240 × 2 bytes = tamanho do buffer). Escreve os pixels direto ali. Lê entradas via USB (`/dev/input/eventX` e `/dev/input/mice`) usando pthreads que executam em paralelo no HPS sem travar o jogo. Além disso, mapeia a **Lightweight Bridge** (`0xFF200000`) para interagir com LEDs e Chaves. Só compila em Linux.

⚠️ O endereço `0xC8000000` difere do tradicional `0x08000000` pois no Linux, o HPS acessa os periféricos via a *Heavyweight Bridge* mapeada a partir de `0xC0000000`.

## Sprites e animação

Duas camadas pequenas em cima do framebuffer, ambas sem saber nada de SDL/hardware:

**`include/sprite/sprite.h`** — um frame de sprite é `{ width, height, pixels }` (pixels em RGB565). `sprite_draw(frame, x, y)` desenha pixel a pixel via `fb_put_pixel`, pulando qualquer pixel igual a `SPRITE_TRANSPARENT` (`0xF81F`) — é assim que o fundo "vazado" do sprite não aparece por cima do cenário.

**`include/animacao/animacao.h`** — `animacao_t` guarda um array de `sprite_frame_t` e troca de frame a cada N frames de jogo (`frames_per_sprite`). Duas modalidades:
- `loop = 1`: ao chegar no último frame, volta pro primeiro (idle, andar, atirar).
- `loop = 0`: trava no último frame ao terminar (morrer) — `animacao_terminou()` avisa quando isso acontece.

```c
animacao_iniciar(&anim, frames, frame_count, frames_per_sprite, loop);
animacao_atualizar(&anim);                         // chamar uma vez por frame de jogo
const sprite_frame_t *f = animacao_frame_atual(&anim);
sprite_draw(f, x, y);
```

## Dados de personagem

Cada estado de animação do Achilles vive no seu próprio header dentro de `include/personagem/achilles/movimentos/` (`achilles_idle.h`, `achilles_andar.h`, `achilles_pulo.h`, `achilles_atirar.h`, `achilles_morrer.h`), com os pixels crus (arrays `uint16_t`) e um array de `sprite_frame_t` + constante de contagem:

```c
static const sprite_frame_t achilles_idle_frames[]   = { ... };  // ACHILLES_IDLE_FRAME_COUNT   (6)
static const sprite_frame_t achilles_andar_frames[]  = { ... };  // ACHILLES_ANDAR_FRAME_COUNT  (12)
static const sprite_frame_t achilles_pulo_frames[]   = { ... };  // ACHILLES_PULO_FRAME_COUNT   (6)
static const sprite_frame_t achilles_atirar_frames[] = { ... };  // ACHILLES_ATIRAR_FRAME_COUNT (4)
static const sprite_frame_t achilles_morrer_frames[] = { ... };  // ACHILLES_MORRER_FRAME_COUNT (12)
```

`achilles.h` só agrupa os `#include` dos cinco. Todos fazem `#include "sprite/sprite.h"` em vez de redeclarar `sprite_frame_t`/`SPRITE_TRANSPARENT` — se surgir um personagem novo, o header dele deve fazer o mesmo (não duplicar esses tipos, senão dá erro de "conflicting types" ao compilar).

Ao adicionar/editar frames, vale conferir que `width × height` bate com o tamanho declarado do array de pixels — isso não dá erro de compilação (só `mmap`/pointer, sem bounds check), só corrompe a imagem ou lê memória fora do array. Um comando rápido pra checar todos de uma vez:
```bash
awk '
  match($0, /static const uint16_t ([a-z0-9_]+)_data\[([0-9]+)\]/, m) { sizes[m[1]] = m[2] }
  match($0, /\{\s*([0-9]+),\s*([0-9]+),\s*([a-z0-9_]+)_data\s*\}/, m2) {
    name=m2[3]; w=m2[1]; h=m2[2]; expect=w*h; actual=sizes[name]
    print name, (actual==expect ? "OK" : "MISMATCH " actual " vs " expect)
  }
' include/personagem/achilles/movimentos/achilles_*.h
```

## Sistema de tiro (`ataques/tiros/`)

Motor de projétil genérico — não é específico do Achilles, apesar do nome da pasta pai (`ataques/`) sugerir personagem. Qualquer personagem que atire reaproveita o mesmo motor.

**Pool fixo, sem alocação dinâmica:** `tiros_t` guarda um array de `TIRO_MAX` (16) `tiro_t`, cada um com uma flag `ativo`. `tiros_disparar` procura o primeiro slot livre e ativa; se não achar nenhum, o disparo é ignorado (sem crescer o array).

**Direção livre (mira pelo mouse):** cada `tiro_t` guarda um vetor normalizado `(dx, dy)` em vez de só esquerda/direita — dá pra atirar em qualquer um dos 360°. `main.c` calcula esse vetor todo frame a partir da posição do personagem até `fb_mouse_pos()`:
```c
float vx = (float)(mx - centro_x);
float vy = (float)(my - centro_y);
float dist = sqrtf(vx * vx + vy * vy);
float dx = (dist > 0.0f) ? vx / dist : 1.0f;
float dy = (dist > 0.0f) ? vy / dist : 0.0f;
```

**Formato/velocidade variam por personagem via `tipo_tiro_t`:**
```c
typedef struct {
    const sprite_frame_t *sprite; /* formato/desenho do tiro */
    int velocidade;
} tipo_tiro_t;
```
Em vez do motor desenhar um retângulo fixo, cada tiro aponta pra um `tipo_tiro_t` que dita seu sprite e velocidade — `tiros_desenhar` só chama `sprite_draw(t->tipo->sprite, t->x, t->y)`. Assim um personagem pode ter bala redonda, outro laser, etc., sem mudar o motor — só cria seu próprio `tipo_tiro_t` (ex: `tiro_achilles` em `main.c`, com um sprite placeholder 4x2 amarelo até ter pixel art de verdade).

**Cooldown de disparo:** `main.c` só chama `tiros_disparar` quando um contador (`cooldown_tiro`) chega a zero, resetando pra `TIRO_COOLDOWN` (15 frames) a cada tiro — sem isso, segurar o botão disparava um tiro por frame.

⚠️ **Sem hitbox/colisão ainda.** Existiu uma versão inicial (`hitbox.h`/`hitbox.c`) que foi deletada de propósito — o dono do projeto quer escrever a colisão na mão pra aprender. Não recriar isso sem pedir.

## Máquina de estados em `main.c`

`main.c` escolhe qual `animacao_t` tocar com base num `estado_t` bem simples. **Movimento (setas/WASD) roda sempre**, independente do estado — dá pra andar enquanto atira ou pula; só a *animação exibida* segue uma prioridade (não dá pra mostrar duas poses ao mesmo tempo):

| Estado | Quando entra | Sai quando |
|---|---|---|
| `ESTADO_IDLE` | nenhuma tecla de ação e sem movimento | anda, pula, atira ou morre |
| `ESTADO_ANDAR` | seta/WASD segurada, sem pular/atirar | solta a tecla |
| `ESTADO_PULAR` | `FB_KEY_JUMP` (Left Shift) segurada | solta a tecla — **só a animação, sem física de salto (gravidade/impulso) ainda** |
| `ESTADO_ATIRAR` | `FB_KEY_FIRE` (Space ou botão esquerdo do mouse) segurada | solta a tecla/botão |
| `ESTADO_MORRER` | `FB_KEY_ACTION` (K) apertada (borda de subida) | `FB_KEY_ACTION` apertada de novo (reinicia, só pra demonstrar — **não é lógica de dano real ainda**); enquanto morto, movimento é ignorado |

Prioridade da animação exibida quando várias teclas estão pressionadas ao mesmo tempo: morrer > atirar > pular > andar/idle. Atirar de fato (spawn do tiro) só acontece nesse estado, respeitando o cooldown.

## A pegadinha do `main` no Windows

```cmake
if(WIN32)
    target_compile_definitions(metalslug PRIVATE main=SDL_main)
    target_link_libraries(metalslug PRIVATE mingw32 SDL2main)
endif()
```

No Windows, o SDL2 precisa interceptar o `WinMain` do sistema — pra isso, o pré-processador troca todo `main` por `SDL_main` no seu código. Isso é **só do Windows**: no Linux (DE1-SoC) essa definição não existe, por isso está dentro de `if(WIN32)`. Sem esse guard, o link quebraria no Linux (procuraria por um `main` que não existe, já que foi renomeado).

Efeito prático: ao debugar com gdb no Windows, colocar breakpoint na função `main` por *nome* é ambíguo (existe um `main` de verdade vindo do CRT do MinGW, que só chama o nosso `SDL_main`). Prefira breakpoint por **linha do arquivo** (clicando na margem do editor).


# Portando para a DE1-SoC

## O que já está pronto pra isso

O jogo é C17 + SDL2 (no PC) por trás de uma abstração de framebuffer (veja [03-arquitetura.md](03-arquitetura.md)). Toda a lógica de personagens/jogo, por construção, não conhece SDL nem hardware — só chama `fb_*`. Isso significa que, em teoria, só o arquivo `framebuffer_de1soc.c` precisa mudar; o resto do jogo não.

## Caminho recomendado: compilar direto na placa

A DE1-SoC roda Linux de verdade no HPS (ARM Cortex-A9). **Não vale a pena fazer cross-compile pelo Windows** — é mais simples copiar o código pra placa (via rede/SD/USB) e compilar nativamente lá com `gcc`/`cmake` instalados via `apt`/`opkg`, e:

```bash
cmake -S . -B build -G Ninja -DUSE_SDL_BACKEND=OFF
cmake --build build
./build/metalslug
```

## O que muda de verdade

- **`fb_key_down`** no backend `de1soc` está stub (retorna sempre 0). Precisa ser implementado lendo os botões/switches físicos da placa (normalmente também via `/dev/mem`, endereço dos registradores de GPIO da FPGA — ainda não mapeado).
- **Span do `mmap`**: hoje está fixo em `320 * 240 * sizeof(fb_color_t)` bytes a partir de `0x08000000`. Confirmar com quem configurou a ponte FPGA↔HPS que esse é o tamanho certo antes de rodar na placa.
- **Performance**: Cortex-A9 é bem mais fraco que qualquer PC atual. Sprites grandes ou lógica pesada por frame podem não rodar a 60fps lá mesmo rodando liso no PC — testar na placa com frequência, não só perto do fim do projeto.
- **Input físico**: se o plano for usar botões/switches da placa em vez de teclado (o que é bem provável), isso é código novo dentro de `framebuffer_de1soc.c`, sem precisar tocar no resto do jogo.

## O que **não** precisa mudar

- Lógica de personagens, movimento, colisão, animação — desde que só usem `include/framebuffer.h`.
- `main.c` — só troca o backend linkado (`USE_SDL_BACKEND=OFF`), não o código.

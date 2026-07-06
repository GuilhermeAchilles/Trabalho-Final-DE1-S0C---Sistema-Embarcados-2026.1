# Documentação da Arquitetura: Metal Slug - Demake

Este documento descreve a arquitetura do projeto após as refatorações guiadas por princípios de **Clean Code**, com o objetivo principal de facilitar a escalabilidade de novas fases e realizar o port para a placa FPGA **DE1-SoC (ARM v7, Linux)**.

---

## 1. Princípios de Clean Code Aplicados

Para garantir que o código seja legível, modular e escalável, dividimos um monolito (o antigo `main.c`) em múltiplos módulos especialistas (Princípio da Responsabilidade Única - SRP).

- **`src/main.c`**: Atua apenas como orquestrador inicial. Não possui implementação de física ou de desenhos. Apenas inicia a tela, o jogador, e invoca o loop da fase atual.
- **`src/jogador.c` e `include/personagem/jogador.h`**: Encapsula tudo que diz respeito ao MegaMan. A física de plataforma, recebimento de dano, pulos, tiros, estados e leitura isolada de botões são tratados exclusivamente aqui.
- **`src/ui.c` e `include/ui/ui.h`**: Módulo responsável unicamente pela HUD. Contém as primitivas de renderização em tela e métodos especializados (ex: `desenhar_vida`, `desenhar_barras`, ícones flutuantes e números).
- **`src/fase1_logic.c`**: O loop da fase 1. Guarda toda a lógica de spawn de inimigos, checagem de finalização (matar 50 inimigos) e background. 

---

## 2. Escalabilidade: Sistema de Fases

O projeto foi configurado para suportar facilmente até 3 (ou infinitas) fases:

1. **Separação de Contexto**: Cada fase tem seu arquivo próprio (ex: `fase1_logic.c`, `fase2_logic.c`).
2. **Continuidade de Estado**: O loop de fase é chamado a partir do `main.c` passando o ponteiro da struct `jogador_t` (`rodar_fase_1(&jogador);`). 
3. **Preservação**: Como o ponteiro do jogador trafega de uma fase para outra, todos os atributos sensíveis como Vida, Munição Normal (e seu superaquecimento) e Tiros Carregados (espingarda) são **automaticamente** levados da Fase 1 para a Fase 2.

---

## 3. Port para DE1-SoC (VGA, Displays de 7-Seg e LEDs)

Para facilitar a integração com os Periféricos Memory-Mapped da DE1-SoC (FPGA + Processador HPS), foram criadas interfaces que abstenham a engine do hardware:

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
```

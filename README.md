# Metal Slug - Demake (MegaMan Edition)

Um "demake" inspirado em Metal Slug protagonizado pelo MegaMan, desenvolvido inteiramente em **C17** do zero (sem engines externas) com foco na renderização de buffers via CPU. 

O principal diferencial arquitetural deste projeto é o seu planejamento híbrido: o jogo roda tanto no **Windows (via SDL2 como mock para o framebuffer)**, quanto nativamente na placa **FPGA DE1-SoC (ARM v7, Linux Embarcado)** se comunicando diretamente com mapeamento de memória em registradores de Hardware!

---

## 🎮 Como Rodar o Projeto (PC / Windows)

Para compilar e rodar o jogo no seu computador, o projeto utiliza o **CMake** juntamente com o **MinGW64 / MSYS2**.

### Pré-requisitos
1. **MSYS2** instalado em `C:\msys64`
2. No terminal do MSYS2, instale o compilador C e o SDL2:
   ```bash
   pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-make mingw-w64-x86_64-SDL2
   ```

### Compilando
1. Abra um terminal no diretório do projeto.
2. Certifique-se de que o MSYS2 está no seu PATH (ex: `$env:PATH = "C:\msys64\mingw64\bin;" + $env:PATH` no PowerShell).
3. Crie a pasta de build e compile:
   ```bash
   cmake -B build
   cmake --build build
   ```
4. O executável `metalslug.exe` será gerado. É só rodar!

---

## 🕹️ Controles

- **A / D**: Andar para Esquerda / Direita
- **S (Segurar)**: Pular através das plataformas flutuantes
- **Espaço**: Pulo (Permite Pulo Duplo no ar!)
- **Mouse**: Mira (Crosshair) livre em 360 graus
- **Clique Esquerdo**: Tiro Normal (Pistola de plasma). *Cuidado para não atirar sem parar e superaquecer a arma!*
- **Clique Direito**: Tiro Forte (Espingarda de plasma - Consume 1 carga das barras azuis).

---

## ⚙️ Como o Jogo Funciona (Mecânicas)

- **Fases e Escala:** O jogo foi estruturado com Clean Code para comportar múltiplas fases. Na Fase 1, seu objetivo é sobreviver a hordas até matar **50 Soldados**.
- **Superaquecimento (Overheat):** Sua arma principal suporta 15 tiros seguidos. Ao chegar no zero, a arma superaquece (o MegaMan vai brilhar de baixo para cima com uma temperatura Laranja) e você ficará impossibilitado de atirar por 3 segundos.
- **Tiros Fortes:** Você possui 5 cargas de tiros especiais (Espingarda). Quando usadas, demoram cerca de 3 a 5 segundos para recarregar o pente completo.
- **Dano e Game Over:** Você aguenta 10 hits de vida. Quando sofre dano, o personagem pisca em vermelho intensamente. Se a vida zerar, a animação de morte roda, levando-o a tela de **Game Over**, onde pode recomeçar a fase com o Botão Esquerdo ou Sair com o Botão Direito.
- **Ícones (Loot):** Os inimigos mortos têm 30% de chance de derrubar caixas, que contêm buffs instantâneos como: Regeneração de Vida, Super Velocidade de Movimento, Instakill (Morte Súbita x3 de Dano) e Super Pulo (deixa o rastro do MegaMan verde neon!).

---

## 🛠️ Arquitetura e Port para a FPGA (DE1-SoC Linux)

Este projeto foi desenhado sob a perspectiva do **Princípio da Responsabilidade Única (SRP)**. Não utilizamos bibliotecas prontas como Unity ou Godot. O jogo manipula os pixels manualmente.

Para rodar este jogo num sistema Embarcado, como a placa DE1-SoC (ARM v7) utilizando um Kernel Linux:

1. **Alteração do Driver (VGA)**:
   Abra o `CMakeLists.txt` e desative o suporte ao Windows alterando para `option(USE_SDL_BACKEND "Use SDL2 for testing" OFF)`.
   A Engine passará a compilar o arquivo nativo `framebuffer_de1soc.c`, que ignora a janela do computador e passa a cuspir o output de renderização diretamente na porta VGA física através de mapeamento de memória via Linux (`/dev/mem` no endereço físico `0x08000000`).

2. **Displays e LEDs**:
   Para simplificar o desenvolvimento de drivers para a placa, a vida do jogador e os inimigos restantes (que podem ser mostrados respectivamente nos LEDs e nos Displays de 7-Segmentos da placa) são expostos em variáveis globais limpas criadas em `include/hardware/hardware_state.h`.

3. **Input Físico USB**:
   O controle pelo mouse e teclado no Linux Embarcado foi idealizado com PThreads em Backgound que escutam os nós do Linux (`/dev/input/mice` e `/dev/input/eventX`).

Toda a lógica de colisões em mapas matriciais, subida de ladeiras e gravidade é feita nativamente por cálculos analíticos na classe `jogador.c`, mantendo a execução levíssima e otimizada para o processador de baixo poder computacional da placa!

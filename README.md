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

### Compilando para PC (Windows / SDL2)
1. Abra um terminal no diretório do projeto.
2. Certifique-se de que o MSYS2 está no seu PATH (ex: `$env:PATH = "C:\msys64\mingw64\bin;" + $env:PATH` no PowerShell).
3. Compile o jogo ativando a renderização gráfica no PC:
   ```bash
   cmake -B build
   cmake --build build
   ```
4. O executável `metalslug.exe` será gerado na pasta `build`. É só rodar!

### Cross-Compilando para a DE1-SoC (ARM v7) no Windows
1. Adicione o compilador ARM ao PATH:
   ```bash
   export PATH=/c/Users/gollu/Documents/AntiGravity/2_Demake_Slug/Trabalho-Final-DE1-S0C---Sistema-Embarcados-2026.1/build/arm-gcc/gcc-arm/bin:$PATH
   ```
2. Compile desligando a SDL e ativando a toolchain ARM:
   ```bash
   cmake -S . -B build_arm -G Ninja -DCMAKE_TOOLCHAIN_FILE=arm_toolchain.cmake -DUSE_SDL_BACKEND=OFF -DCMAKE_BUILD_TYPE=Release
   cmake --build build_arm
   ```
3. Envie o binário `metalslug` da pasta `build_arm` para o Linux Embarcado da DE1-SoC e execute como `root`!

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

1. **Alteração do Driver (VGA) com Double Buffering**:
   Quando a tag `USE_SDL_BACKEND` é desligada, a Engine compila o backend `framebuffer_de1soc.c`. Ele realiza o acesso ao Controlador VGA via memória (`/dev/mem`). Para evitar que a tela pisque a 60fps (*Tearing*), fazemos o mapeamento duplo das memórias: o Front Buffer (`0xC8000000`) e o Back Buffer (`0xC0000000`). Nós desenhamos os pixels no Buffer oculto e comandamos o registrador `0xFF203020` da placa para trocar de tela no próximo *V-Sync*. Além disso, varremos e limpamos o *Character Buffer* (`0xC9000000`) para apagar textos fantasmas do console Linux!

2. **Displays, LEDs e Chaves (Lightweight Bridge)**:
   Mapeamos um total de 2MB sobre o barramento HPS-to-FPGA Lightweight Bridge (`0xFF200000`). Interagimos nativamente com o hardware usando aritmética de ponteiros C (ex: `lw_ptr + 0x0` para LEDs e `lw_ptr + 0x20` para os Displays de 7-Seg). A cada frame, os Leds representam graficamente a vida do MegaMan, enquanto os Displays (convertidos nativamente de BCD para 7 Segmentos via código) realizam a contagem regressiva viva de Inimigos restantes. Também criamos uma Thread separada que lê os *Switches* 20 vezes por segundo: levantar a `CHAVE 0` pausa automaticamente toda a engine de colisão e física, congelando o jogo!

3. **Input Físico USB**:
   O controle pelo mouse e teclado no Linux Embarcado foi idealizado com PThreads em Background que escutam os nós crus do Linux (`/dev/input/mice` e `/dev/input/eventX`).

Toda a lógica de colisões em mapas matriciais, subida de ladeiras e gravidade é feita nativamente por cálculos analíticos na classe `jogador.c`, mantendo a execução levíssima e otimizada para o processador de baixo poder computacional da placa!

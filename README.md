# Metal Slug Demake na DE1-SoC

Um "demake" inspirado na mecânica *run-and-gun* clássica da franquia Metal Slug (protagonizado pelo MegaMan), desenvolvido em **C17** do zero (sem engines externas) para rodar nativamente na placa **FPGA DE1-SoC (ARM v7, Linux Embarcado)** manipulando os registradores de Hardware diretamente via mapeamento de memória. 

O projeto conta com uma arquitetura híbrida: você pode jogar, testar e debugar no seu **PC (Windows/Linux via SDL2)** e compilar o código cruzado (*Cross-Compile*) para a placa alvo!

---

## 🎮 Como Rodar o Jogo (PC / Windows)

Para testar rapidamente o jogo na sua máquina local com interface gráfica usando a SDL2:

### Pré-requisitos
1. **MSYS2** instalado (ex: em `C:\msys64`).
2. Pacotes no MSYS2 (rode no terminal MSYS2 MinGW64):
   ```bash
   pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-make mingw-w64-x86_64-SDL2
   ```

### Compilando e Jogando
1. Abra um terminal PowerShell no diretório do projeto e inclua o MSYS2 no PATH:
   ```powershell
   $env:PATH = "C:\msys64\mingw64\bin;" + $env:PATH
   ```
2. Crie os arquivos de build da interface para PC e compile:
   ```bash
   cmake -B build
   cmake --build build
   ```
3. O executável será gerado em `build/metalslug.exe`. Basta rodar!

---

## 🚀 Como Compilar e Transferir para a DE1-SoC (Placa)

O jogo utiliza os periféricos nativos da placa (VGA, Displays de 7-Segmentos, LEDs, Switches e Mouse/Teclado USB).

### 1. Compilando o binário ARM no Windows
1. Adicione a *toolchain* cruzada do ARM ao seu PATH (substitua pelo seu caminho real):
   ```powershell
   $env:PATH = "C:\Users\gollu\Documents\AntiGravity\2_Demake_Slug\Trabalho-Final-DE1-S0C---Sistema-Embarcados-2026.1\build\arm-gcc\gcc-arm\bin;" + $env:PATH
   ```
2. Compile desligando o SDL (`-DUSE_SDL_BACKEND=OFF`):
   ```bash
   cmake -S . -B build_arm -G Ninja -DCMAKE_TOOLCHAIN_FILE=arm_toolchain.cmake -DUSE_SDL_BACKEND=OFF
   cmake --build build_arm
   ```

### 2. Acessando a Placa via Cabo Serial (PuTTY)
Antes de transferir via rede, você precisa entrar no terminal da placa para saber qual IP ela recebeu.
1. Instale o **Driver USB-to-UART** (FTDI ou Prolific) para que o Windows reconheça o cabo Console da placa.
2. Descubra a porta COM no *Gerenciador de Dispositivos* (ex: `COM3`).
3. Abra o **PuTTY**, selecione conexão do tipo **Serial**, insira sua porta COM e defina o *Speed (Baud rate)* como `115200`.
4. Clique em *Open*, faça login como `root` e rode o comando `ifconfig` para anotar o IP da placa (ex: `164.41.179.50`).

### 3. Transferindo o Jogo via Ethernet (TCP)
Devido ao tamanho do executável, **não use o cabo Serial para a transferência de arquivos** (é muito lento e corrompe o binário). Use a rede Ethernet:

1. **Na placa DE1-SoC (Receptor):**
   Abra a porta e aguarde o arquivo ficar escutando:
   ```bash
   nc -l 12345 > metalslug
   ```
2. **No seu Computador (Remetente):**
   Mande o binário com um script Python rápido apontando para o IP da placa (ex: `164.41.179.50`):
   ```bash
   python -c "import socket; sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM); sock.connect(('164.41.179.50', 12345)); f = open('build_arm/metalslug', 'rb'); sock.sendall(f.read()); f.close(); sock.close(); print('Transferência Concluída!')"
   ```
3. De volta na Placa, dê permissão de execução e inicie o jogo como `root`:
   ```bash
   chmod +x metalslug
   ./metalslug
   ```

---

## ⚙️ Fases e Mecânicas

*   **Fase 1 (Ártico):** Hordas de soldados paraquedistas. Avance para a direita, ou explore o caminho escondido à esquerda para acessar o *Easter Egg*.
*   **Fase 1.5 (Bônus - Copycat):** Enfrente um chefe que copia todos os seus movimentos com 30 frames de atraso!
*   **Fase 2 (Cidade Quebrada):** Oponentes "Spider Jockey" que disparam flechas em trajetórias balísticas-parabólicas em tempo real.
*   **Fase 3 (Bunker - Tank Boss):** Um embate contra o tanque de guerra que lança Goombas dinâmicos pelo canhão superior.
*   **Power-Ups (Caixas de Loot):** Ao destruir inimigos, há chances de *drops* contendo cura (+2 HP), Super Velocidade/Dano (3x mais forte) e Super Pulo (rastro neon).
*   **Aquecimento da Arma:** Limite dinâmico de 15 tiros consecutivos. Tiros excessivos travam a arma na cor Laranja por 3 segundos de resfriamento.

## 🛠️ Detalhes da Arquitetura de Hardware (DE1-SoC)

O software interage cruamente com o sistema:
1. **Vídeo VGA (Shadow Buffering):** O acesso gráfico é feito alocando uma matriz paralela na memória RAM do processador HPS e despejando seu conteúdo diretamente para a base `0xC8000000` (Pixel Buffer) usando `memcpy`. Isso neutraliza o *tearing* (tremulações visuais). 
2. **Displays e LEDs:** Toda a barra de vida (10 LEDs) e o marcador de inimigos restantes (Hexadecimais) são controlados via ponteiro em C sobre a *Lightweight Bridge* (`0xFF200000`).
3. **Hardware Pause:** Uma PThread em *background* lê periodicamente a Chave 0 (`SW0`). Ao levantá-la, a atualização física da engine entra em hiato. A entrada USB bruta é extraída diretamente dos nós descritores do Linux (`/dev/input`).

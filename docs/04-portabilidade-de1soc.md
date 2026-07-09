# Portando para a DE1-SoC

## O que já está pronto pra isso

O jogo é C17 + SDL2 (no PC) por trás de uma abstração de framebuffer (veja [03-arquitetura.md](03-arquitetura.md)). Toda a lógica de personagens/jogo, por construção, não conhece SDL nem hardware — só chama fb_*.

## Compilação Cruzada (Cross-Compile) do Windows para ARM

O melhor caminho é utilizar a nossa toolchain GCC ARM local (build/arm-gcc/gcc-arm). Para compilar para a placa de dentro do próprio Windows usando o ambiente MSYS2:

`ash
# Exportar a toolchain para o PATH do bash
export PATH=/c/Users/gollu/Documents/AntiGravity/2_Demake_Slug/Trabalho-Final-DE1-S0C---Sistema-Embarcados-2026.1/build/arm-gcc/gcc-arm/bin:$PATH

# Compilar com CMake desligando o SDL
cmake -S . -B build_arm -G Ninja -DCMAKE_TOOLCHAIN_FILE=arm_toolchain.cmake -DUSE_SDL_BACKEND=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build_arm
`
Após isso, basta transferir o arquivo metalslug da pasta uild_arm para a placa e executar!

## Como funciona o Hardware da DE1-SoC

- VGA (Pixel Buffer e Character Buffer): O endereço real mapeado pelo HPS é 0xC8000000 (Front Buffer) ou 0xC0000000 (Back Buffer). Para limpar o console do Linux da tela, mapeamos e limpamos o Character Buffer em 0xC9000000. Além disso, a memória exige um alinhamento de 512 pixels (1024 bytes) por linha.
- Periféricos Nativos (Lightweight Bridge): Usamos /dev/mem no endereço 0xFF200000 para controlar LEDs, Chaves e Displays de 7-Segmentos, integrando tudo nativamente.
- Teclado/Mouse USB: O Linux embarcado recebe os inputs físicos processados por Threads em C (pthread) que não bloqueiam o desenho de telas.

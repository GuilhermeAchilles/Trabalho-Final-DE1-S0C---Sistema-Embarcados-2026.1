# Compilar, rodar e debugar

## Pelo terminal (MSYS2 MINGW64)

```bash
cmake -S . -B build -G Ninja      # configura (só precisa rodar de novo se o CMakeLists mudar)
cmake --build build               # compila
./build/metalslug.exe             # roda
```

## Pelo VS Code

Foram criadas três tasks em `.vscode/tasks.json`:

| Task | O que faz | Como chamar |
|---|---|---|
| **CMake: Configure** | roda `cmake -S . -B build -G Ninja` | chamada automaticamente pelas outras duas |
| **CMake: Build** | compila (chama Configure antes) | `Ctrl+Shift+B` |
| **Run** | compila e executa o `.exe` (chama Build antes), sem debugger | `Ctrl+Shift+P` → "Tasks: Run Task" → **Run**, ou menu `Terminal` → `Run Task...` → **Run** |

`Ctrl+Shift+B` só builda — ele **não** roda o executável. Pra ver o jogo rodando sem parar em breakpoints, use a task **Run**.

## Debug (breakpoints, step, inspecionar variáveis)

1. Clique na margem esquerda de uma linha do código pra criar um breakpoint.
2. Abra a aba **Run and Debug** (`Ctrl+Shift+D`).
3. No dropdown do topo, selecione **"Debug metalslug"** (configuração em `.vscode/launch.json`).
4. Aperte `F5`.

⚠️ **Não use o botão de play (▷) que aparece no canto superior direito do editor** ao abrir um `.c` — ele ignora o `launch.json` e usa outro compilador (TDM-GCC) sem saber da nossa estrutura de `include/`. Detalhes em [05-problemas-resolvidos.md](05-problemas-resolvidos.md).

Requer `gdb` instalado no MinGW64 (`pacman -S mingw-w64-x86_64-gdb`).

## Trocando o backend de renderização

Por padrão a build usa a janela SDL2 (preview de desenvolvimento). Pra simular o backend de destino (DE1-SoC):

```bash
cmake -S . -B build -G Ninja -DUSE_SDL_BACKEND=OFF
```

⚠️ Isso **não compila no Windows** — `framebuffer_de1soc.c` usa `sys/mman.h` (mmap de `/dev/mem`), que só existe em Linux. Esse modo só faz sentido compilando direto na DE1-SoC (ou em qualquer Linux), não no MinGW64.

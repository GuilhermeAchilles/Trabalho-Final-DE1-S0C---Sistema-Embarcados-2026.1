# Problemas encontrados e como foram resolvidos

Registro histórico — útil pra não repetir a mesma investigação se o mesmo sintoma aparecer de novo.

## 1. `undefined reference to 'SDL_main'` no link

**Sintoma:** `cmake --build build` falhava no link, não na compilação.

**Causa:** `src/main.c` estava vazio. `target_compile_definitions(metalslug PRIVATE main=SDL_main)` renomeia `main` → `SDL_main` via macro (truque do SDL2 no Windows) — sem nenhum `main` escrito, não existe `SDL_main` pro linker achar.

**Fix:** escrever de fato uma função `main` em `main.c`.

## 2. `cmake`/`ninja`/`gcc` não reconhecidos no terminal

**Sintoma:** `cmake: command not found` (Git Bash) ou `cmake não é reconhecido` (PowerShell).

**Causa:** o toolchain vive em `C:\msys64\mingw64\bin`, que só entra no PATH dentro de um terminal MSYS2 MINGW64 (variável `MSYSTEM=MINGW64` ativa isso via script de perfil).

**Fix:** usar o terminal "MSYS2 MINGW64" (já configurado como padrão em `.vscode/settings.json`), ou exportar o PATH manualmente. Ver [01-toolchain-setup.md](01-toolchain-setup.md).

## 3. IntelliSense não achava `SDL2/SDL.h`

**Sintoma:** sublinhado vermelho no `#include <SDL2/SDL.h>`, mesmo com o build funcionando.

**Causa:** IntelliSense (extensão C/C++) não sabe dos include paths que o CMake configura internamente via `find_package(SDL2)` — ele analisa o arquivo isoladamente.

**Fix:** `.vscode/c_cpp_properties.json` com `includePath` apontando pra `C:/msys64/mingw64/include` e `${workspaceFolder}/**`. Puramente cosmético — não afeta a compilação real.

## 4. Breakpoints não paravam em nada (gdb)

**Sintoma:** `break main.c:5` no gdb retornava `No source file named main.c`.

**Causa:** build sem símbolos de debug (`-g`) — `CMAKE_BUILD_TYPE` não estava definido.

**Fix:** em `CMakeLists.txt`:
```cmake
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()
```

## 5. `F5` compilava com o compilador errado e não achava `framebuffer.h`

**Sintoma:**
```
C:\TDM-GCC-64\bin\gcc.EXE ... c:\...\src\main.c -o c:\...\src\output\main.exe
fatal error: framebuffer.h: No such file or directory
```

**Causa:** `F5` (ou o botão ▷ no canto do editor) sem uma configuração de debug selecionada usa o recurso genérico "Build and debug active file" da extensão C/C++ — ele **ignora o `launch.json`** e monta um comando próprio, usando o primeiro `gcc` que acha no PATH global do Windows (TDM-GCC, não o do MinGW64), e compila só o arquivo aberto, sem saber da pasta `include/` do projeto nem do CMake.

**Fix:** na aba **Run and Debug**, selecionar explicitamente **"Debug metalslug"** no dropdown do topo antes de apertar `F5`. Essa configuração (`.vscode/launch.json`) builda via CMake e usa o gdb do MinGW64 corretamente. Ver [02-build-e-execucao.md](02-build-e-execucao.md).

## 6. Backend `de1soc` não compila no Windows

**Sintoma:** `fatal error: sys/mman.h: No such file or directory` ao configurar com `-DUSE_SDL_BACKEND=OFF`.

**Causa:** `mmap`/`/dev/mem` são APIs de Linux — não existem no MinGW64/Windows. Isso é esperado, não é um bug: esse backend só é pra compilar na própria DE1-SoC (ou outro Linux).

**Não tem fix no Windows** — é assim mesmo. Ver [04-portabilidade-de1soc.md](04-portabilidade-de1soc.md).

## 7. IntelliSense quebrando em arquivos `.h` com dados de sprite (arrays hex grandes)

**Sintoma:** erros tipo `operador literal definido pelo usuário não encontrado` ou `texto extra depois do esperado fim de número`, um por linha, em `sprites_achilles.h`.

**Causa:** o VS Code trata arquivos `.h` como **C++** por padrão no editor (independente do que o CMake realmente compila). Em C++, sequências de literais hex sem separador (`0xF81F,0xF81F,...`) colidem com a feature de "literais definidos pelo usuário" — algo que só existe em C++, não em C. O build real (gcc/CMake) nunca teve esse problema, porque compila como C de verdade.

**Fix:** `.vscode/settings.json` → `"files.associations": { "*.h": "c" }`, forçando o modo de linguagem C pra qualquer `.h` do projeto. Pode precisar reabrir o arquivo (ou `Ctrl+Shift+P` → "Reload Window") pra o editor aplicar.

## 8. Typos ao digitar `tiro.h`/`tiro.c` na mão (não compilava)

**Sintoma:** vários erros de compilação ao criar o sistema de tiro do zero: `TIRO_MAC` não definido, `esperada uma expressão`, `struct não possui campo "sprite"`.

**Causa:** erros de digitação comuns ao copiar/adaptar código manualmente:
- `#include "sprite.h"` em vez de `#include "sprite/sprite.h"` (caminho errado).
- Campo do struct `tipo_tiro_t` declarado como `frames`, mas usado como `sprite` no `.c` — inconsistência entre declaração e uso.
- Função declarada `tiros_iniciatr` no header mas definida com outro nome, ou vice-versa.
- `TIRO_MAC` em vez de `TIRO_MAX` numa condição de loop.
- `if (!->ativo)` em vez de `if (!t->ativo)` — faltou a variável antes da seta, erro de sintaxe.

**Fix:** revisão linha a linha comparando declaração (`.h`) com uso (`.c`); nenhum desses exigia mudança de lógica, só corrigir nome/caminho. Lição: depois de digitar um módulo novo na mão, builda **antes** de seguir adicionando mais coisa em cima — os erros ficam pequenos e fáceis de achar.

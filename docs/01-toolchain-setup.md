# Setup do ambiente (Windows)

## Por que MSYS2, e não outra coisa

O projeto compila com **gcc do MinGW64** via **CMake + Ninja**, usando **SDL2** como lib de janela/renderização durante o desenvolvimento. Tudo isso vem do MSYS2, instalado em `C:\msys64`.

`cmake`, `ninja`, `gcc` e `SDL2` ficam em `C:\msys64\mingw64\bin`. Esse caminho **não está** no PATH padrão do PowerShell/Git Bash — só existe quando você abre um terminal MSYS2 MINGW64 (que configura isso automaticamente via variável `MSYSTEM=MINGW64`).

## Pacotes instalados via pacman

```bash
pacman -S mingw-w64-x86_64-gdb
```

(o gcc, cmake, ninja e SDL2 já estavam presentes na instalação existente do MSYS2 quando o projeto começou).

Pra verificar o que está instalado:
```bash
ls /c/msys64/mingw64/bin | grep -iE "cmake|ninja|gcc|gdb"
ls /c/msys64/mingw64/include/SDL2
```

## Terminal padrão do VS Code

`.vscode/settings.json` define o perfil **MSYS2 MINGW64** como terminal padrão do VS Code — assim, o terminal integrado já abre com o PATH certo, sem precisar exportar nada manualmente.

## ⚠️ Cuidado: TDM-GCC também instalado na máquina

A máquina tem outro compilador instalado, **TDM-GCC** (`C:\TDM-GCC-64\bin\gcc.exe`), que aparece primeiro no PATH global do Windows/PowerShell. Isso não afeta o build normal (CMake/Ninja usam o compilador do MinGW64 explicitamente), mas afeta o botão de "play" (▷) da extensão C/C++ do VS Code — veja [05-problemas-resolvidos.md](05-problemas-resolvidos.md).

Se precisar rodar comandos manualmente fora do terminal MSYS2 MINGW64 (ex: dentro do Git Bash do Claude Code), é preciso forçar o PATH:
```bash
export PATH="/c/msys64/mingw64/bin:/c/msys64/usr/bin:$PATH"
```

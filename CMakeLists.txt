cmake_minimum_required(VERSION 3.16)
project(MetalSlugDemake C)

set(CMAKE_C_STANDARD 17)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# ON no PC (janela SDL de preview) / OFF na DE1-SoC (escreve direto no framebuffer VGA)
option(USE_SDL_BACKEND "Use SDL2 window as the framebuffer preview backend" ON)

add_executable(metalslug
    src/main.c
    src/jogador.c
    src/ui.c
    src/hardware_state.c
    src/fase1_logic.c
    sprite/sprite.c
    animacao/animacao.c
    ataques/tiros/tiro.c
    colisao/colisao.c
    inimigo/inimigo.c
    cenario/cenario.c
    cenario/fase1/fase1_dados.c)

target_include_directories(metalslug PRIVATE include)

if(USE_SDL_BACKEND)
    find_package(SDL2 REQUIRED)

    target_sources(metalslug PRIVATE framebuffer/framebuffer_sdl.c)
    target_include_directories(metalslug PRIVATE ${SDL2_INCLUDE_DIRS})

    if(WIN32)
        target_compile_definitions(metalslug PRIVATE main=SDL_main)
        target_link_libraries(metalslug PRIVATE mingw32 SDL2main)
    endif()

    target_link_libraries(metalslug PRIVATE SDL2)
else()
    target_sources(metalslug PRIVATE framebuffer/framebuffer_de1soc.c)
endif()

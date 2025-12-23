cmake_minimum_required(VERSION 3.13)

# ------------------------------------------------------------------------------
# Toolchain
# ------------------------------------------------------------------------------
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{DEVKITPRO})
        set(DEVKITPRO $ENV{DEVKITPRO})
        set(CMAKE_TOOLCHAIN_FILE
            "$ENV{DEVKITPRO}/cmake/Switch.cmake"
            CACHE PATH "toolchain file")
    else()
        message(FATAL_ERROR "Please define DEVKITPRO to point to your SDK path!")
    endif()
endif()

project(renpy-switch C CXX)

# ------------------------------------------------------------------------------
# Build type
# ------------------------------------------------------------------------------
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

# ------------------------------------------------------------------------------
# Compile definitions (Python / Switch)
# ------------------------------------------------------------------------------
add_compile_definitions(
    __SWITCH__
    Py_NO_ENABLE_SHARED
)

# ------------------------------------------------------------------------------
# Compiler flags
# ------------------------------------------------------------------------------
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(C_FLAGS
    -DFRIBIDI_ENTRY=
    -DHAVE_CONFIG_H=1
)

set(OPT_FLAGS
    -Ofast
    -fsingle-precision-constant
    -fno-common
)

# ------------------------------------------------------------------------------
# Include paths
# ------------------------------------------------------------------------------
set(INCLUDES
    ${DEVKITPRO}/portlibs/switch/include/python3.9
    ${DEVKITPRO}/portlibs/switch/include/fribidi
    include
    include/module
    include/module/src
    include/module/pygame_sdl2
    include/module/pygame_sdl2/src
)

# ------------------------------------------------------------------------------
# Sources
# ------------------------------------------------------------------------------
set(SOURCES
    source/module/IMG_savepng.c
    source/module/SDL2_rotozoom.c
    source/module/SDL_gfxPrimitives.c
    source/module/_renpy.c
    source/module/_renpybidi.c
    source/module/alphablit.c
    source/module/core.c
    source/module/ffmedia.c
    source/module/ftsupport.c
    source/module/pygame_sdl2.color.c
    source/module/pygame_sdl2.controller.c
    source/module/pygame_sdl2.display.c
    source/module/pygame_sdl2.draw.c
    source/module/pygame_sdl2.error.c
    source/module/pygame_sdl2.event.c
    source/module/pygame_sdl2.font.c
    source/module/pygame_sdl2.gfxdraw.c
    source/module/pygame_sdl2.image.c
    source/module/pygame_sdl2.joystick.c
    source/module/pygame_sdl2.key.c
    source/module/pygame_sdl2.locals.c
    source/module/pygame_sdl2.mixer.c
    source/module/pygame_sdl2.mixer_music.c
    source/module/pygame_sdl2.mouse.c
    source/module/pygame_sdl2.power.c
    source/module/pygame_sdl2.pygame_time.c
    source/module/pygame_sdl2.rect.c
    source/module/pygame_sdl2.render.c
    source/module/pygame_sdl2.rwobject.c
    source/module/pygame_sdl2.scrap.c
    source/module/pygame_sdl2.surface.c
    source/module/pygame_sdl2.transform.c
    source/module/renpy.audio.renpysound.c
    source/module/renpy.display.accelerator.c
    source/module/renpy.display.matrix.c
    source/module/renpy.display.render.c
    source/module/renpy.gl.gldraw.c
    source/module/renpy.gl.glenviron_shader.c
    source/module/renpy.gl.glrtt_copy.c
    source/module/renpy.gl.glrtt_fbo.c
    source/module/renpy.gl.gltexture.c
    source/module/renpy.pydict.c
    source/module/renpy.style.c
    source/module/renpy.styledata.style_activate_functions.c
    source/module/renpy.styledata.style_functions.c
    source/module/renpy.styledata.style_hover_functions.c
    source/module/renpy.styledata.style_idle_functions.c
    source/module/renpy.styledata.style_insensitive_functions.c
    source/module/renpy.styledata.style_selected_activate_functions.c
    source/module/renpy.styledata.style_selected_functions.c
    source/module/renpy.styledata.style_selected_hover_functions.c
    source/module/renpy.styledata.style_selected_idle_functions.c
    source/module/renpy.styledata.style_selected_insensitive_functions.c
    source/module/renpy.styledata.styleclass.c
    source/module/renpy.styledata.stylesets.c
    source/module/renpy.text.ftfont.c
    source/module/renpy.text.textsupport.c
    source/module/renpy.text.texwrap.c
    source/module/renpysound_core.c
    source/module/ttgsubtable.c
    source/module/write_jpeg.c
    source/module/write_png.c
    source/module/renpybidicore.c

    source/module/renpy.gl2.gl2draw.c
    source/module/renpy.gl2.gl2mesh.c
    source/module/renpy.gl2.gl2mesh2.c
    source/module/renpy.gl2.gl2mesh3.c
    source/module/renpy.gl2.gl2model.c
    source/module/renpy.gl2.gl2polygon.c
    source/module/renpy.gl2.gl2shader.c
    source/module/renpy.gl2.gl2texture.c
    source/module/renpy.uguu.gl.c
    source/module/renpy.uguu.uguu.c
    
    source/module/renpy.lexersupport.c
    source/module/renpy.display.quaternion.c
)

# ------------------------------------------------------------------------------
# pkg-config
# ------------------------------------------------------------------------------
find_package(PkgConfig REQUIRED)

pkg_check_modules(EGL REQUIRED IMPORTED_TARGET egl)
pkg_check_modules(FREETYPE2 REQUIRED IMPORTED_TARGET freetype2)
pkg_check_modules(GLAPI REQUIRED IMPORTED_TARGET glapi)
pkg_check_modules(GLESV2 REQUIRED IMPORTED_TARGET glesv2)
pkg_check_modules(LIBAVCODEC REQUIRED IMPORTED_TARGET libavcodec)
pkg_check_modules(LIBAVFILTER REQUIRED IMPORTED_TARGET libavfilter)
pkg_check_modules(LIBAVFORMAT REQUIRED IMPORTED_TARGET libavformat)
pkg_check_modules(LIBAVUTIL REQUIRED IMPORTED_TARGET libavutil)
pkg_check_modules(LIBDRM_NOUVEAU REQUIRED IMPORTED_TARGET libdrm_nouveau)
pkg_check_modules(LIBSWRESAMPLE REQUIRED IMPORTED_TARGET libswresample)
pkg_check_modules(LIBSWSCALE REQUIRED IMPORTED_TARGET libswscale)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
pkg_check_modules(SDL2_GFX REQUIRED IMPORTED_TARGET SDL2_gfx)
pkg_check_modules(SDL2_IMAGE REQUIRED IMPORTED_TARGET SDL2_image)
pkg_check_modules(SDL2_MIXER REQUIRED IMPORTED_TARGET SDL2_mixer)
pkg_check_modules(SDL2_TTF REQUIRED IMPORTED_TARGET SDL2_ttf)
pkg_check_modules(ZLIB REQUIRED IMPORTED_TARGET zlib)

# ---- FIX: pkg-config uses $(DEVKITPRO) which CMake cannot expand ----
get_target_property(_drm_includes PkgConfig::LIBDRM_NOUVEAU INTERFACE_INCLUDE_DIRECTORIES)

if(_drm_includes)
    set(_fixed_includes "")
    foreach(dir IN LISTS _drm_includes)
        if(dir MATCHES "\\$\\(DEVKITPRO\\)")
            string(REPLACE "\$(DEVKITPRO)" "${DEVKITPRO}" dir "${dir}")
        endif()
        list(APPEND _fixed_includes "${dir}")
    endforeach()

    set_target_properties(PkgConfig::LIBDRM_NOUVEAU PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${_fixed_includes}"
    )
endif()

# ------------------------------------------------------------------------------
add_executable(renpy-switch ${SOURCES})

set_target_properties(renpy-switch PROPERTIES
    LINKER_LANGUAGE CXX
)

target_include_directories(${PROJECT_NAME} PUBLIC ${INCLUDES})

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<COMPILE_LANGUAGE:C>:${C_FLAGS}>
    $<$<CONFIG:RELEASE>:${OPT_FLAGS}>
    $<$<CONFIG:RELWITHDEBINFO>:${OPT_FLAGS}>
)

target_link_libraries(${PROJECT_NAME}
    ${DEVKITPRO}/portlibs/switch/lib/libpython3.9.a
    ${DEVKITPRO}/portlibs/switch/lib/libfribidi.a
    m
    z
    PkgConfig::EGL
    PkgConfig::FREETYPE2
    PkgConfig::GLAPI
    PkgConfig::GLESV2
    PkgConfig::LIBAVCODEC
    PkgConfig::LIBAVFILTER
    PkgConfig::LIBAVFORMAT
    PkgConfig::LIBAVUTIL
    PkgConfig::LIBDRM_NOUVEAU
    PkgConfig::LIBSWRESAMPLE
    PkgConfig::LIBSWSCALE
    PkgConfig::SDL2
    PkgConfig::SDL2_GFX
    PkgConfig::SDL2_IMAGE
    PkgConfig::SDL2_MIXER
    PkgConfig::SDL2_TTF
    PkgConfig::ZLIB
)

# ------------------------------------------------------------------------------
# NSO generation
# ------------------------------------------------------------------------------
add_custom_command(
    OUTPUT renpy-switch.nso
    COMMAND ${DEVKITPRO}/tools/bin/elf2nso
            renpy-switch.elf
            renpy-switch.nso
    DEPENDS renpy-switch
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_custom_target(renpy-switch_nso ALL
    DEPENDS renpy-switch.nso
)

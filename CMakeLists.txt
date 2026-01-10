cmake_minimum_required(VERSION 3.0.2)

# Проверка кросс-компиляции ДО определения проекта
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{DEVKITPRO})
        set(DEVKITPRO $ENV{DEVKITPRO})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{DEVKITPRO}/switch.cmake" CACHE PATH "toolchain file")
    else()
        message(FATAL_ERROR "Please define DEVKITPRO to point to your SDK path!")
    endif()
endif()

project(renpy-switch-modules)

# Устанавливаем переменные окружения для pkg-config ПЕРЕД find_package
set(ENV{PKG_CONFIG_EXECUTABLE} /opt/devkitpro/portlibs/switch/bin/aarch64-none-elf-pkg-config)
set(ENV{PKG_CONFIG_PATH} /opt/devkitpro/portlibs/switch/lib/pkgconfig)
set(ENV{PKG_CONFIG_LIBDIR} /opt/devkitpro/portlibs/switch/lib/pkgconfig)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

# Добавляем флаги для Switch и кросс-компиляции
set(C_FLAGS
    -DFRIBIDI_ENTRY=
    -DHAVE_CONFIG_H=1
    -DPy_LIMITED_API=0x03090000
    -D__SWITCH__  # Важно для Switch-специфичного кода
    -mcpu=cortex-a57+fp+simd  # Архитектура Switch
    -ffunction-sections
    -fdata-sections
)

set(OPTIMIZATION_FLAGS
    -Ofast
    -flto
    -fno-stack-protector
)

# Обновляем пути для Python 3.9
set(INCLUDES
    ${DEVKITPRO}/portlibs/switch/include/python3.9
    ${DEVKITPRO}/portlibs/switch/include/fribidi
    include
    include/module
    include/module/src
)

# source/libnx_wrap.c disabled for the time being

set(SOURCES
    # Ваши исходные файлы...
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

    source/module/renpy.compat.dictviews.c
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

# Проверяем кросс-компиляцию
if(NOT CMAKE_CROSSCOMPILING)
    message(WARNING "Project is not being cross-compiled! Use -DCMAKE_TOOLCHAIN_FILE")
endif()

# Устанавливаем корректные пути для Python
find_file(PYTHON_LIBRARY libpython3.9.a 
    PATHS ${DEVKITPRO}/portlibs/switch/lib
    NO_DEFAULT_PATH
    REQUIRED
)

find_path(PYTHON_INCLUDE_DIR Python.h
    PATHS ${DEVKITPRO}/portlibs/switch/include/python3.9
    NO_DEFAULT_PATH
    REQUIRED
)

find_package(PkgConfig REQUIRED)

# Проверяем библиотеки с правильными именами для devkitPro
pkg_check_modules(EGL REQUIRED egl)
pkg_check_modules(FREETYPE2 REQUIRED freetype2)
pkg_check_modules(GLAPI REQUIRED glapi)
pkg_check_modules(GLESV2 REQUIRED glesv2)
pkg_check_modules(LIBAVCODEC REQUIRED libavcodec)
pkg_check_modules(LIBAVFILTER REQUIRED libavfilter)
pkg_check_modules(LIBAVFORMAT REQUIRED libavformat)
pkg_check_modules(LIBAVUTIL REQUIRED libavutil)
pkg_check_modules(LIBSWRESAMPLE REQUIRED libswresample)
pkg_check_modules(LIBSWSCALE REQUIRED libswscale)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_GFX REQUIRED SDL2_gfx)
pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
pkg_check_modules(SDL2_MIXER REQUIRED SDL2_mixer)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
pkg_check_modules(ZLIB REQUIRED zlib)

# Для Ren'Py 8
pkg_check_modules(HARFBUZZ REQUIRED harfbuzz)
pkg_check_modules(WEBP REQUIRED libwebp)
pkg_check_modules(OPENAL REQUIRED openal-soft)  # Важно: openal-soft, а не openal
pkg_check_modules(OGG REQUIRED ogg)
pkg_check_modules(VORBIS REQUIRED vorbis)
pkg_check_modules(VORBISFILE REQUIRED vorbisfile)  # Добавляем vorbisfile
pkg_check_modules(OPUSFILE REQUIRED opusfile)
pkg_check_modules(MPG123 REQUIRED libmpg123)
pkg_check_modules(MODPLUG REQUIRED libmodplug)
pkg_check_modules(FLAC REQUIRED flac)  # Добавляем FLAC поддержку

# Дополнительные библиотеки, которые могут понадобиться
pkg_check_modules(LIBDRM REQUIRED libdrm)
pkg_check_modules(LIBPNG REQUIRED libpng)
pkg_check_modules(LIBJPEG REQUIRED libjpeg)

add_library(${PROJECT_NAME} STATIC ${SOURCES})

# Связываем библиотеки - УБИРАЕМ IMPORTED_TARGET
target_link_libraries(${PROJECT_NAME}
    ${PYTHON_LIBRARY}
    ${DEVKITPRO}/portlibs/switch/lib/libfribidi.a
    m
    ${EGL_LIBRARIES}
    ${FREETYPE2_LIBRARIES}
    ${GLAPI_LIBRARIES}
    ${GLESV2_LIBRARIES}
    ${LIBAVCODEC_LIBRARIES}
    ${LIBAVFILTER_LIBRARIES}
    ${LIBAVFORMAT_LIBRARIES}
    ${LIBAVUTIL_LIBRARIES}
    ${LIBSWRESAMPLE_LIBRARIES}
    ${LIBSWSCALE_LIBRARIES}
    ${SDL2_LIBRARIES}
    ${SDL2_GFX_LIBRARIES}
    ${SDL2_IMAGE_LIBRARIES}
    ${SDL2_MIXER_LIBRARIES}
    ${SDL2_TTF_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${HARFBUZZ_LIBRARIES}
    ${WEBP_LIBRARIES}
    ${OPENAL_LIBRARIES}
    ${OGG_LIBRARIES}
    ${VORBIS_LIBRARIES}
    ${VORBISFILE_LIBRARIES}
    ${OPUSFILE_LIBRARIES}
    ${MPG123_LIBRARIES}
    ${MODPLUG_LIBRARIES}
    ${FLAC_LIBRARIES}
    ${LIBDRM_LIBRARIES}
    ${LIBPNG_LIBRARIES}
    ${LIBJPEG_LIBRARIES}
)

# Устанавливаем include директории из pkg-config
target_include_directories(${PROJECT_NAME} PUBLIC 
    ${INCLUDES}
    ${EGL_INCLUDE_DIRS}
    ${FREETYPE2_INCLUDE_DIRS}
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_MIXER_INCLUDE_DIRS}
    ${OPENAL_INCLUDE_DIRS}
    ${HARFBUZZ_INCLUDE_DIRS}
)

# Флаги компиляции
target_compile_options(${PROJECT_NAME} PUBLIC 
    $<$<COMPILE_LANGUAGE:C>:${C_FLAGS}>
    $<$<COMPILE_LANGUAGE:CXX>:${C_FLAGS}>
)

target_compile_options(${PROJECT_NAME} PUBLIC 
    $<$<CONFIG:RELEASE>:${OPTIMIZATION_FLAGS}>
    $<$<CONFIG:RELWITHDEBINFO>:${OPTIMIZATION_FLAGS}>
)

# Линкер флаги
target_link_options(${PROJECT_NAME} PUBLIC
    -specs=${DEVKITPRO}/libnx/switch.specs
    -Wl,-Map,${PROJECT_NAME}.map
    -Wl,--gc-sections
    -Wl,--start-group
    -Wl,--end-group
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    LINKER_LANGUAGE C
    POSITION_INDEPENDENT_CODE ON
)

# Проверяем, что все библиотеки найдены
message(STATUS "EGL libraries: ${EGL_LIBRARIES}")
message(STATUS "MPG123 libraries: ${MPG123_LIBRARIES}")
message(STATUS "OPENAL libraries: ${OPENAL_LIBRARIES}")

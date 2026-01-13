cmake_minimum_required(VERSION 3.0.2)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{DEVKITPRO})
        set(DEVKITPRO $ENV{DEVKITPRO})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{DEVKITPRO}/switch.cmake" CACHE PATH "toolchain file")
    else()
        message(FATAL_ERROR "Please define DEVKITPRO to point to your SDK path!")
    endif()
endif()

project(renpy-switch)

set(SWITCH_APP_NAME "Ren'Py")
set(SWITCH_AUTHOR   "renpytom")
set(SWITCH_VERSION  "8.3.7")  # Обновляем версию

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

set(C_FLAGS
    -DFRIBIDI_ENTRY=
    -DHAVE_CONFIG_H=1
    -DPy_LIMITED_API=0x03090000  # Для Python 3.9
)

set(OPTIMIZATION_FLAGS
    -Ofast
)

# Обновляем путь для Python 3.9
set(INCLUDES
    ${DEVKITPRO}/portlibs/switch/include/python3.9
)

set(SOURCES
    source/main.c
)

set(CMAKE_PREFIX_PATH "${DEVKITPRO}/portlibs/switch")
set(ENV{PKG_CONFIG_PATH} "${DEVKITPRO}/portlibs/switch/lib/pkgconfig")

find_package(PkgConfig REQUIRED)

# Обновляем все пути библиотек для Python 3.9
# Убираем старые workaround'ы и используем правильные пути

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

# Дополнительные библиотеки для Ren'Py 8
pkg_check_modules(HARFBUZZ REQUIRED IMPORTED_TARGET harfbuzz)
pkg_check_modules(WEBP REQUIRED IMPORTED_TARGET libwebp)
pkg_check_modules(OPENAL REQUIRED IMPORTED_TARGET openal)
pkg_check_modules(OGG REQUIRED IMPORTED_TARGET ogg)
pkg_check_modules(VORBIS REQUIRED IMPORTED_TARGET vorbis)
pkg_check_modules(OPUSFILE REQUIRED IMPORTED_TARGET opusfile)
pkg_check_modules(MPG123 REQUIRED IMPORTED_TARGET libmpg123)
pkg_check_modules(MODPLUG REQUIRED IMPORTED_TARGET libmodplug)

# Workaround для pkg-config
if(LIBDRM_NOUVEAU_INCLUDE_DIRS)
    set_property(TARGET PkgConfig::LIBDRM_NOUVEAU PROPERTY
                 INTERFACE_INCLUDE_DIRECTORIES "${DEVKITPRO}/portlibs/switch/include")
endif()

add_executable(${PROJECT_NAME} ${SOURCES})

# Обновляем библиотеку Python на 3.9
target_link_libraries(${PROJECT_NAME}
    ${DEVKITPRO}/portlibs/switch/lib/librenpy-switch-modules.a
    ${DEVKITPRO}/portlibs/switch/lib/libpython3.9.a
    m
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
    PkgConfig::HARFBUZZ
    PkgConfig::WEBP
    PkgConfig::OPENAL
    PkgConfig::OGG
    PkgConfig::VORBIS
    PkgConfig::OPUSFILE
    PkgConfig::MPG123
    PkgConfig::MODPLUG
)

target_include_directories(${PROJECT_NAME} PUBLIC ${INCLUDES})
target_compile_options(${PROJECT_NAME} PUBLIC $<$<COMPILE_LANGUAGE:C>:${C_FLAGS}>)
target_compile_options(${PROJECT_NAME} PUBLIC $<$<CONFIG:RELEASE>:${OPTIMIZATION_FLAGS}>)
target_compile_options(${PROJECT_NAME} PUBLIC $<$<CONFIG:RELWITHDEBINFO>:${OPTIMIZATION_FLAGS}>)
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

#add_custom_target(${PROJECT_NAME}.nso ALL
#    COMMAND ${DEVKITPRO}/tools/bin/elf2nso
#        ${PROJECT_NAME}
#        ${PROJECT_NAME}.nso
#    DEPENDS ${PROJECT_NAME}
#)


add_custom_command(
    OUTPUT ${PROJECT_NAME}.nso
    COMMAND ${DEVKITPRO}/tools/bin/elf2nso
        $<TARGET_FILE:${PROJECT_NAME}>
        ${PROJECT_NAME}.nso
    DEPENDS ${PROJECT_NAME}
    COMMENT "Converting ELF to NSO"
)

add_custom_target(${PROJECT_NAME}_nso ALL
    DEPENDS ${PROJECT_NAME}.nso
)

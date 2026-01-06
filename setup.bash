#!/bin/bash
set -e

export DEVKITPRO=/opt/devkitpro
export RENPY_VER=8.3.7
export PYGAME_SDL2_VER=2.1.0

# Обновление системы и установка зависимостей
apt-get -y update
apt-get -y upgrade

# Базовые инструменты сборки
apt -y install build-essential checkinstall git cmake ninja-build
apt -y install libncurses5-dev libssl-dev libsqlite3-dev tk-dev libgdbm-dev libc6-dev libbz2-dev libffi-dev liblzma-dev

# Python 3.9 (вместо Python 2.7)
apt -y install python3.9 python3.9-dev python3.9-distutils python3-pip

python3.9 --version

# Установка pip для Python 3.9
curl -sS https://bootstrap.pypa.io/get-pip.py | python3.9
pip3.9 --version

# Установка системных зависимостей
apt-get -y install p7zip-full libsdl2-dev libsdl2-image-dev libjpeg-dev libpng-dev libsdl2-ttf-dev libsdl2-mixer-dev libavformat-dev libfreetype6-dev libswscale-dev libglew-dev libfribidi-dev libavcodec-dev libswresample-dev libsdl2-gfx-dev libgl1-mesa-glx libharfbuzz-dev libwebp-dev libopenal-dev libvorbis-dev libopusfile-dev libmpg123-dev

# Установка Python-зависимостей для Ren'Py 8.3.7
pip3.9 install --upgrade pip
pip3.9 install future six typing requests ecdsa pefile Cython setuptools wheel
pip3.9 install pycryptodomex

# Установка devkitpro зависимостей
curl -LOC - https://github.com/greg6821-debug/scripts/releases/download/1.0-scripts/devkitpro-pkgbuild-helpers-2.2.3-1-any.pkg.tar.xz
curl -LOC - https://github.com/greg6821-debug/scripts/releases/download/1.0-scripts/switch-libfribidi-1.0.12-1-any.pkg.tar.xz

dkp-pacman -U --noconfirm devkitpro-pkgbuild-helpers-2.2.3-1-any.pkg.tar.xz
dkp-pacman -U --noconfirm switch-libfribidi-1.0.12-1-any.pkg.tar.xz

# Python 3.9 для Switch (используем предоставленную ссылку)
curl -LOC - https://github.com/knautilus/Utils/releases/download/v1.0/python39-switch.zip
unzip -qq python39-switch.zip -d $DEVKITPRO/portlibs/switch
rm python39-switch.zip

rm devkitpro-pkgbuild-helpers-2.2.3-1-any.pkg.tar.xz
rm switch-libfribidi-1.0.12-1-any.pkg.tar.xz

# Исправление CMake флагов
/bin/bash -c 'sed -i'"'"'.bak'"'"' '"'"'s/set(CMAKE_EXE_LINKER_FLAGS_INIT "/set(CMAKE_EXE_LINKER_FLAGS_INIT "-fPIC /'"'"' $DEVKITPRO/switch.cmake'

# Скачивание Ren'Py 8.3.7 SDK и исходников
curl -LOC - https://www.renpy.org/dl/$RENPY_VER/pygame_sdl2-$PYGAME_SDL2_VER+renpy$RENPY_VER.tar.gz
curl -LOC - https://www.renpy.org/dl/$RENPY_VER/renpy-$RENPY_VER-sdk.zip
curl -LOC - https://www.renpy.org/dl/$RENPY_VER/renpy-$RENPY_VER-source.tar.bz2

# Очистка и распаковка Ren'Py
rm -rf pygame_sdl2-$PYGAME_SDL2_VER+renpy$RENPY_VER pygame_sdl2-source
tar -xf pygame_sdl2-$PYGAME_SDL2_VER+renpy$RENPY_VER.tar.gz
mv pygame_sdl2-$PYGAME_SDL2_VER+renpy$RENPY_VER pygame_sdl2-source
rm pygame_sdl2-$PYGAME_SDL2_VER+renpy$RENPY_VER.tar.gz

rm -rf renpy-$RENPY_VER-source renpy-source
tar -xf renpy-$RENPY_VER-source.tar.bz2
mv renpy-$RENPY_VER-source renpy-source
rm renpy-$RENPY_VER-source.tar.bz2

rm -rf renpy-$RENPY_VER-sdk renpy_sdk
unzip -qq renpy-$RENPY_VER-sdk.zip -d renpy_sdk
rm renpy-$RENPY_VER-sdk.zip

# Применение патчей
if [ -f "renpy.patch" ]; then
    pushd renpy-source
    patch -p1 < ../renpy.patch
    pushd module
    rm -rf gen gen-static
    popd
    popd
fi

pushd pygame_sdl2-source
rm -rf gen gen-static
popd

echo "Установка завершена! Проверьте наличие необходимых патчей для Ren'Py 8.3.7"

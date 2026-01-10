#!/bin/bash
set -e

export DEVKITPRO=/opt/devkitpro
export RENPY_VER=8.3.7
export PYGAME_SDL2_VER=2.1.0

# Обновление системы
apt-get -y update
apt-get -y upgrade

# Базовые инструменты и Python 3.9 (как в Vita порте)
apt -y install build-essential checkinstall git cmake ninja-build rsync
apt -y install python3.9 python3.9-dev python3.9-distutils python3-pip

# Установка pip для Python 3.9
curl -sS https://bootstrap.pypa.io/get-pip.py | python3.9
pip3.9 --version

# Фиксация КРИТИЧЕСКИ важных версий (как в Vita порте)
pip3.9 install --upgrade pip
pip3.9 install "Cython==0.29.37"  # Не использовать > 3.0!
pip3.9 install future six typing requests ecdsa pefile setuptools wheel
pip3.9 install pycryptodomex

# Системные зависимости
apt-get -y install p7zip-full libsdl2-dev libsdl2-image-dev libjpeg-dev \
    libpng-dev libsdl2-ttf-dev libsdl2-mixer-dev libavformat-dev \
    libfreetype6-dev libswscale-dev libglew-dev libfribidi-dev \
    libavcodec-dev libswresample-dev libsdl2-gfx-dev libgl1-mesa-glx \
    libharfbuzz-dev libwebp-dev libopenal-dev libvorbis-dev \
    libopusfile-dev libmpg123-dev libmodplug-dev

# Установка devkitpro зависимостей
curl -LOC - https://github.com/greg6821-debug/scripts/releases/download/1.0-scripts/devkitpro-pkgbuild-helpers-2.2.3-1-any.pkg.tar.xz
curl -LOC - https://github.com/greg6821-debug/scripts/releases/download/1.0-scripts/switch-libfribidi-1.0.12-1-any.pkg.tar.xz

dkp-pacman -U --noconfirm devkitpro-pkgbuild-helpers-2.2.3-1-any.pkg.tar.xz
dkp-pacman -U --noconfirm switch-libfribidi-1.0.12-1-any.pkg.tar.xz

# Python 3.9 для Switch (нужно найти или собрать)
echo "Installing Python 3.9 for Switch..."
# Пока используем 3.9, но нужно искать 3.9
curl -LOC - https://github.com/knautilus/Utils/releases/download/v1.0/python39-switch.zip
unzip -qq python39-switch.zip -d $DEVKITPRO/portlibs/switch
rm python39-switch.zip

# Установка mpg123 для Switch (чтобы исправить ошибку CMake)
echo "Installing mpg123 for Switch..."
dkp-pacman -S --noconfirm switch-mpg123

# Сборка harfbuzz с поддержкой freetype (как в Vita порте)
echo "Building harfbuzz with freetype support..."
dkp-pacman -S --noconfirm switch-freetype switch-harfbuzz

# Исправление CMake флагов
/bin/bash -c 'sed -i'"'"'.bak'"'"' '"'"'s/set(CMAKE_EXE_LINKER_FLAGS_INIT "/set(CMAKE_EXE_LINKER_FLAGS_INIT "-fPIC /'"'"' $DEVKITPRO/switch.cmake'

# Скачивание Ren'Py 8.3.7
curl -LOC - https://www.renpy.org/dl/$RENPY_VER/pygame_sdl2-$PYGAME_SDL2_VER%2Brenpy$RENPY_VER.tar.gz
curl -LOC - https://www.renpy.org/dl/$RENPY_VER/renpy-$RENPY_VER-sdk.zip
curl -LOC - https://www.renpy.org/dl/$RENPY_VER/renpy-$RENPY_VER-source.tar.bz2

# Распаковка
rm -rf pygame_sdl2-source renpy-source renpy_sdk
tar -xf pygame_sdl2-$PYGAME_SDL2_VER+renpy$RENPY_VER.tar.gz
mv pygame_sdl2-$PYGAME_SDL2_VER+renpy$RENPY_VER pygame_sdl2-source
rm pygame_sdl2-$PYGAME_SDL2_VER+renpy$RENPY_VER.tar.gz

tar -xf renpy-$RENPY_VER-source.tar.bz2
mv renpy-$RENPY_VER-source renpy-source
rm renpy-$RENPY_VER-source.tar.bz2

unzip -qq renpy-$RENPY_VER-sdk.zip -d renpy_sdk
rm renpy-$RENPY_VER-sdk.zip

echo "Setup completed with Vita port adaptations!"

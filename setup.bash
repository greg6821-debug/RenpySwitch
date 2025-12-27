#!/bin/bash
set -e

export DEVKITPRO=/opt/devkitpro
export RENPY_VER=8.3.7
export PYGAME_SDL2_VER=2.1.0

apt-get -y update
apt-get -y upgrade

# host python (для pip, утилит)
export HOST_PYTHON=/usr/bin/python3.9
# switch python
export SWITCH_PYTHON=$DEVKITPRO/portlibs/switch/bin/python3.9
export SWITCH_PYTHON_INC=$DEVKITPRO/portlibs/switch/include/python3.9
export SWITCH_PYTHON_LIB=$DEVKITPRO/portlibs/switch/lib

# Установка системных зависимостей
apt -y install build-essential checkinstall
apt -y install libncursesw5-dev libssl-dev libsqlite3-dev tk-dev \
    libgdbm-dev libc6-dev libbz2-dev

# Установка Python 3.9 для хоста
apt -y install python3.9 python3.9-dev python3.9-distutils
curl -sS https://bootstrap.pypa.io/get-pip.py | python3.9

# Альтернативно, если python3.9 не доступен напрямую:
apt -y install python3 python3-dev python3-pip
python3 --version

# Установка зависимостей Ren'Py
apt-get -y install p7zip-full libsdl2-dev libsdl2-image-dev libjpeg-dev \
    libpng-dev libsdl2-ttf-dev libsdl2-mixer-dev libavformat-dev \
    libfreetype6-dev libswscale-dev libglew-dev libfribidi-dev \
    libavcodec-dev libswresample-dev libsdl2-gfx-dev libgl1-mesa-glx \
    libharfbuzz-dev

# Установка Python пакетов
python3 -m pip uninstall -y distribute
python3 -m pip install future six typing requests ecdsa pefile \
    Cython==3.0.12 setuptools wheel

# Установка devkitPro и зависимостей для Switch
curl -LOC - https://github.com/greg6821-debug/scripts/releases/download/1.0-scripts/devkitpro-pkgbuild-helpers-2.2.3-1-any.pkg.tar.xz
curl -LOC - https://github.com/knautilus/Utils/releases/download/v1.0/python39-switch.zip
curl -LOC - https://github.com/greg6821-debug/scripts/releases/download/1.0-scripts/switch-libfribidi-1.0.12-1-any.pkg.tar.xz

# Установка devkitPro пакетов
dkp-pacman -U --noconfirm devkitpro-pkgbuild-helpers-2.2.3-1-any.pkg.tar.xz
dkp-pacman -U --noconfirm switch-libfribidi-1.0.12-1-any.pkg.tar.xz

# Распаковка Python для Switch
unzip -qq python39-switch.zip -d $DEVKITPRO/portlibs/switch

# Очистка временных файлов
rm -f devkitpro-pkgbuild-helpers-2.2.3-1-any.pkg.tar.xz \
    switch-libfribidi-1.0.12-1-any.pkg.tar.xz \
    python39-switch.zip

# Исправление флагов для Switch
if [ -f "$DEVKITPRO/switch.cmake" ]; then
    sed -i.bak 's/set(CMAKE_EXE_LINKER_FLAGS_INIT "/set(CMAKE_EXE_LINKER_FLAGS_INIT "-fPIC /' $DEVKITPRO/switch.cmake
fi

# Загрузка Ren'Py и зависимостей
curl -LOC - https://www.renpy.org/dl/$RENPY_VER/pygame_sdl2-$PYGAME_SDL2_VER+renpy$RENPY_VER.tar.gz
curl -LOC - https://www.renpy.org/dl/$RENPY_VER/renpy-$RENPY_VER-sdk.zip
curl -LOC - https://www.renpy.org/dl/$RENPY_VER/renpy-$RENPY_VER-source.tar.bz2
curl -LOC - https://github.com/greg6821-debug/scripts/releases/download/1.0-scripts/rawproject.zip

# Обработка pygame_sdl2
rm -rf pygame_sdl2-$PYGAME_SDL2_VER+renpy$RENPY_VER pygame_sdl2-source
tar -xf pygame_sdl2-$PYGAME_SDL2_VER+renpy$RENPY_VER.tar.gz
mv pygame_sdl2-$PYGAME_SDL2_VER+renpy$RENPY_VER pygame_sdl2-source
rm pygame_sdl2-$PYGAME_SDL2_VER+renpy$RENPY_VER.tar.gz

# Обработка Ren'Py source
rm -rf renpy-$RENPY_VER-source renpy-source
tar -xf renpy-$RENPY_VER-source.tar.bz2
mv renpy-$RENPY_VER-source renpy-source
rm renpy-$RENPY_VER-source.tar.bz2

# Обработка Ren'Py SDK
rm -rf renpy-$RENPY_VER-sdk renpy_sdk
unzip -qq renpy-$RENPY_VER-sdk.zip -d renpy_sdk
rm renpy-$RENPY_VER-sdk.zip

# Обработка raw проекта
rm -rf raw
unzip -qq rawproject.zip -d raw
rm rawproject.zip

# Применение патчей (если есть)
if [ -f "renpy.patch" ]; then
    pushd renpy-source
    patch -p1 < ../renpy.patch
    popd
fi

# Очистка временных директорий
rm -rf renpy-source/module/gen renpy-source/module/gen-static
rm -rf pygame_sdl2-source/gen pygame_sdl2-source/gen-static

echo "Setup completed successfully!"
echo "Python version: $(python3 --version)"
echo "Pip version: $(python3 -m pip --version)"

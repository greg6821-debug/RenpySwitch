#!/bin/bash
set -e

export DEVKITPRO=/opt/devkitpro
export RENPY_VER=8.3.7
export PYGAME_SDL2_VER=2.1.0

# Проверка, запущен ли скрипт с правами root
if [ "$EUID" -ne 0 ]; then 
    echo "Пожалуйста, запустите этот скрипт с правами root (sudo)"
    exit 1
fi

# Обновление системы
apt-get -y update
apt-get -y upgrade

# Базовые инструменты и Python 3.9
apt -y install build-essential checkinstall git cmake ninja-build rsync
apt -y install python3.9 python3.9-dev python3.9-distutils python3-pip curl wget unzip p7zip-full

# Установка pip для Python 3.9
curl -sS https://bootstrap.pypa.io/get-pip.py | python3.9
python3.9 -m pip --version

# Установка Cython 0.29.x (обязательно, а не 3.0+)
python3.9 -m pip install --upgrade pip
python3.9 -m pip install "Cython==0.29.37"  # Критически важно!
python3.9 -m pip install future six typing requests ecdsa pefile setuptools wheel
python3.9 -m pip install pycryptodomex numpy

# Системные зависимости для компиляции хост-инструментов
apt-get -y install libsdl2-dev libsdl2-image-dev libjpeg-dev \
    libpng-dev libsdl2-ttf-dev libsdl2-mixer-dev libavformat-dev \
    libswscale-dev libglew-dev libfribidi-dev libharfbuzz-dev \
    libavcodec-dev libswresample-dev libsdl2-gfx-dev libgl1-mesa-glx \
    libwebp-dev libopenal-dev libvorbis-dev libopusfile-dev \
    libmpg123-dev libmodplug-dev libflac-dev \
    pkg-config autoconf automake libtool

# ========== УСТАНОВКА devkitPro ==========
echo "Установка devkitPro..."
if [ ! -d "$DEVKITPRO" ]; then
    # Установка devkitPro
    apt-get -y install lsb-release
    wget https://apt.devkitpro.org/install-devkitpro-pacman
    chmod +x install-devkitpro-pacman
    ./install-devkitpro-pacman
    
    # Удаление инсталлятора после установки
    rm install-devkitpro-pacman
else
    echo "devkitPro уже установлен в $DEVKITPRO"
fi

# Настройка переменных окружения devkitPro
source /etc/profile.d/devkit-env.sh 2>/dev/null || true

# ========== УСТАНОВКА Switch библиотек ==========
echo "Установка библиотек для Nintendo Switch..."

# Основные инструменты и библиотеки
dkp-pacman -S --noconfirm devkitA64 devkitpro-pkgbuild-helpers \
    switch-dev switch-tools switch-pkg-config switch-cmake \
    switch-libconfig switch-mesa switch-glad

# Python для Switch
dkp-pacman -S --noconfirm switch-python3 switch-python3-pip

# Графические библиотеки
dkp-pacman -S --noconfirm switch-sdl2 switch-sdl2_gfx switch-sdl2_image \
    switch-sdl2_mixer switch-sdl2_ttf switch-sdl2_net \
    switch-libdrm_nouveau switch-mesa switch-glad

# Аудио библиотеки
dkp-pacman -S --noconfirm switch-openal-soft switch-libvorbis \
    switch-libvorbisidec switch-libogg switch-opusfile \
    switch-libmpg123 switch-libmodplug switch-flac \
    switch-libmad switch-mikmod

# Медиа библиотеки
dkp-pacman -S --noconfirm switch-ffmpeg switch-libtheora \
    switch-libwebp switch-libjpeg-turbo switch-libpng

# Шрифты и текст
dkp-pacman -S --noconfirm switch-freetype switch-harfbuzz \
    switch-fribidi

# Дополнительные утилиты
dkp-pacman -S --noconfirm switch-zlib switch-bzip2 \
    switch-libxml2 switch-curl

# Проверка установленных библиотек
echo "Проверка установленных библиотек..."
ls -la /opt/devkitpro/portlibs/switch/lib/pkgconfig/ | grep -E "sdl2|openal|mpg123|modplug|opus|vorbis|ogg|harfbuzz"

# ========== НАСТРОЙКА Python для Switch ==========
echo "Настройка Python для Switch..."
if [ ! -f "$DEVKITPRO/portlibs/switch/lib/libpython3.11.a" ] && [ ! -f "$DEVKITPRO/portlibs/switch/lib/libpython3.9.a" ]; then
    echo "Python для Switch не найден. Установка..."
    
    # Попробуем найти альтернативный источник Python для Switch
    if [ -f "/opt/devkitpro/portlibs/switch/lib/libpython3.a" ]; then
        echo "Найден libpython3.a, создаём symlink для python3.9..."
        ln -sf /opt/devkitpro/portlibs/switch/lib/libpython3.a /opt/devkitpro/portlibs/switch/lib/libpython3.9.a
    else
        echo "Скачивание Python 3.9 для Switch..."
        mkdir -p /tmp/switch-python
        cd /tmp/switch-python
        
        # Альтернативные источники Python для Switch
        wget -q https://github.com/rashevskyv/switch/releases/download/python3.9/python39-switch.zip || \
        wget -q https://github.com/knautilus/Utils/releases/download/v1.0/python39-switch.zip || \
        wget -q https://github.com/fortheusers/switch-packages/releases/download/python-3.9.0/python-3.9.0-switch.zip
        
        if [ -f "python39-switch.zip" ]; then
            unzip -qq python39-switch.zip -d "$DEVKITPRO/portlibs/switch"
        elif [ -f "python-3.9.0-switch.zip" ]; then
            unzip -qq python-3.9.0-switch.zip -d "$DEVKITPRO/portlibs/switch"
        else
            echo "Не удалось скачать Python для Switch. Продолжаем без него..."
        fi
        
        cd -
        rm -rf /tmp/switch-python
    fi
fi

# ========== Исправление CMake флагов ==========
echo "Исправление CMake флагов для Switch..."
if [ -f "$DEVKITPRO/switch.cmake" ]; then
    # Создаем backup оригинального файла
    cp "$DEVKITPRO/switch.cmake" "$DEVKITPRO/switch.cmake.backup"
    
    # Исправляем флаги линковки
    sed -i 's/set(CMAKE_EXE_LINKER_FLAGS_INIT "/set(CMAKE_EXE_LINKER_FLAGS_INIT "-fPIC /g' "$DEVKITPRO/switch.cmake"
    
    # Добавляем поддержку статических библиотек Python
    echo "" >> "$DEVKITPRO/switch.cmake"
    echo "# Python поддержка" >> "$DEVKITPRO/switch.cmake"
    echo 'set(Python3_LIBRARIES ${PORTLIBS}/lib/libpython3.9.a)' >> "$DEVKITPRO/switch.cmake"
    echo 'set(Python3_INCLUDE_DIRS ${PORTLIBS}/include/python3.9)' >> "$DEVKITPRO/switch.cmake"
    echo 'set(PYTHON_LIBRARY ${PORTLIBS}/lib/libpython3.9.a)' >> "$DEVKITPRO/switch.cmake"
    echo 'set(PYTHON_INCLUDE_DIR ${PORTLIBS}/include/python3.9)' >> "$DEVKITPRO/switch.cmake"
    
    echo "Файл switch.cmake обновлен"
else
    echo "Файл switch.cmake не найден!"
fi

# ========== Скачивание Ren'Py ==========
echo "Скачивание Ren'Py $RENPY_VER..."

# Создаем рабочую директорию
WORKDIR=$(pwd)
mkdir -p "$WORKDIR/renpy-build"
cd "$WORKDIR/renpy-build"

# Скачиваем исходники
echo "Скачивание pygame_sdl2..."
wget -q --show-progress https://www.renpy.org/dl/$RENPY_VER/pygame_sdl2-$PYGAME_SDL2_VER+renpy$RENPY_VER.tar.gz || \
curl -LOC - https://www.renpy.org/dl/$RENPY_VER/pygame_sdl2-$PYGAME_SDL2_VER+renpy$RENPY_VER.tar.gz

echo "Скачивание Ren'Py SDK..."
wget -q --show-progress https://www.renpy.org/dl/$RENPY_VER/renpy-$RENPY_VER-sdk.zip || \
curl -LOC - https://www.renpy.org/dl/$RENPY_VER/renpy-$RENPY_VER-sdk.zip

echo "Скачивание исходников Ren'Py..."
wget -q --show-progress https://www.renpy.org/dl/$RENPY_VER/renpy-$RENPY_VER-source.tar.bz2 || \
curl -LOC - https://www.renpy.org/dl/$RENPY_VER/renpy-$RENPY_VER-source.tar.bz2

# Распаковка
echo "Распаковка файлов..."
rm -rf pygame_sdl2-source renpy-source renpy_sdk

tar -xf pygame_sdl2-$PYGAME_SDL2_VER+renpy$RENPY_VER.tar.gz
mv pygame_sdl2-$PYGAME_SDL2_VER+renpy$RENPY_VER pygame_sdl2-source
rm -f pygame_sdl2-$PYGAME_SDL2_VER+renpy$RENPY_VER.tar.gz

tar -xf renpy-$RENPY_VER-source.tar.bz2
mv renpy-$RENPY_VER-source renpy-source
rm -f renpy-$RENPY_VER-source.tar.bz2

unzip -qq renpy-$RENPY_VER-sdk.zip -d renpy_sdk
rm -f renpy-$RENPY_VER-sdk.zip

# ========== Настройка переменных окружения ==========
echo "Настройка переменных окружения..."
cat >> ~/.bashrc << EOF

# devkitPro для Nintendo Switch
export DEVKITPRO=/opt/devkitpro
export DEVKITARM=\$DEVKITPRO/devkitARM
export DEVKITA64=\$DEVKITPRO/devkitA64
export PATH=\$DEVKITPRO/tools/bin:\$PATH
export PATH=\$DEVKITA64/bin:\$PATH

# Python для Switch
export PYTHON_FOR_SWITCH=\$DEVKITPRO/portlibs/switch
export PKG_CONFIG_PATH=\$DEVKITPRO/portlibs/switch/lib/pkgconfig:\$PKG_CONFIG_PATH
export PKG_CONFIG_LIBDIR=\$DEVKITPRO/portlibs/switch/lib/pkgconfig
export PKG_CONFIG_EXECUTABLE=/opt/devkitpro/portlibs/switch/bin/aarch64-none-elf-pkg-config

# Ren'Py
export RENPY_VER=$RENPY_VER
export PYGAME_SDL2_VER=$PYGAME_SDL2_VER

# Для сборки
export ARCH=switch
export PLATFORM=switch
export TARGET=aarch64-none-elf
export CC=\$DEVKITA64/bin/aarch64-none-elf-gcc
export CXX=\$DEVKITA64/bin/aarch64-none-elf-g++
export AR=\$DEVKITA64/bin/aarch64-none-elf-ar
export RANLIB=\$DEVKITA64/bin/aarch64-none-elf-ranlib
export STRIP=\$DEVKITA64/bin/aarch64-none-elf-strip

alias switchenv='source /opt/devkitpro/switchvars.sh'
EOF

# Применяем изменения в текущей сессии
source ~/.bashrc
source /opt/devkitpro/switchvars.sh 2>/dev/null || true

# ========== ЗАВЕРШЕНИЕ ==========
echo "========================================"
echo "Установка завершена!"
echo ""
echo "Для работы с окружением Switch выполните:"
echo "  source ~/.bashrc"
echo "  source /opt/devkitpro/switchvars.sh"
echo ""
echo "Проверьте установку командой:"
echo "  aarch64-none-elf-gcc --version"
echo "  which aarch64-none-elf-pkg-config"
echo ""
echo "Перейдите в директорию renpy-build для начала работы"
echo "========================================"

cd "$WORKDIR"

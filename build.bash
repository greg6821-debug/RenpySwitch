#!/bin/bash
set -e

export DEVKITPRO=/opt/devkitpro

# Установка Python 3.9 для хоста (если еще не установлен)
if ! command -v python3.9 &> /dev/null; then
    echo "Installing Python 3.9..."
    apt-get update
    apt-get install -y python3.9 python3.9-dev python3.9-distutils
fi

# Компиляция pygame_sdl2 для хоста
echo "Building pygame_sdl2 for host..."
pushd pygame_sdl2-source
rm -rf gen gen-static build
python3.9 setup.py build_ext --inplace || true
PYGAME_SDL2_STATIC=1 python3.9 setup.py build_ext --inplace || true
popd

# Установка заголовочных файлов
echo "Installing headers..."
pushd pygame_sdl2-source
python3.9 setup.py install_headers
popd

# Создание символических ссылок
echo "Linking source files..."
bash link_sources.bash

# Компиляция renpy модулей для хоста
echo "Building Ren'Py modules for host..."
pushd renpy-source/module
rm -rf gen gen-static build
RENPY_DEPS_INSTALL=/usr/lib/x86_64-linux-gnu:/usr:/usr/local python3.9 setup.py build_ext --inplace || true
RENPY_DEPS_INSTALL=/usr/lib/x86_64-linux-gnu:/usr:/usr/local RENPY_STATIC=1 python3.9 setup.py build_ext --inplace || true
popd

# Компиляция для Switch
echo "Building for Switch..."
export PREFIXARCHIVE=$(realpath renpy-switch-modules.tar.gz)

rm -rf build-switch
mkdir build-switch
pushd build-switch
mkdir local_prefix
export LOCAL_PREFIX=$(realpath local_prefix)
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j$(nproc)
mkdir -p $LOCAL_PREFIX/lib
cp librenpy-switch-modules.a $LOCAL_PREFIX/lib/librenpy-switch-modules.a
popd

# Установка в devkitpro
echo "Installing to devkitpro..."
tar -czvf $PREFIXARCHIVE -C $LOCAL_PREFIX .
tar -xf renpy-switch-modules.tar.gz -C $DEVKITPRO/portlibs/switch
rm renpy-switch-modules.tar.gz
rm -rf build-switch

# Настройка переменных окружения Switch
source /opt/devkitpro/switchvars.sh

# Компиляция основного исполняемого файла Switch
echo "Building main Switch executable..."
pushd switch
rm -rf build
mkdir build
pushd build
cmake ..
make -j$(nproc)
popd
popd

# Подготовка структуры данных Ren'Py
echo "Preparing Ren'Py structure..."
mkdir -p ./raw/switch/exefs
mv ./switch/build/renpy-switch.nso ./raw/switch/exefs/main
rm -rf switch/build

# Очистка и подготовка Ren'Py SDK
echo "Cleaning Ren'Py SDK..."
rm -rf renpy_clear
mkdir renpy_clear

# Для Ren'Py 8 структура SDK может отличаться
# Ищем правильный путь
if [ -d "./renpy_sdk/renpy-8.3.7-sdk" ]; then
    RENPY_SDK_DIR="./renpy_sdk/renpy-8.3.7-sdk"
elif [ -d "./renpy_sdk/renpy-8.3.7" ]; then
    RENPY_SDK_DIR="./renpy_sdk/renpy-8.3.7"
else
    # Берем первый найденный каталог
    RENPY_SDK_DIR=$(find ./renpy_sdk -maxdepth 1 -type d | head -2 | tail -1)
fi

echo "Using Ren'Py SDK from: $RENPY_SDK_DIR"

cp "$RENPY_SDK_DIR/renpy.sh" ./renpy_clear/renpy.sh
cp -r "$RENPY_SDK_DIR/lib" ./renpy_clear/lib
mkdir ./renpy_clear/game
cp -r ./renpy-source/renpy ./renpy_clear/renpy
cp ./renpy-source/renpy.py ./renpy_clear/renpy.py
cp ./script.rpy ./renpy_clear/game/script.rpy

# Копируем исполняемые файлы (если есть)
find "$RENPY_SDK_DIR" -name "*.exe" -exec cp {} ./renpy_clear/ \;

# Удаляем ненужные файлы
rm -rf ./renpy_clear/lib/*mac*

# Компиляция скриптов Ren'Py
echo "Compiling Ren'Py scripts..."
pushd renpy_clear
# Используем Python 3.9 для компиляции
python3.9 ./renpy.py --compile . compile
find ./renpy/ -regex ".*\.\(pxd\|pyx\|rpym\|pxi\)" -delete
popd

# Подготовка приватных данных
echo "Preparing private data..."
rm -rf private
mkdir private
mkdir private/lib

# Копируем Python 3.9 библиотеки
cp -r renpy_clear/lib/python3.9/ private/lib/
cp -r renpy_clear/renpy private/renpy
cp renpy_clear/renpy.py private/main.py
rm -rf private/renpy/common

# Генерация приватных данных (если нужен скрипт)
if [ -f "generate_private.py" ]; then
    echo "Generating private data..."
    python3.9 generate_private.py
fi

rm -rf private

# Создание окончательной структуры для Switch
echo "Creating final Switch structure..."
mkdir -p ./raw/switch/romfs/Contents/renpy/common
mkdir -p ./raw/switch/romfs/Contents/renpy
mkdir -p ./raw/lib

# Копирование общих файлов
cp -r ./renpy_clear/renpy/common ./raw/switch/romfs/Contents/renpy/

# Копирование основного скрипта
cp ./renpy_clear/renpy.py ./raw/switch/romfs/Contents/

# Распаковка lib.zip если существует
if [ -f "./raw/lib.zip" ]; then
    unzip -qq ./raw/lib.zip -d ./raw/lib/
    rm ./raw/lib.zip
fi

# Копирование библиотек Ren'Py
cp -r ./renpy_clear/renpy/ ./raw/lib/renpy/
rm -rf ./raw/lib/renpy/common/

# Создание архива библиотек
if [ -d "./raw/lib" ]; then
    7z a -tzip ./raw/switch/romfs/Contents/lib.zip ./raw/lib/*
    rm -rf ./raw/lib
fi

# Очистка
rm -rf ./renpy_clear/game

# Создание финального архива
echo "Creating final archive..."
7z a -tzip raw.zip ./raw/*

echo "Build completed successfully!"

#!/bin/bash
set -e

export DEVKITPRO=/opt/devkitpro

# ========== СБОРКА ДЛЯ ХОСТА (без переменных окружения Switch) ==========
echo "=== Сборка хостовых модулей ==="

# Сохраняем текущие переменные окружения
OLD_PATH="$PATH"
OLD_CC="$CC"
OLD_CXX="$CXX"
OLD_CFLAGS="$CFLAGS"
OLD_LDFLAGS="$LDFLAGS"

# Очищаем переменные окружения devkitPro для хостовой сборки
export PATH=$(echo "$PATH" | sed 's|/opt/devkitpro/[^:]*:||g' | sed 's|:/opt/devkitpro/[^:]*||g')
unset CC
unset CXX
unset CFLAGS
unset LDFLAGS
unset PKG_CONFIG_PATH
unset PKG_CONFIG_LIBDIR


# --- FIX: запрет Py_UNICODE для Cython (Python 3.9+) ---
export CFLAGS="-DCYTHON_USE_PYTHON_UNICODE=0"
export CPPFLAGS="-DCYTHON_USE_PYTHON_UNICODE=0"


# Проверяем, находимся ли мы в правильной директории
if [ ! -d "pygame_sdl2-source" ] && [ -d "renpy-build" ]; then
    echo "Переход в директорию renpy-build..."
    cd renpy-build
fi

# Проверяем, существует ли директория pygame_sdl2-source
if [ ! -d "pygame_sdl2-source" ]; then
    echo "Ошибка: Директория pygame_sdl2-source не найдена!"
    echo "Текущая директория: $(pwd)"
    echo "Содержимое:"
    ls -la
    exit 1
fi

# Установка Python 3.9 для хоста (если еще не установлен)
if ! command -v python3.9 &> /dev/null; then
    echo "Installing Python 3.9..."
    apt-get update
    apt-get install -y python3.9 python3.9-dev python3.9-distutils
fi

echo "Cython version:"
python3.9 -m cython --version
which cython
cython --version

# Компиляция pygame_sdl2 для хоста
echo "Building pygame_sdl2 for host..."
pushd pygame_sdl2-source
rm -rf gen3 gen3-static build
CFLAGS="$CFLAGS" CPPFLAGS="$CPPFLAGS" \
python3.9 setup.py build_ext --inplace || true

CFLAGS="$CFLAGS" CPPFLAGS="$CPPFLAGS" \
PYGAME_SDL2_STATIC=1 python3.9 setup.py build_ext --inplace || true
popd

# Установка заголовочных файлов
echo "Installing headers..."
pushd pygame_sdl2-source
python3.9 setup.py install_headers
popd

# Компиляция renpy модулей для хоста
echo "Building Ren'Py modules for host..."
pushd renpy-source/module
rm -rf gen3 gen3-static build

# СОЗДАЕМ ДИРЕКТОРИЮ ПЕРЕД СБОРКОЙ
mkdir -p renpy/audio
mkdir -p renpy/styledata
mkdir -p renpy/display
mkdir -p renpy/uguu
mkdir -p renpy/gl
mkdir -p renpy/gl2
mkdir -p renpy/text


CFLAGS="$CFLAGS" CPPFLAGS="$CPPFLAGS" \
RENPY_DEPS_INSTALL=/usr/lib/x86_64-linux-gnu:/usr:/usr/local \
python3.9 setup.py build_ext --inplace || true

CFLAGS="$CFLAGS" CPPFLAGS="$CPPFLAGS" \
RENPY_DEPS_INSTALL=/usr/lib/x86_64-linux-gnu:/usr:/usr/local \
RENPY_STATIC=1 python3.9 setup.py build_ext --inplace || true
popd

# Создание символических ссылок
echo "Linking source files..."
bash link_sources.bash

echo "=== Контрольная проверка  Py_UNICODE ==="
grep -R "Py_UNICODE" renpy-source/module/gen3
grep -R "Py_UNICODE" pygame_sdl2-source/gen3

echo "=== Сборка для Nintendo Switch ==="

# Восстанавливаем переменные окружения devkitPro
export PATH="$OLD_PATH"
export CC="$OLD_CC"
export CXX="$OLD_CXX"
export CFLAGS="$OLD_CFLAGS"
export LDFLAGS="$OLD_LDFLAGS"

export DEVKITPRO=/opt/devkitpro

# ЗАГРУЖАЕМ ПЕРЕМЕННЫЕ ОКРУЖЕНИЯ devkitPro ПЕРЕД НАЧАЛОМ
if [ -f "/opt/devkitpro/switchvars.sh" ]; then
    echo "Загрузка переменных окружения Switch..."
    source /opt/devkitpro/switchvars.sh
else
    echo "Ошибка: switchvars.sh не найден!"
    exit 1
fi

# Компиляция для Switch
echo "Building for Switch..."
SWITCH_CMAKE="$DEVKITPRO/cmake/Switch.cmake"
# ========== Исправление CMake флагов ==========
echo "Исправление CMake флагов для Switch..."
if [ -f "$DEVKITPRO/cmake/Switch.cmake" ]; then
    # Создаем backup оригинального файла
    cp "$DEVKITPRO/cmake/Switch.cmake" "$DEVKITPRO/cmake/Switch.cmake.backup"
    
    # Исправляем флаги линковки
    sed -i 's/set(CMAKE_EXE_LINKER_FLAGS_INIT "/set(CMAKE_EXE_LINKER_FLAGS_INIT "-fPIC /g' "$DEVKITPRO/cmake/Switch.cmake"
    
    # Добавляем поддержку статических библиотек Python
    echo "" >> "$DEVKITPRO/cmake/Switch.cmake"
    echo "# Python поддержка" >> "$DEVKITPRO/cmake/Switch.cmake"
    echo 'set(Python3_LIBRARIES ${PORTLIBS}/lib/libpython3.9.a)' >> "$DEVKITPRO/cmake/Switch.cmake"
    echo 'set(Python3_INCLUDE_DIRS ${PORTLIBS}/include/python3.9)' >> "$DEVKITPRO/cmake/Switch.cmake"
    echo 'set(PYTHON_LIBRARY ${PORTLIBS}/lib/libpython3.9.a)' >> "$DEVKITPRO/cmake/Switch.cmake"
    echo 'set(PYTHON_INCLUDE_DIR ${PORTLIBS}/include/python3.9)' >> "$DEVKITPRO/cmake/Switch.cmake"
    
    echo "Файл switch.cmake обновлен"
else
    echo "Файл switch.cmake не найден!"
fi



export LD_LIBRARY_PATH=$DEVKITPRO/portlibs/switch/lib:$LD_LIBRARY_PATH
export PYTHONPATH=$DEVKITPRO/portlibs/switch/lib/python3.9:$PYTHONPATH
export C_INCLUDE_PATH=$DEVKITPRO/portlibs/switch/include:$C_INCLUDE_PATH
export CPLUS_INCLUDE_PATH=$DEVKITPRO/portlibs/switch/include:$CPLUS_INCLUDE_PATH

export PREFIXARCHIVE=$(realpath renpy-switch-modules.tar.gz)



# Копируем .c файлы из pygame_sdl2-source
rsync -avm --include='*/' --include='*.c' --exclude='*' pygame_sdl2-source/ source/module
# Копируем .c файлы из renpy-source/module
rsync -avm --include='*/' --include='*.c' --exclude='*' renpy-source/module/ source/module
# Перемещаем все файлы из поддиректорий в source/module, не перезаписывая существующие
find source/module -mindepth 2 -type f -exec mv -n -t source/module {} +
# Удаляем пустые директории
find source/module -type d -empty -delete
# Копируем .h файлы из pygame_sdl2-source
rsync -avm --include='*/' --include='*.h' --exclude='*' pygame_sdl2-source/ include/module/pygame_sdl2
# Перемещаем все .h файлы из поддиректорий, не перезаписывая существующие
find include/module/pygame_sdl2 -mindepth 2 -type f -exec mv -n -t include/module/pygame_sdl2 {} +
# Перемещаем surface.h
mv include/module/pygame_sdl2/surface.h include/module/pygame_sdl2/src
# Копируем .h файлы из renpy-source/module
rsync -avm --include='*/' --include='*.h' --exclude='*' renpy-source/module/ include/module
# Перемещаем hydrogen.c
mv source/module/hydrogen.c include/module/libhydrogen



rm -rf build-switch
mkdir build-switch
pushd build-switch
mkdir local_prefix
export LOCAL_PREFIX=$(realpath local_prefix)

# ИСПОЛЬЗУЕМ ПРАВИЛЬНЫЙ CMAKE С TOOLCHAIN
echo "Запуск CMake с toolchain: $SWITCH_CMAKE"
cmake -DCMAKE_TOOLCHAIN_FILE="$SWITCH_CMAKE" -DCMAKE_BUILD_TYPE=Release ..

# ПРОВЕРЯЕМ, ЧТО MAKE УСТАНОВЛЕН
if ! command -v make &> /dev/null; then
    echo "Установка make..."
    apt-get update
    apt-get install -y make
fi

#cmake -DCMAKE_BUILD_TYPE=Release ..
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

# Проверяем наличие директории switch
if [ -d "switch" ]; then
    # Компиляция основного исполняемого файла Switch
    echo "Building main Switch executable..."
    pushd switch
    rm -rf build
    mkdir build
    pushd build
    cmake -DCMAKE_TOOLCHAIN_FILE="$SWITCH_CMAKE" ..
    make -j$(nproc)
    popd
    popd
    
    # Подготовка структуры данных Ren'Py
    echo "Preparing Ren'Py structure..."
    mkdir -p ./raw/switch/exefs
    if [ -f "switch/build/renpy-switch.nso" ]; then
        mv switch/build/renpy-switch.nso ./raw/switch/exefs/main
    fi
    rm -rf switch/build
else
    echo "Директория 'switch' не найдена, пропускаем создание исполняемого файла"
fi

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

if [ -z "$RENPY_SDK_DIR" ] || [ ! -d "$RENPY_SDK_DIR" ]; then
    echo "Ошибка: Не удалось найти директорию Ren'Py SDK!"
    exit 1
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

mv raw.zip ..

echo "Build completed successfully!"

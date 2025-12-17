set -e  # Останавливаться при ошибках

echo "=== [1] Настройка окружения ==="
export DEVKITPRO=/opt/devkitpro
echo "DEVKITPRO установлен: $DEVKITPRO"

echo "=== [1.1] Сборка pygame_sdl2 ==="
if [ ! -d "pygame_sdl2-source" ]; then
    echo "❌ Ошибка: pygame_sdl2-source не найден!"
    exit 1
fi

pushd pygame_sdl2-source
echo "Текущая директория: $(pwd)"

echo "=== [1.2] Очистка старых файлов ==="
rm -rf gen gen-static build dist *.egg-info
find . -name "*.pyc" -delete
find . -name "*.so" -delete
echo "Очистка завершена"

echo "=== [1.3] Обычная сборка pygame_sdl2 ==="
RENPY_DEPS_INSTALL=/usr/lib/x86_64-linux-gnu:/usr:/usr/local \
python2 setup.py build_ext --inplace 2>&1 | tee build.log || {
    echo "❌ Ошибка сборки pygame_sdl2"
    exit 1
}
echo "✅ pygame_sdl2 собран локально"

echo "=== [1.4] Статическая сборка и установка ==="
# Сначала проверим, существует ли setup.py
if [ ! -f "setup.py" ]; then
    echo "❌ setup.py не найден!"
    exit 1
fi

# Установка с принудительной перезаписью
PYGAME_SDL2_STATIC=1 \
RENPY_DEPS_INSTALL=/usr/lib/x86_64-linux-gnu:/usr:/usr/local \
python2 setup.py install --force 2>&1 | tee install.log || {
    echo "❌ Ошибка установки pygame_sdl2"
    exit 1
}
echo "✅ pygame_sdl2 установлен"
popd
echo "Возврат в: $(pwd)"

echo -e "\n=== [2] Сборка Ren'Py модулей ==="
if [ ! -d "renpy-source/module" ]; then
    echo "❌ Ошибка: renpy-source/module не найден!"
    exit 1
fi

pushd renpy-source/module
echo "Текущая директория: $(pwd)"

echo "=== [2.1] Очистка Ren'Py ==="
rm -rf gen gen-static build
echo "Очистка завершена"

echo "=== [2.1.1] Поиск заголовков pygame_sdl2 ==="
echo "Заголовочные файлы pygame_sdl2:"
find /usr -name "pygame_sdl2.h" 2>/dev/null | head -5 || echo "   Не найдены"

echo "=== [2.1.3] Подготовка заголовочных файлов ==="

# Проверяем несколько возможных мест
HEADER_FOUND=false
HEADER_PATHS=(
    "../pygame_sdl2-source/src/pygame_sdl2/pygame_sdl2.h"
    "../pygame_sdl2-source/pygame_sdl2/pygame_sdl2.h"
    "pygame_sdl2-source/src/pygame_sdl2/pygame_sdl2.h"
    "../../pygame_sdl2-source/src/pygame_sdl2/pygame_sdl2.h"
)

for path in "${HEADER_PATHS[@]}"; do
    if [ -f "$path" ]; then
        echo "✅ Найден pygame_sdl2.h: $path"
        HEADER_FOUND=true
        HEADER_PATH="$path"
        break
    fi
done

if [ "$HEADER_FOUND" = false ]; then
    echo "🔍 Поиск в файловой системе..."
    # Рекурсивный поиск
    find .. -name "pygame_sdl2.h" 2>/dev/null | head -5 | while read found; do
        echo "   Найден: $found"
        HEADER_FOUND=true
        HEADER_PATH="$found"
    done
    
    if [ "$HEADER_FOUND" = false ]; then
        echo "🔍 Глубокий поиск в исходниках..."
        # Поиск в исходной распакованной структуре
        if [ -d "../pygame_sdl2-source" ]; then
            find ../pygame_sdl2-source -type f -name "*.h" | head -10 | while read file; do
                echo "   Файл: $(basename "$file") -> $file"
                if [[ "$(basename "$file")" == "pygame_sdl2.h" ]]; then
                    HEADER_PATH="$file"
                    HEADER_FOUND=true
                fi
            done
        fi
    fi
fi

if [ "$HEADER_FOUND" = true ] && [ -n "$HEADER_PATH" ]; then
    echo "📁 Создание структуры заголовков..."
    
    # Создаем директорию для заголовков
    mkdir -p pygame_sdl2
    
    # Копируем основной заголовочный файл
    cp "$HEADER_PATH" pygame_sdl2/
    echo "✅ Скопирован: $(basename "$HEADER_PATH")"
    
    # Копируем ВСЕ заголовочные файлы из той же директории
    HEADER_DIR=$(dirname "$HEADER_PATH")
    echo "📂 Копирование файлов из: $HEADER_DIR"
    
    # Копируем все .h файлы
    find "$HEADER_DIR" -name "*.h" -exec cp {} pygame_sdl2/ \; 2>/dev/null
    echo "✅ Скопировано .h файлов: $(ls -1 pygame_sdl2/*.h 2>/dev/null | wc -l)"
    
    # Копируем все .pxd файлы (Cython)
    find "$HEADER_DIR" -name "*.pxd" -exec cp {} pygame_sdl2/ \; 2>/dev/null
    echo "✅ Скопировано .pxd файлов: $(ls -1 pygame_sdl2/*.pxd 2>/dev/null | wc -l)"
    
    # Проверяем содержимое
    echo "📋 Содержимое pygame_sdl2/:"
    ls -la pygame_sdl2/ 2>/dev/null || echo "   Директория пуста"
    
    # Если директория пуста, создаем минимальный заголовок
    if [ ! -f "pygame_sdl2/pygame_sdl2.h" ] && [ -f "$HEADER_PATH" ]; then
        echo "⚠️  Создание минимального заголовка..."
        cp "$HEADER_PATH" pygame_sdl2/pygame_sdl2.h
    fi
else
    echo "⚠️  pygame_sdl2.h не найден, пытаемся найти альтернативы..."
    
    # Ищем любые заголовочные файлы SDL
    echo "🔍 Поиск SDL заголовков:"
    find /usr -name "*SDL*.h" 2>/dev/null | grep -i sdl | head -5
    
    # Создаем минимальный заголовочный файл
    echo "📝 Создание заглушки pygame_sdl2.h..."
    mkdir -p pygame_sdl2
    cat > pygame_sdl2/pygame_sdl2.h << 'EOF'
/* Заглушка для pygame_sdl2.h */
#ifndef PYGAME_SDL2_H
#define PYGAME_SDL2_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>

/* Базовые определения */
typedef struct {
    int dummy;
} PygameSdl2Context;

#endif /* PYGAME_SDL2_H */
EOF
    echo "✅ Создана заглушка pygame_sdl2.h"
fi

# Дополнительно: проверяем структуру pygame_sdl2-source
echo "🔍 Проверка структуры pygame_sdl2-source..."
if [ -d "../pygame_sdl2-source" ]; then
    echo "Структура ../pygame_sdl2-source/:"
    find "../pygame_sdl2-source" -type f -name "*.h" -o -name "*.pxd" | head -20 | while read file; do
        echo "   $(basename "$file") -> ${file#../pygame_sdl2-source/}"
    done
    
    # Если нашли src директорию
    if [ -d "../pygame_sdl2-source/src" ]; then
        echo "Копирование из src/..."
        # Копируем всю структуру src/pygame_sdl2
        if [ -d "../pygame_sdl2-source/src/pygame_sdl2" ]; then
            cp -r "../pygame_sdl2-source/src/pygame_sdl2/"* pygame_sdl2/ 2>/dev/null || true
        fi
    fi
    
    # Альтернатива: создаем симлинк
    echo "🔗 Создание симлинка на исходники..."
    if [ -d "../pygame_sdl2-source/src/pygame_sdl2" ]; then
        ln -sf ../pygame_sdl2-source/src/pygame_sdl2 .
        echo "✅ Симлинк создан: pygame_sdl2 -> ../pygame_sdl2-source/src/pygame_sdl2"
    elif [ -d "../pygame_sdl2-source/pygame_sdl2" ]; then
        ln -sf ../pygame_sdl2-source/pygame_sdl2 .
        echo "✅ Симлинк создан: pygame_sdl2 -> ../pygame_sdl2-source/pygame_sdl2"
    fi
fi

echo "✅ Подготовка заголовочных файлов завершена"

echo "=== [2.2] Обычная сборка Ren'Py ==="
# Проверяем существование setup.py
if [ ! -f "setup.py" ]; then
    echo "❌ setup.py не найден в $(pwd)"
    exit 1
fi

# Добавляем пути к заголовкам pygame_sdl2
PYGAME_INCLUDE=""
if [ -d "../pygame_sdl2-source" ]; then
    PYGAME_INCLUDE="-I../pygame_sdl2-source"
fi

echo "Используемые переменные:"
echo "  RENPY_DEPS_INSTALL: /usr/lib/x86_64-linux-gnu:/usr:/usr/local"
echo "  PYGAME_INCLUDE: $PYGAME_INCLUDE"

RENPY_DEPS_INSTALL=/usr/lib/x86_64-linux-gnu:/usr:/usr/local \
CFLAGS="$PYGAME_INCLUDE" \
python2 setup.py build_ext --inplace 2>&1 | tee renpy_build.log || {
    echo "❌ Ошибка сборки Ren'Py"
    echo "Последние строки лога:"
    tail -20 renpy_build.log
    exit 1
}
echo "✅ Ren'Py собран"

echo "=== [2.3] Статическая сборка Ren'Py ==="
# Устанавливаем Ren'Py модули
RENPY_DEPS_INSTALL=/usr/lib/x86_64-linux-gnu:/usr:/usr/local \
RENPY_STATIC=1 \
CFLAGS="$PYGAME_INCLUDE" \
python2 setup.py install --force 2>&1 | tee renpy_install.log || {
    echo "❌ Ошибка установки Ren'Py"
    echo "Последние строки лога:"
    tail -20 renpy_install.log
    exit 1
}
echo "✅ Ren'Py установлен"
popd
echo -e "\n=== Сборка завершена успешно! ==="
echo "Итоговое местоположение файлов:"
echo "  • pygame_sdl2: $(python2 -c 'import pygame_sdl2; print(pygame_sdl2.__file__)' 2>/dev/null || echo 'не установлен')"
echo "  • renpy модули: /usr/local/lib/python2.7/dist-packages/renpy/"
popd
echo "=== ---3--- ==="
pushd pygame_sdl2-source
echo "=== ---3.1--- ==="
python2 setup.py build
echo "=== ---3.2--- ==="
python2 setup.py install_headers
echo "=== ---3.3--- ==="
python2 setup.py install
popd
echo "=== ---4--- ==="
pushd renpy-source/module
echo "=== ---4.1--- ==="
RENPY_DEPS_INSTALL=/usr/lib/x86_64-linux-gnu:/usr:/usr/local python2 setup.py build
echo "=== ---4.2--- ==="
RENPY_DEPS_INSTALL=/usr/lib/x86_64-linux-gnu:/usr:/usr/local python2 setup.py install
echo "=== ---4.3--- ==="
popd
echo "=== ---5--- ==="

bash link_sources.bash

export PREFIXARCHIVE=$(realpath renpy-switch-modules.tar.gz)

rm -rf build-switch
mkdir build-switch

sed -i 's|#include <.*fribidi.h.*>|#include "fribidi.h"|' renpy-source/module/renpybidicore.c 

pushd build-switch
mkdir local_prefix
export LOCAL_PREFIX=$(realpath local_prefix)
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
mkdir -p $LOCAL_PREFIX/lib
cp librenpy-switch-modules.a $LOCAL_PREFIX/lib/librenpy-switch-modules.a
popd

tar -czvf $PREFIXARCHIVE -C $LOCAL_PREFIX .
tar -xf renpy-switch-modules.tar.gz -C $DEVKITPRO/portlibs/switch
rm renpy-switch-modules.tar.gz
rm -rf build-switch

source /opt/devkitpro/switchvars.sh

pushd switch
rm -rf build
mkdir build
pushd build
cmake ..
make
popd
popd

mkdir -p ./raw/switch/exefs
mv ./switch/build/renpy-switch.nso ./raw/switch/exefs/main
rm -rf switch include source pygame_sdl2-source


rm -rf renpy_clear
mkdir renpy_clear
#cp -r ./renpy_sdk/*/renpy ./renpy_clear/renpy

cp ./renpy_sdk/*/renpy.sh ./renpy_clear/renpy.sh
cp -r ./renpy_sdk/*/lib ./renpy_clear/lib
mkdir ./renpy_clear/game
cp -r ./renpy-source/module ./renpy_clear/module
cp -af ./renpy-source/renpy/ ./renpy_clear/renpy/
cp ./renpy-source/renpy.py ./renpy_clear/renpy.py
mv ./script.rpy ./renpy_clear/game/script.rpy
cp ./renpy_sdk/*/*.exe ./renpy_clear/
rm -rf renpy-source renpy_sdk ./renpy_clear/lib/*mac*


pushd renpy_clear
./renpy.sh --compile . compile
find ./renpy/ -regex ".*\.\(pxd\|pyx\|rpym\|pxi\)" -delete  # py\|rpy\| ???
popd


rm -rf private
mkdir private
mkdir private/lib
cp -r renpy_clear/renpy private/renpy
cp -r renpy_clear/lib/python2.7/ private/lib/
cp renpy_clear/renpy.py private/main.py
rm -rf private/renpy/common
python2 generate_private.py
rm -rf private


mkdir -p ./raw/switch/romfs/Contents/renpy/common
mkdir -p ./raw/switch/romfs/Contents/renpy
mkdir -p ./raw/lib
#mkdir -p ./raw/android/assets/renpy/common
cp -r ./renpy_clear/renpy/common ./raw/switch/romfs/Contents/renpy/
#cp -r ./renpy_clear/renpy/common ./raw/android/assets/renpy/
#mv private.mp3 ./raw/android/assets
cp ./renpy_clear/renpy.py ./raw/switch/romfs/Contents/
unzip -qq ./raw/lib.zip -d ./raw/lib/
rm ./raw/lib.zip

#cp -r ./renpy_clear/lib/python2.7/. ./raw/lib
cp -r ./renpy_clear/renpy ./raw/lib/renpy/
rm -rf ./raw/lib/renpy/common/
7z a -tzip ./raw/switch/romfs/Contents/lib.zip ./raw/lib/*
rm -rf ./raw/lib
#rm ./renpy_clear/*.txt
rm -rf ./renpy_clear/game
mv ./renpy_clear/ ./raw/renpy_clear/
#rm -rf ./renpy_clear
7z a -tzip raw.zip ./raw/*

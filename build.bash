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
python2 setup.py build
# Проверяем, что заголовки появились
ls gen | grep rwobject_api.h
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

echo "=== [2.1.3] Создание правильной структуры директорий ==="

# Создаем вложенную структуру pygame_sdl2/pygame_sdl2/
rm -rf pygame_sdl2
mkdir -p pygame_sdl2/pygame_sdl2

HEADER_DIR="../../pygame_sdl2-source/src/pygame_sdl2"

if [ -d "$HEADER_DIR" ]; then
    echo "Копирование файлов в pygame_sdl2/pygame_sdl2/..."
    
    # Копируем все файлы во вложенную директорию
    cp -r "$HEADER_DIR/"* pygame_sdl2/pygame_sdl2/ 2>/dev/null
    # Простое копирование всех файлов
    cp -r ../../pygame_sdl2-source/gen/* pygame_sdl2/pygame_sdl2/ 2>/dev/null
    
    # Создаем основной .h файл на верхнем уровне
    if [ -f "pygame_sdl2/pygame_sdl2/pygame_sdl2.h" ]; then
        echo "Создание ссылки на верхнем уровне..."
        ln -sf pygame_sdl2/pygame_sdl2.h pygame_sdl2/pygame_sdl2.h
    fi
    
    echo "✅ Структура создана:"
    echo "  pygame_sdl2/pygame_sdl2.h -> ссылка"
    echo "  pygame_sdl2/pygame_sdl2/  -> исходные файлы"
    
    ls -la pygame_sdl2/
    ls -la pygame_sdl2/pygame_sdl2/ | head -10
else
    echo "❌ Исходная директория не найдена"
fi





echo "=== [2.1.9] Диагностика заголовков pygame_sdl2 ==="

PYGAME_SRC="../../pygame_sdl2-source"
GEN_DIR="$PYGAME_SRC/gen"
SRC_DIR="$PYGAME_SRC/src/pygame_sdl2"

echo "--- Проверка директорий ---"
for d in "$PYGAME_SRC" "$GEN_DIR" "$SRC_DIR"; do
    if [ -d "$d" ]; then
        echo "✅ $d существует"
    else
        echo "❌ $d НЕ существует"
    fi
done

echo
echo "--- Содержимое gen/ ---"
if [ -d "$GEN_DIR" ]; then
    ls -la "$GEN_DIR"
else
    echo "gen/ отсутствует"
fi

echo
echo "--- Поиск *_api.h ---"
find "$PYGAME_SRC" -name "*_api.h" 2>/dev/null || echo "❌ *_api.h не найдены"

echo
echo "--- Поиск rwobject_api.h ---"
find "$PYGAME_SRC" -name "pygame_sdl2.rwobject_api.h" 2>/dev/null \
    || echo "❌ pygame_sdl2.rwobject_api.h не найден"

echo
echo "--- Поиск surface_api.h ---"
find "$PYGAME_SRC" -name "pygame_sdl2.surface_api.h" 2>/dev/null \
    || echo "❌ pygame_sdl2.surface_api.h не найден"

echo
echo "--- Поиск display_api.h ---"
find "$PYGAME_SRC" -name "pygame_sdl2.display_api.h" 2>/dev/null \
    || echo "❌ pygame_sdl2.display_api.h не найден"

echo
echo "--- Проверка include-пути для gcc ---"
TEST_INCLUDE="$GEN_DIR/pygame_sdl2.rwobject_api.h"
if [ -f "$TEST_INCLUDE" ]; then
    echo "✅ gcc сможет включить: $TEST_INCLUDE"
else
    echo "❌ gcc НЕ сможет включить: $TEST_INCLUDE"
fi

echo
echo "--- Проверка include из pygame_sdl2.h ---"
MAIN_H="$SRC_DIR/pygame_sdl2.h"
if [ -f "$MAIN_H" ]; then
    echo "Найден $MAIN_H"
    echo "Включаемые заголовки:"
    grep '#include "pygame_sdl2/.*_api.h"' "$MAIN_H" || echo "  (include не найдены)"
else
    echo "❌ pygame_sdl2.h не найден"
fi

echo "=== [2.1.9] Конец диагностики ==="
echo


echo "=== [ДИАГНОСТИКА] Проверка модуля renpy.compat.dictviews ==="

# Проверяем текущую директорию
echo "Текущая директория: $(pwd)"
echo "Содержимое:"
ls -la

# Ищем файл dictviews
echo -e "\n🔍 Поиск файла dictviews:"
find . -name "*dictviews*" -type f 2>/dev/null
find .. -name "*dictviews*" -type f 2>/dev/null
find ../.. -name "*dictviews*" -type f 2>/dev/null

# Проверяем структуру renpy
echo -e "\n📁 Структура renpy-source:"
if [ -d "../.." ]; then
    find "../.." -path "*renpy*" -name "*dictviews*" 2>/dev/null
fi

# Проверяем установленные модули
echo -e "\n📦 Проверка установленных модулей Python:"
python2 -c "
import sys
print('Python путь:')
for p in sys.path:
    print('  ' + p)

print('\nПоиск renpy...')
try:
    import renpy
    print('renpy найден:', renpy.__file__)
except:
    print('renpy не найден')

print('\nПопытка импорта dictviews...')
try:
    import renpy.compat.dictviews
    print('✅ dictviews импортирован успешно')
except ImportError as e:
    print('❌ Ошибка импорта:', e)
    import traceback
    traceback.print_exc()
"

# Проверяем .pyx и .c файлы
echo -e "\n🔧 Поиск исходных файлов dictviews:"
find . -name "*dictviews*" -o -name "*dictviews.*" 2>/dev/null

# Проверяем сгенерированные файлы
echo -e "\n📂 Содержимое gen и gen-static:"
for dir in gen gen-static; do
    if [ -d "$dir" ]; then
        echo "Директория $dir:"
        find "$dir" -name "*dictviews*" 2>/dev/null
        echo "Все файлы в $dir (первые 20):"
        find "$dir" -type f 2>/dev/null | head -20
    else
        echo "Директория $dir не существует"
    fi
done


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

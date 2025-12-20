set -e

export DEVKITPRO=/opt/devkitpro

# 1. Сборка Python с поддержкой динамических библиотек
if [ ! -d "python-build" ]; then
    mkdir -p python-build
    pushd python-build
    echo "=== Текущая директория: $(pwd) ==="
    
    # Скачивание Python 3.9.22
    if [ ! -f "Python-3.9.22.tgz" ]; then
        wget https://www.python.org/ftp/python/3.9.22/Python-3.9.22.tgz
    fi
    
    # Распаковка
    tar -xzf Python-3.9.22.tgz
    cd Python-3.9.22
    
    echo "=== Конфигурация Python в: $(pwd) ==="
    
    # Конфигурация с shared библиотеками
    ./configure --prefix=$PWD/install --enable-shared --disable-ipv6 \
        --without-ensurepip --with-system-ffi --with-system-expat
    
    echo "=== Начало сборки Python ==="
    
    # Компиляция
    make
    
    echo "=== Поиск lib-dynload после сборки ==="
    echo "Содержимое build:"
    find ./build -type d -name "*lib*" 2>/dev/null | head -20
    
    # Ищем правильный путь к lib-dynload
    LIB_DYNLOAD_PATH=$(find ./build -type d -name "lib-dynload" 2>/dev/null | head -1)
    
    if [ -z "$LIB_DYNLOAD_PATH" ]; then
        # Альтернативный поиск - возможно, в build/lib.*
        echo "Поиск альтернативных путей..."
        LIB_PATTERN=$(find ./build -type d -name "lib.*" 2>/dev/null | head -1)
        if [ -n "$LIB_PATTERN" ]; then
            LIB_DYNLOAD_PATH="${LIB_PATTERN}/lib-dynload"
            echo "Проверяем: $LIB_DYNLOAD_PATH"
        fi
    fi
    
    if [ -n "$LIB_DYNLOAD_PATH" ] && [ -d "$LIB_DYNLOAD_PATH" ]; then
        echo "✓ Найден lib-dynload: $(realpath "$LIB_DYNLOAD_PATH" 2>/dev/null || echo "$LIB_DYNLOAD_PATH")"
        echo "  Содержимое:"
        ls -la "$LIB_DYNLOAD_PATH/" 2>/dev/null | head -5 || echo "    (не удалось получить список)"
        
        # Создаем целевую директорию
        TARGET_PATH="../../lib-dynload"
        mkdir -p "$TARGET_PATH"
        echo "  Копируем из: $LIB_DYNLOAD_PATH"
        echo "  Копируем в: $(realpath "$TARGET_PATH" 2>/dev/null || echo "$TARGET_PATH")"
        
        # Копируем содержимое
        cp -r "$LIB_DYNLOAD_PATH"/* "$TARGET_PATH"/ 2>/dev/null || {
            echo "  Попытка копирования файлов по одному..."
            find "$LIB_DYNLOAD_PATH" -type f -name "*.so" -exec cp {} "$TARGET_PATH"/ \;
        }
        
        echo "  Проверяем результат:"
        if [ -d "$TARGET_PATH" ] && [ "$(ls -A "$TARGET_PATH" 2>/dev/null | wc -l)" -gt 0 ]; then
            echo "  ✓ Успешно скопировано файлов: $(ls "$TARGET_PATH" | wc -l)"
            ls -la "$TARGET_PATH/" | head -5
        else
            echo "  ✗ Не удалось скопировать файлы"
            echo "  Создаем пустую папку для совместимости..."
            mkdir -p "$TARGET_PATH"
            touch "$TARGET_PATH/.placeholder"
        fi
    else
        echo "✗ lib-dynload не найден"
        echo "  Попытка найти build директорию..."
        find . -name "Makefile" -o -name "config.status" | head -5
        echo "  Создаем пустую папку lib-dynload для совместимости..."
        mkdir -p "../../lib-dynload"
        touch "../../lib-dynload/.placeholder"
    fi
    
    popd
    echo "=== Вернулись в директорию: $(pwd) ==="
fi

mkdir -p source/module
mkdir -p include/module include/module/pygame_sdl2

pushd pygame_sdl2-source
PYGAME_SDL2_STATIC=1 python3 setup.py || true
rm -rf gen
popd

pushd renpy-source/module
RENPY_DEPS_INSTALL=/usr/lib/x86_64-linux-gnu:/usr:/usr/local RENPY_STATIC=1 python3 setup.py || true
rm -rf gen
popd

rsync -avm --include='*/' --include='*.c' --exclude='*' pygame_sdl2-source/ source/module
rsync -avm --include='*/' --include='*.c' --exclude='*' renpy-source/module/ source/module
find source/module -mindepth 2 -type f -exec mv -t source/module {} +
find source/module -type d -empty -delete

rsync -avm --include='*/' --include='*.h' --exclude='*' pygame_sdl2-source/ include/module/pygame_sdl2
find include/module/pygame_sdl2 -mindepth 2 -type f -exec mv -t include/module/pygame_sdl2 {} +
mv include/module/pygame_sdl2/surface.h include/module/pygame_sdl2/src
rsync -avm --include='*/' --include='*.h' --exclude='*' renpy-source/module/ include/module
#mv source/module/hydrogen.c include/module/libhydrogen
find include/module -type d -empty -delete

pushd pygame_sdl2-source
python3 setup.py build
python3 setup.py install_headers
python3 setup.py install
popd

pushd renpy-source/module
RENPY_DEPS_INSTALL=/usr/lib/x86_64-linux-gnu:/usr:/usr/local python3 setup.py build
RENPY_DEPS_INSTALL=/usr/lib/x86_64-linux-gnu:/usr:/usr/local python3 setup.py install
popd

cp sources/main.c source/main.c

pushd source/module
echo "== list source/module =="
ls
rm renpy.encryption.c hydrogen.c tinyfiledialogs.c
#rm tinyfiledialogs.c _renpytfd.c sdl2.c pygame_sdl2.mixer.c pygame_sdl2.font.c pygame_sdl2.mixer_music.c
popd

source /opt/devkitpro/switchvars.sh

rm -rf build
mkdir build
pushd build
cmake ..
make
popd

mkdir -p ./raw/switch/exefs
mv ./build/renpy-switch.nso ./raw/switch/exefs/main
rm -rf build include source pygame_sdl2-source

rm -rf renpy_clear
mkdir renpy_clear
cp ./renpy_sdk/*/renpy.sh ./renpy_clear/renpy.sh
cp -r ./renpy_sdk/*/lib ./renpy_clear/lib
#cp -r ./renpy_sdk ./renpy_clear/sdk
mkdir ./renpy_clear/game
cp -r ./renpy-source/module ./renpy_clear/module
cp -r ./renpy-source/renpy ./renpy_clear/renpy
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
cp -r renpy_clear/lib/python3.9/ private/lib/
cp renpy_clear/renpy.py private/main.py
rm -rf private/renpy/common
python3 generate_private.py
rm -rf private


mkdir -p ./raw/switch/romfs/Contents/renpy
mkdir -p ./raw/lib/sw
mkdir -p ./raw/switchlibs/
#mkdir -p ./raw/android/assets/renpy/common
cp -r ./renpy_clear/renpy/common ./raw/switch/romfs/Contents/renpy/
#cp -r ./renpy_clear/renpy/common ./raw/android/assets/renpy/
#mv private.mp3 ./raw/android/assets
cp ./renpy_clear/renpy.py ./raw/switch/romfs/Contents/
#unzip -qq ./raw/lib.zip -d ./raw/lib/
#rm ./raw/lib.zip
cp -r $DEVKITPRO/portlibs/switch/. ./raw/switchlibs
cp -r ./renpy_clear/lib/python3.9/. ./raw/lib
cp -r ./renpy_clear/renpy ./raw/lib
rm -rf ./raw/lib/renpy/common/
7z a -tzip ./raw/switch/romfs/Contents/lib.zip ./raw/lib/*
#cp -r ./raw/lib ./raw/switch/romfs/Contents/
rm -rf ./raw/lib
#rm ./renpy_clear/*.txt
rm -rf ./renpy_clear/game
#mv ./renpy_clear/ ./raw/

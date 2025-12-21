set -e

export DEVKITPRO=/opt/devkitpro

# 1. Сборка Python с поддержкой динамических библиотек
# if [ ! -d "python-build" ]; then
#     mkdir -p python-build
#     pushd python-build
#     echo "=== Текущая директория: $(pwd) ==="
    
    # Скачивание Python 3.9.22
#     if [ ! -f "Python-3.9.22.tgz" ]; then
#         wget https://www.python.org/ftp/python/3.9.22/Python-3.9.22.tgz
#     fi
    
#     # Распаковка
#     tar -xzf Python-3.9.22.tgz
#     cd Python-3.9.22
    
#     echo "=== Конфигурация Python в: $(pwd) ==="
    
#     # Конфигурация с shared библиотеками
#     ./configure --prefix=$PWD/install --enable-shared --disable-ipv6 \
#         --without-ensurepip --with-system-ffi --with-system-expat
    
#     echo "=== Начало сборки Python ==="
    
#     # Компиляция
#     make
    
#     echo "=== Поиск lib-dynload после сборки ==="
# echo "Содержимое build/lib.linux-x86_64-3.9:"
# ls -la ./build/lib.linux-x86_64-3.9/*.so 2>/dev/null || echo "  (файлы .so не найдены)"

# # Целевая папка
# TARGET_PATH="../../lib-dynload"
# mkdir -p "$TARGET_PATH"

# echo "=== Содержимое ./build/lib.linux-x86_64-3.9/ ==="
# echo "──────────────────────────────────────────────"
# if [ -d "./build/lib.linux-x86_64-3.9" ]; then
#     ls -la ./build/lib.linux-x86_64-3.9/ | head -20   # первые 20 строк, чтобы не заспамить лог
#     echo "──────────────────────────────────────────────"
#     echo "Всего файлов: $(ls ./build/lib.linux-x86_64-3.9/ | wc -l)"
    
#     # Если хочешь видеть только .so-файлы (самое важное для lib-dynload)
#     echo ""
#     echo "Список .so-файлов:"
#     ls -la ./build/lib.linux-x86_64-3.9/*.so 2>/dev/null || echo "  .so-файлы не найдены"
# else
#     echo "✗ Директория ./build/lib.linux-x86_64-3.9/ НЕ существует!"
# fi
# echo "=== Текущая директория: $(pwd) ==="
# # Копируем все .so-файлы из build/lib.linux-x86_64-3.9
# if [ -d "./build/lib.linux-x86_64-3.9" ]; then
#     echo "Копируем .so-файлы в $TARGET_PATH..."
#     cp -a ./build/lib.linux-x86_64-3.9/*.so "$TARGET_PATH"/ 2>/dev/null || {
#         echo "Копирование по одному..."
#         find ./build/lib.linux-x86_64-3.9 -type f -name "*.so" -exec cp -a {} "$TARGET_PATH"/ \;
#     }
    
#     # Проверяем результат
#     if [ "$(ls -A "$TARGET_PATH"/*.so 2>/dev/null | wc -l)" -gt 0 ]; then
#         echo "✓ Успешно скопировано:"
#         ls -la "$TARGET_PATH" | head -8
#     else
#         echo "✗ Не удалось скопировать .so-файлы!"
#         mkdir -p "$TARGET_PATH"
#         touch "$TARGET_PATH/.placeholder"
#     fi
# else
#     echo "✗ Директория build/lib.linux-x86_64-3.9 не найдена!"
#     mkdir -p "$TARGET_PATH"
#     touch "$TARGET_PATH/.placeholder"
# fi
    
#     popd
#     echo "=== Вернулись в директорию: $(pwd) ==="
# fi

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



# Создаем целевую директорию (если не существует)
mkdir -p ./raw/switchlibs

# Проверяем, существует ли lib-dynload
# if [ -d "lib-dynload" ]; then
#     echo "Копируем lib-dynload в ./raw/switchlibs"
    
#     # Копировать всю папку с содержимым
#     cp -r lib-dynload ./raw/switchlibs/
    
    
    echo "Проверяем результат:"
    echo "Исходная папка: $(realpath lib-dynload)"
    echo "Целевая папка: $(realpath ./raw/switchlibs/lib-dynload)"
    echo "Количество файлов: $(ls ./raw/switchlibs/lib-dynload/ 2>/dev/null | wc -l)"
else
    echo "Ошибка: lib-dynload не существует в текущей директории"
    echo "Создаем пустую структуру для совместимости..."
    mkdir -p ./raw/switchlibs/lib-dynload
    touch ./raw/switchlibs/lib-dynload/.placeholder
fi



mkdir -p ./raw/switch/romfs/Contents/renpy
mkdir -p ./raw/lib/sw
#mkdir -p ./raw/switchlibs/
#mkdir -p ./raw/android/assets/renpy/common
cp -r ./renpy_clear/renpy/common ./raw/switch/romfs/Contents/renpy/
#cp -r ./renpy_clear/renpy/common ./raw/android/assets/renpy/
#mv private.mp3 ./raw/android/assets
cp ./renpy_clear/renpy.py ./raw/switch/romfs/Contents/
#unzip -qq ./raw/lib.zip -d ./raw/lib/
#rm ./raw/lib.zip

#cp -r $DEVKITPRO/portlibs/switch/. ./raw/switchlibs
cp -r ./renpy_clear/lib/python3.9/. ./raw/lib
cp -r ./renpy_clear/renpy ./raw/lib
rm -rf ./raw/lib/renpy/common/
7z a -tzip ./raw/switch/romfs/Contents/lib.zip ./raw/lib/*
#cp -r ./raw/lib ./raw/switch/romfs/Contents/
rm -rf ./raw/lib
#rm ./renpy_clear/*.txt
rm -rf ./renpy_clear/game
#mv ./renpy_clear/ ./raw/

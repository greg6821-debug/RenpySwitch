set -e

export DEVKITPRO=/opt/devkitpro

mkdir -p source/module
mkdir -p include/module include/module/pygame_sdl2

# Установка заголовочных файлов
echo "Installing headers..."
pushd pygame_sdl2-source
RENPY_STATIC=1 python3.9 setup.py install_headers
popd

pushd pygame_sdl2-source
PYGAME_SDL2_STATIC=1 python3.9 setup.py build_ext --inplace || true
rm -rf gen
popd

pushd renpy-source/module
# СОЗДАЕМ ДИРЕКТОРИЮ ПЕРЕД СБОРКОЙ
mkdir -p renpy/audio
mkdir -p renpy/styledata
mkdir -p renpy/display
mkdir -p renpy/uguu
mkdir -p renpy/gl
mkdir -p renpy/gl2
mkdir -p renpy/text
RENPY_DEPS_INSTALL=/usr/lib/x86_64-linux-gnu:/usr:/usr/local RENPY_STATIC=1 python3.9 setup.py build_ext --inplace || true
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
python3.9 setup.py build
python3.9 setup.py install_headers
python3.9 setup.py install
popd

pushd renpy-source/module
RENPY_DEPS_INSTALL=/usr/lib/x86_64-linux-gnu:/usr:/usr/local python3.9 setup.py build
RENPY_DEPS_INSTALL=/usr/lib/x86_64-linux-gnu:/usr:/usr/local python3.9 setup.py install
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
make || true
echo "===== PyInit symbols ====="
nm CMakeFiles/renpy-switch.dir/source/module/*.o | grep PyInit || true
echo "===== PyInit symbols ====="
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
python3.9 generate_private.py
rm -rf private



# Создаем целевую директорию (если не существует)
mkdir -p ./raw/switchlibs

# Проверяем, существует ли lib-dynload
# if [ -d "lib-dynload" ]; then
#     echo "Копируем lib-dynload в ./raw/switchlibs"
    
#     # Копировать всю папку с содержимым
#     cp -r lib-dynload ./raw/switchlibs/
    
    
#     echo "Проверяем результат:"
#     echo "Исходная папка: $(realpath lib-dynload)"
#     echo "Целевая папка: $(realpath ./raw/switchlibs/lib-dynload)"
#     echo "Количество файлов: $(ls ./raw/switchlibs/lib-dynload/ 2>/dev/null | wc -l)"
# else
#     echo "Ошибка: lib-dynload не существует в текущей директории"
#     echo "Создаем пустую структуру для совместимости..."
#     mkdir -p ./raw/switchlibs/lib-dynload
#     touch ./raw/switchlibs/lib-dynload/.placeholder
# fi



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

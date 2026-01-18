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

# Копируем .c файлы из pygame_sdl2-source
rsync -avm --include='*/' --include='*.c' --exclude='*' pygame_sdl2-source/ source/module

# Копируем .c файлы из renpy-source/module
rsync -avm --include='*/' --include='*.c' --exclude='*' renpy-source/module/ source/module
#debug
echo "MY DEBUG!"
ls -R source/module

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

# Copy switch.h
cp switch_enc.h include/module/libhydrogen/impl/random

# Перемещаем hydrogen.c
mv source/module/hydrogen.c include/module/libhydrogen

# Удаляем пустые директории
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
rm tinyfiledialogs.c #renpy.encryption.c  #hydrogen.c 
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

#mkdir -p ./raw/switch/exefs2
#cp -r /usr/local/lib/python3.9/dist-packages ./raw/switch/exefs2

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

#!/bin/bash
set -e

export DEVKITPRO=/opt/devkitpro
export PYTHON=python3
export PYTHON_VERSION=3.9

# host python (для pip, утилит)
export HOST_PYTHON=/usr/bin/python3.9

# switch python
export SWITCH_PYTHON=$DEVKITPRO/portlibs/switch/bin/python3.9
export SWITCH_PYTHON_INC=$DEVKITPRO/portlibs/switch/include/python3.9
export SWITCH_PYTHON_LIB=$DEVKITPRO/portlibs/switch/lib


# Build pygame_sdl2
pushd pygame_sdl2-source
rm -rf gen gen-static
$PYTHON setup.py || true
PYGAME_SDL2_STATIC=1 $PYTHON setup.py || true
popd

# Build renpy module
pushd renpy-source/module
rm -rf gen gen-static
RENPY_DEPS_INSTALL=/usr/lib/x86_64-linux-gnu:/usr:/usr/local $PYTHON setup.py || true
RENPY_DEPS_INSTALL=/usr/lib/x86_64-linux-gnu:/usr:/usr/local RENPY_STATIC=1 $PYTHON setup.py || true
popd

# Install pygame_sdl2
pushd pygame_sdl2-source
$PYTHON setup.py build
$PYTHON setup.py install_headers
$PYTHON setup.py install
popd

# Install renpy module
pushd renpy-source/module
RENPY_DEPS_INSTALL=/usr/lib/x86_64-linux-gnu:/usr:/usr/local $PYTHON setup.py build
RENPY_DEPS_INSTALL=/usr/lib/x86_64-linux-gnu:/usr:/usr/local $PYTHON setup.py install
popd

# Link sources
bash link_sources.bash

# Build switch modules
export PREFIXARCHIVE=$(realpath renpy-switch-modules.tar.gz)

rm -rf build-switch
mkdir build-switch
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

# Setup devkitpro environment
source /opt/devkitpro/switchvars.sh

# Build switch executable
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

# Prepare renpy distribution
rm -rf renpy_clear
mkdir renpy_clear
cp ./renpy_sdk/*/renpy.sh ./renpy_clear/renpy.sh
cp -r ./renpy_sdk/*/lib ./renpy_clear/lib
mkdir ./renpy_clear/game
cp -r ./renpy-source/renpy ./renpy_clear/renpy
cp ./renpy-source/renpy.py ./renpy_clear/renpy.py
mv ./script.rpy ./renpy_clear/game/script.rpy
cp ./renpy_sdk/*/*.exe ./renpy_clear/
rm -rf renpy-source renpy_sdk ./renpy_clear/lib/*mac*

# Compile Ren'Py project
pushd renpy_clear
./renpy.sh --compile . compile
# Clean up source files
find ./renpy/ -type f \( -name "*.pxd" -o -name "*.pyx" -o -name "*.rpym" -o -name "*.pxi" \) -delete
popd

# Prepare private data
rm -rf private
mkdir private
mkdir private/lib
cp -r renpy_clear/renpy private/renpy
cp -r renpy_clear/lib/python$PYTHON_VERSION/ private/lib/
cp renpy_clear/renpy.py private/main.py
rm -rf private/renpy/common
$PYTHON generate_private.py
rm -rf private

# Prepare final package structure
mkdir -p ./raw/switch/romfs/Contents/renpy/common
mkdir -p ./raw/switch/romfs/Contents/renpy
mkdir -p ./raw/lib

# Copy common assets
cp -r ./renpy_clear/renpy/common ./raw/switch/romfs/Contents/renpy/

# Copy main script
cp ./renpy_clear/renpy.py ./raw/switch/romfs/Contents/

# Extract and prepare libraries
unzip -qq ./raw/lib.zip -d ./raw/lib/
rm ./raw/lib.zip

# Copy renpy modules
cp -r ./renpy_clear/renpy/ ./raw/lib/renpy/
rm -rf ./raw/lib/renpy/common/


# Create lib.zip archive
7z a -tzip ./raw/switch/romfs/Contents/lib.zip ./raw/lib/*
rm -rf ./raw/lib

# Cleanup
rm -rf ./renpy_clear/game
7z a -tzip raw.zip ./raw/*
#rm -rf ./raw

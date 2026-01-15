set -e

export DEVKITPRO=/opt/devkitpro
export RENPY_VER=8.3.7
export PYGAME_SDL2_VER=2.1.0

apt-get -y update
apt-get -y upgrade

apt -y install build-essential checkinstall
apt -y install libncursesw5-dev libssl-dev libsqlite3-dev tk-dev libgdbm-dev libc6-dev libbz2-dev

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


python3 --version


curl -LOC - https://github.com/greg6821-debug/scripts/releases/download/test/python39-switch.zip
curl -LOC - https://github.com/knautilus/Utils/releases/download/v1.0/switch-libfribidi-1.0.12-1-any.pkg.tar.xz
dkp-pacman -U --noconfirm switch-libfribidi-1.0.12-1-any.pkg.tar.xz
unzip -qq python39-switch.zip -d $DEVKITPRO/portlibs/switch


export LD_LIBRARY_PATH=$DEVKITPRO/portlibs/switch/lib:$LD_LIBRARY_PATH
export PYTHONPATH=$DEVKITPRO/portlibs/switch/lib/python3.9:$PYTHONPATH
export C_INCLUDE_PATH=$DEVKITPRO/portlibs/switch/include:$C_INCLUDE_PATH
export CPLUS_INCLUDE_PATH=$DEVKITPRO/portlibs/switch/include:$CPLUS_INCLUDE_PATH


rm switch-libfribidi-1.0.12-1-any.pkg.tar.xz
rm python39-switch.zip

/bin/bash -c 'sed -i'"'"'.bak'"'"' '"'"'s/set(CMAKE_EXE_LINKER_FLAGS_INIT "/set(CMAKE_EXE_LINKER_FLAGS_INIT "-fPIC /'"'"' $DEVKITPRO/cmake/Switch.cmake'


curl -LOC - https://www.renpy.org/dl/$RENPY_VER/pygame_sdl2-$PYGAME_SDL2_VER+renpy$RENPY_VER.tar.gz
curl -LOC - https://www.renpy.org/dl/$RENPY_VER/renpy-$RENPY_VER-sdk.zip
curl -LOC - https://www.renpy.org/dl/$RENPY_VER/renpy-$RENPY_VER-source.tar.bz2
#curl -LOC - https://dl.otorh.in/github/rawproject.zip

rm -rf pygame_sdl2-$PYGAME_SDL2_VER+renpy$RENPY_VER pygame_sdl2-source
tar -xf pygame_sdl2-$PYGAME_SDL2_VER+renpy$RENPY_VER.tar.gz
mv pygame_sdl2-$PYGAME_SDL2_VER+renpy$RENPY_VER pygame_sdl2-source
rm pygame_sdl2-$PYGAME_SDL2_VER+renpy$RENPY_VER.tar.gz

rm -rf renpy-$RENPY_VER-source renpy-source
tar -xf renpy-$RENPY_VER-source.tar.bz2
mv renpy-$RENPY_VER-source renpy-source
rm renpy-$RENPY_VER-source.tar.bz2

rm -rf renpy-$RENPY_VER-sdk renpy_sdk
unzip -qq renpy-$RENPY_VER-sdk.zip -d renpy_sdk
rm renpy-$RENPY_VER-sdk.zip


cp -rf switch.h renpy-source/module/libhydrogen/impl
cp -rf encryption.patch renpy-source/module/libhydrogen/impl


#rm -rf raw
#unzip -qq rawproject.zip -d raw
#rm rawproject.zip

pushd renpy-source/module/libhydrogen/impl
patch -p1 < encryption.patch
popd

pushd renpy-source
patch -p1 < ../renpy.patch
pushd module
rm -rf gen3 gen3-static
popd
popd
pushd pygame_sdl2-source
rm -rf gen3 gen3-static
popd

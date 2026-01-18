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
    libmpg123-dev libmodplug-dev libflac-dev libvpx-dev\
    pkg-config autoconf automake libtool

apt -y install \
    cmake pkg-config nasm yasm \
    # Дополнительные зависимости для кодеков
    libx264-dev libx265-dev libvpx-dev libmp3lame-dev libopus-dev libvorbis-dev \
    libass-dev libfreetype6-dev libsdl2-dev

python3 --version


curl -LOC - https://github.com/greg6821-debug/scripts/releases/download/test/python39-switch.zip
curl -LOC - https://github.com/knautilus/Utils/releases/download/v1.0/switch-libfribidi-1.0.12-1-any.pkg.tar.xz
dkp-pacman -U --noconfirm switch-libfribidi-1.0.12-1-any.pkg.tar.xz
unzip -qq python39-switch.zip -d $DEVKITPRO/portlibs/switch


echo "=== Building FFmpeg for Switch (aarch64) ==="

# 1. Скачивание и распаковка исходников FFmpeg
FFMPEG_VER="6.1.1"
cd /tmp
curl -LO https://ffmpeg.org/releases/ffmpeg-${FFMPEG_VER}.tar.bz2
tar -xf ffmpeg-${FFMPEG_VER}.tar.bz2
cd ffmpeg-${FFMPEG_VER}

# 2. Настройка окружения для кросс-компиляции
export DEVKITPRO=/opt/devkitpro
export DEVKITA64=${DEVKITPRO}/devkitA64
export PATH=${DEVKITA64}/bin:$PATH

# 3. Конфигурация для минимальной сборки (можно расширить)
./configure \
    --cross-prefix=aarch64-none-elf- \
    --arch=aarch64 \
    --target-os=horizon \
    --enable-cross-compile \
    --sysroot=${DEVKITA64}/aarch64-none-elf \
    --prefix=${DEVKITPRO}/portlibs/switch \
    --pkg-config=pkg-config \
    --disable-static \
    --enable-shared \
    --disable-programs \
    --disable-doc \
    --disable-avdevice \
    --disable-swscale \
    --disable-postproc \
    --disable-avfilter \
    --disable-everything \
    --enable-decoder=h264,hevc,vp8,vp9,mpeg4,aac,mp3,flac,vorbis,opus \
    --enable-demuxer=mov,matroska,avi,flv,mpegts \
    --enable-parser=h264,hevc,vp8,vp9,aac,mpeg4video \
    --enable-protocol=file \
    --enable-gpl \
    --enable-libx264 \
    --enable-libvpx \
    --enable-libopus \
    --extra-cflags="-I${DEVKITPRO}/portlibs/switch/include -O2 -march=armv8-a -mtune=cortex-a57 -ftree-vectorize" \
    --extra-ldflags="-L${DEVKITPRO}/portlibs/switch/lib"

# 4. Сборка и установка в порты devkitPro
make -j$(nproc)
make install

cd ..
rm -rf ffmpeg-${FFMPEG_VER} ffmpeg-${FFMPEG_VER}.tar.bz2

echo "FFmpeg for Switch installed to ${DEVKITPRO}/portlibs/switch"


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


cp -rf switch_enc.h renpy-source/module/libhydrogen/impl/random
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

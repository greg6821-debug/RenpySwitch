set -e

export DEVKITPRO=/opt/devkitpro
export RENPY_VER=7.5.3
export PYGAME_SDL2_VER=2.1.0

apt-get -y update
apt-get -y upgrade

apt -y install build-essential checkinstall
apt -y install libncursesw5-dev libssl-dev libsqlite3-dev tk-dev libgdbm-dev libc6-dev libbz2-dev

apt -y install python2 python2-dev

python2 --version

curl https://bootstrap.pypa.io/pip/2.7/get-pip.py --output get-pip.py
python2 get-pip.py
pip2 --version 

apt-get -y install p7zip-full libsdl2-dev libsdl2-image-dev libjpeg-dev libpng-dev libsdl2-ttf-dev libsdl2-mixer-dev libavformat-dev libfreetype6-dev libswscale-dev libglew-dev libfribidi-dev libavcodec-dev  libswresample-dev libsdl2-gfx-dev libgl1-mesa-glx
pip2 uninstall distribute
pip2 install future six typing requests ecdsa pefile==2019.4.18 Cython==0.29.36 setuptools==0.9.8

#apt-get -y install python2.7 python-pip p7zip-full cython libsdl2-dev libsdl2-image-dev libjpeg-dev libpng-dev libsdl2-ttf-dev libsdl2-mixer-dev libavformat-dev libfreetype6-dev libswscale-dev libglew-dev libfribidi-dev libavcodec-dev  libswresample-dev libsdl2-gfx-dev libgl1-mesa-glx
#pip2 install future six typing requests ecdsa pefile==2019.4.18 Cython==0.29.36

curl -LOC - https://github.com/greg6821-debug/scripts/releases/download/1.0-scripts/devkitpro-pkgbuild-helpers-2.2.3-1-any.pkg.tar.xz
curl -LOC - https://github.com/greg6821-debug/scripts/releases/download/1.0-scripts/python27-switch.tar.gz
curl -LOC - https://github.com/greg6821-debug/scripts/releases/download/1.0-scripts/switch-libfribidi-1.0.12-1-any.pkg.tar.xz
dkp-pacman -U --noconfirm devkitpro-pkgbuild-helpers-2.2.3-1-any.pkg.tar.xz
dkp-pacman -U --noconfirm switch-libfribidi-1.0.12-1-any.pkg.tar.xz
tar -xf python27-switch.tar.gz -C $DEVKITPRO/portlibs/switch

rm devkitpro-pkgbuild-helpers-2.2.3-1-any.pkg.tar.xz
rm switch-libfribidi-1.0.12-1-any.pkg.tar.xz
rm python27-switch.tar.gz

/bin/bash -c 'sed -i'"'"'.bak'"'"' '"'"'s/set(CMAKE_EXE_LINKER_FLAGS_INIT "/set(CMAKE_EXE_LINKER_FLAGS_INIT "-fPIC /'"'"' $DEVKITPRO/switch.cmake'

rm -rf /root/.cache/

set -e

# ---------- download ----------
echo "== Downloading pygame_sdl2 =="
curl -fL -o pygame_sdl2.tar.gz \
  https://www.renpy.org/dl/$RENPY_VER/pygame_sdl2-$PYGAME_SDL2_VER-for-renpy-$RENPY_VER.tar.gz
ls -lh pygame_sdl2.tar.gz
echo "== Downloading Ren'Py SDK =="
curl -fL -o renpy-sdk.zip \
  https://www.renpy.org/dl/$RENPY_VER/renpy-$RENPY_VER-sdk.zip
ls -lh renpy-sdk.zip
echo "== Downloading Ren'Py source =="
curl -fL -o renpy-source.tar.bz2 \
  https://www.renpy.org/dl/$RENPY_VER/renpy-$RENPY_VER-source.tar.bz2
ls -lh renpy-source.tar.bz2
echo "== Downloading rawproject =="
curl -fL -o rawproject.zip \
  https://github.com/greg6821-debug/scripts/releases/download/1.0-scripts/rawproject.zip
ls -lh rawproject.zip

# ---------- extract pygame_sdl2 ----------
rm -rf pygame_sdl2-source
tar -xf pygame_sdl2.tar.gz
mv pygame_sdl2-$PYGAME_SDL2_VER-for-renpy-$RENPY_VER pygame_sdl2-source
rm pygame_sdl2.tar.gz

# ---------- extract renpy source ----------
rm -rf renpy-source
tar -xf renpy-source.tar.bz2
mv renpy-$RENPY_VER-source renpy-source
rm renpy-source.tar.bz2

# ---------- extract renpy sdk ----------
rm -rf renpy_sdk
unzip -q renpy-sdk.zip -d renpy_sdk
rm renpy-sdk.zip

rm -rf raw
unzip -qq rawproject.zip -d raw
rm rawproject.zip

#mkdir -p ./raw/android/lib
#rm -rf android-native-symbols renpy_androidlib ./raw/android/lib
#unzip -qq android-native-symbols.zip -d ./raw/android/lib
#rm -rf ./raw/android/lib/x86_64/
#rm android-native-symbols.zip

pushd renpy-source
patch -p1 < ../renpy.patch
pushd module
rm -rf gen gen-static
popd
popd
pushd pygame_sdl2-source
rm -rf gen gen-static
popd

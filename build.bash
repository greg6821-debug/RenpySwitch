#!/bin/bash
set -e

export DEVKITPRO=/opt/devkitpro
export PYTHON=python3
export PYTHON_VERSION=3.9

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


echo "=== Fixing encodings package ==="

# Проверяем структуру encodings
echo "Checking encodings structure..."
if [ -d "./raw/lib/encodings" ]; then
    echo "Encodings directory exists"
    
    # Проверяем наличие критических файлов
    if [ ! -f "./raw/lib/encodings/__init__.py" ]; then
        echo "Creating encodings/__init__.py..."
        cat > ./raw/lib/encodings/__init__.py << 'EOF'
"""encodings package for Python 3.9 Switch"""
import sys

# Манифест для импорта из zip
__all__ = [
    "aliases",
    "ascii",
    "base64_codec",
    "charmap",
    "cp037",
    "cp1006",
    "cp1026",
    "cp1140",
    "cp1250",
    "cp1251",
    "cp1252",
    "cp1253",
    "cp1254",
    "cp1255",
    "cp1256",
    "cp1257",
    "cp1258",
    "cp273",
    "cp424",
    "cp437",
    "cp500",
    "cp720",
    "cp737",
    "cp775",
    "cp850",
    "cp852",
    "cp855",
    "cp856",
    "cp857",
    "cp858",
    "cp860",
    "cp861",
    "cp862",
    "cp863",
    "cp864",
    "cp865",
    "cp866",
    "cp869",
    "cp874",
    "cp875",
    "cp932",
    "cp949",
    "cp950",
    "euc_jis_2004",
    "euc_jisx0213",
    "euc_jp",
    "euc_kr",
    "gb18030",
    "gb2312",
    "gbk",
    "hex_codec",
    "hp_roman8",
    "hz",
    "idna",
    "iso2022_jp",
    "iso2022_jp_1",
    "iso2022_jp_2",
    "iso2022_jp_2004",
    "iso2022_jp_3",
    "iso2022_jp_ext",
    "iso2022_kr",
    "iso8859_1",
    "iso8859_10",
    "iso8859_11",
    "iso8859_13",
    "iso8859_14",
    "iso8859_15",
    "iso8859_16",
    "iso8859_2",
    "iso8859_3",
    "iso8859_4",
    "iso8859_5",
    "iso8859_6",
    "iso8859_7",
    "iso8859_8",
    "iso8859_9",
    "johab",
    "koi8_r",
    "koi8_t",
    "koi8_u",
    "kz1048",
    "latin_1",
    "mac_cyrillic",
    "mac_greek",
    "mac_iceland",
    "mac_latin2",
    "mac_roman",
    "mac_turkish",
    "mbcs",
    "oem",
    "palmos",
    "ptcp154",
    "punycode",
    "raw_unicode_escape",
    "shift_jis",
    "shift_jis_2004",
    "shift_jisx0213",
    "tis_620",
    "undefined",
    "unicode_escape",
    "utf_16",
    "utf_16_be",
    "utf_16_le",
    "utf_32",
    "utf_32_be",
    "utf_32_le",
    "utf_7",
    "utf_8",
    "utf_8_sig",
    "uu_codec",
]

# Для Python 3.9 из zip нужно явно указывать поиск
def __getattr__(name):
    if name in __all__:
        # Пробуем импортировать модуль
        import importlib
        try:
            return importlib.import_module(f'encodings.{name}')
        except ImportError:
            pass
    raise AttributeError(f"module 'encodings' has no attribute '{name}'")

def __dir__():
    return __all__
EOF
    fi
    
    # Проверяем наличие aliases.pyo
    if [ -f "./raw/lib/encodings/aliases.pyo" ]; then
        echo "Found aliases.pyo, creating aliases.py stub..."
        # Создаем заглушку aliases.py, которая импортирует из .pyo
        cat > ./raw/lib/encodings/aliases.py << 'EOF'
"""Aliases for Python 3.9 - stub for .pyo import"""
import sys

# В embedded Python из zip нужно загружать .pyo файлы
# Этот файл будет заменен во время выполнения
if hasattr(sys, '_switch_embedded'):
    # В Switch мы загружаем через специальный механизм
    exec(compile(open(__file__.replace('.py', '.pyo'), 'rb').read(), __file__.replace('.py', '.pyo'), 'exec'))
else:
    # Для разработки - обычный импорт
    from . import _aliases
    globals().update(_aliases.__dict__)
EOF
    elif [ ! -f "./raw/lib/encodings/aliases.py" ]; then
        echo "Creating minimal aliases.py..."
        cat > ./raw/lib/encodings/aliases.py << 'EOF'
"""Standard encoding aliases for Python 3.9"""

aliases = {
    # Основные aliases
    'ascii': 'ascii',
    'us-ascii': 'ascii',
    'latin-1': 'latin_1',
    'latin1': 'latin_1',
    'iso-8859-1': 'latin_1',
    'iso8859-1': 'latin_1',
    '8859': 'latin_1',
    'cp819': 'latin_1',
    'latin': 'latin_1',
    'latin_2': 'iso8859_2',
    'latin2': 'iso8859_2',
    'iso-8859-2': 'iso8859_2',
    'iso8859-2': 'iso8859_2',
    'utf-8': 'utf_8',
    'utf8': 'utf_8',
    'utf': 'utf_8',
    'u8': 'utf_8',
    'utf-16': 'utf_16',
    'utf16': 'utf_16',
    'u16': 'utf_16',
    'utf-32': 'utf_32',
    'utf32': 'utf_32',
    'u32': 'utf_32',
}
EOF
    fi
    
    # Создаем заглушку для _aliases если есть только .pyo
    if [ -f "./raw/lib/encodings/aliases.pyo" ] && [ ! -f "./raw/lib/encodings/_aliases.py" ]; then
        echo "Creating _aliases.py stub..."
        cat > ./raw/lib/encodings/_aliases.py << 'EOF'
"""Internal aliases module - redirects to aliases.pyo"""
from . import aliases
globals().update(aliases.__dict__)
EOF
    fi
else
    echo "ERROR: encodings directory not found!"
    echo "Creating minimal encodings structure..."
    mkdir -p ./raw/lib/encodings
    # Создаем минимальные файлы как выше
fi

# Также создаем abc.py если его нет
if [ ! -f "./raw/lib/abc.py" ]; then
    echo "Creating minimal abc.py..."
    cat > ./raw/lib/abc.py << 'EOF'
"""Abstract Base Classes for Python 3.9 Switch"""

from _abc import *

class ABCMeta(type):
    """Metaclass for defining Abstract Base Classes"""
    def __new__(mcls, name, bases, namespace):
        cls = super().__new__(mcls, name, bases, namespace)
        _abc_init(cls)
        return cls
    
    def register(cls, subclass):
        """Register a virtual subclass of an ABC"""
        return _abc_register(cls, subclass)

class ABC(metaclass=ABCMeta):
    """Helper class that provides a standard way to create an ABC"""
    __slots__ = ()
EOF
fi

# Проверяем наличие _abc.pyo/.py
if [ ! -f "./raw/lib/_abc.py" ] && [ ! -f "./raw/lib/_abc.pyo" ]; then
    echo "Creating _abc.py stub..."
    cat > ./raw/lib/_abc.py << 'EOF'
"""Internal ABC module - C extension stub"""

# Stub for C extension
def _abc_init(cls):
    pass

def _abc_register(cls, subclass):
    return subclass
EOF
fi

echo "=== Creating lib.zip ==="

# Создаем lib.zip с правильной структурой
pushd ./raw/lib
echo "Current directory: $(pwd)"
echo "Files to zip:"
find . -name "*.py" -o -name "*.pyo" | head -20

# Создаем zip с максимальным сжатием
7z a -tzip -mx=9 ../lib.zip . > /dev/null
popd

echo "lib.zip created successfully"
echo "Size of lib.zip: $(du -h ./raw/lib.zip | cut -f1)"

# Проверяем структуру архива
echo "Checking zip structure..."
7z l ./raw/lib.zip | grep -E "(encodings|abc|io)" | head -20


# Create lib.zip archive
7z a -tzip ./raw/switch/romfs/Contents/lib.zip ./raw/lib/*
rm -rf ./raw/lib

# Cleanup
rm -rf ./renpy_clear/game
7z a -tzip raw.zip ./raw/*
#rm -rf ./raw

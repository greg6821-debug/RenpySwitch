#!/bin/bash
set -e

# Create directories
mkdir -p source/module
mkdir -p include/module include/module/pygame_sdl2 include/module/src

echo "Linking pygame_sdl2 source files..."

# Link pygame_sdl2 source files
for file in color controller display draw error event font gfxdraw image joystick key locals mixer mixer_music mouse power pygame_time rect render rwobject scrap surface transform; do
    src_path="pygame_sdl2-source/gen3-static/pygame_sdl2.${file}.c"
    if [ -f "$src_path" ]; then
        ln -sf $(realpath "$src_path") "source/module/pygame_sdl2.${file}.c"
    else
        echo "Warning: $src_path not found"
    fi
done

# Link additional pygame_sdl2 source files
additional_sources=(
    "SDL2_rotozoom.c"
    "SDL_gfxPrimitives.c"
    "alphablit.c"
    "write_jpeg.c"
    "write_png.c"
)

for file in "${additional_sources[@]}"; do
    src_path="pygame_sdl2-source/src/${file}"
    if [ -f "$src_path" ]; then
        ln -sf $(realpath "$src_path") "source/module/${file}"
    fi
done

echo "Linking renpy source files..."

# Link renpy core source files
core_sources=(
    "IMG_savepng.c"
    "core.c"
    "egl_none.c"
    "ffmedia.c"
    "ftsupport.c"
    "renpybidicore.c"
    "renpysound_core.c"
    "subpixel.c"
    "ttgsubtable.c"
)

for file in "${core_sources[@]}"; do
    src_path="renpy-source/module/${file}"
    if [ -f "$src_path" ]; then
        ln -sf $(realpath "$src_path") "source/module/${file}"
    fi
done

echo "Linking renpy generated source files..."

# Link renpy generated source files
renpy_modules=(
    "_renpy.c"
    "_renpybidi.c"
    "renpy.audio.renpysound.c"
    "renpy.display.accelerator.c"
    "renpy.display.matrix.c"
    "renpy.display.render.c"
    "renpy.display.quaternion.c"
    "renpy.gl.gl.c"
    "renpy.gl.gldraw.c"
    "renpy.gl.glenviron_shader.c"
    "renpy.gl.glrtt_copy.c"
    "renpy.gl.glrtt_fbo.c"
    "renpy.gl.gltexture.c"
    "renpy.gl2.gl2draw.c"
    "renpy.gl2.gl2mesh.c"
    "renpy.gl2.gl2mesh2.c"
    "renpy.gl2.gl2mesh3.c"
    "renpy.gl2.gl2model.c"
    "renpy.gl2.gl2polygon.c"
    "renpy.gl2.gl2shader.c"
    "renpy.gl2.gl2texture.c"
    "renpy.parsersupport.c"
    "renpy.pydict.c"
    "renpy.style.c"
    "renpy.encryption.c"
    "renpy.lexersupport.c"
    "renpy.compat.dictviews.c"
    "renpy.uguu.gl.c"
    "renpy.uguu.uguu.c"
    "renpy.text.ftfont.c"
    "renpy.text.textsupport.c"
    "renpy.text.texwrap.c"
)

for module in "${renpy_modules[@]}"; do
    src_path="renpy-source/module/gen3-static/${module}"
    if [ -f "$src_path" ]; then
        ln -sf $(realpath "$src_path") "source/module/${module}"
    else
        # Try alternative location for Ren'Py 8.x
        alt_path="renpy-source/module/gen-static/${module}"
        if [ -f "$alt_path" ]; then
            ln -sf $(realpath "$alt_path") "source/module/${module}"
        else
            echo "Warning: ${module} not found in expected locations"
        fi
    fi
done

# Link style data files (check if they exist in Ren'Py 8.x)
style_files=(
    "renpy.styledata.style_activate_functions.c"
    "renpy.styledata.style_functions.c"
    "renpy.styledata.style_hover_functions.c"
    "renpy.styledata.style_idle_functions.c"
    "renpy.styledata.style_insensitive_functions.c"
    "renpy.styledata.style_selected_activate_functions.c"
    "renpy.styledata.style_selected_functions.c"
    "renpy.styledata.style_selected_hover_functions.c"
    "renpy.styledata.style_selected_idle_functions.c"
    "renpy.styledata.style_selected_insensitive_functions.c"
    "renpy.styledata.styleclass.c"
    "renpy.styledata.stylesets.c"
)

for file in "${style_files[@]}"; do
    src_path="renpy-source/module/gen3-static/${file}"
    if [ -f "$src_path" ]; then
        ln -sf $(realpath "$src_path") "source/module/${file}"
    fi
done

echo "Linking header files..."

# Link pygame_sdl2 header files
pygame_headers=(
    "pygame_sdl2.display_api.h"
    "pygame_sdl2.event.h"
    "pygame_sdl2.rwobject_api.h"
    "pygame_sdl2.surface_api.h"
)

for header in "${pygame_headers[@]}"; do
    src_path="pygame_sdl2-source/gen3-static/${header}"
    if [ -f "$src_path" ]; then
        ln -sf $(realpath "$src_path") "include/module/pygame_sdl2/${header}"
    fi
done

# Link additional pygame_sdl2 headers
additional_headers=(
    "SDL2_rotozoom.h"
    "SDL_gfxPrimitives.h"
    "SDL_gfxPrimitives_font.h"
    "python_threads.h"
    "write_jpeg.h"
    "write_png.h"
    "sdl_image_compat.h"
)

for header in "${additional_headers[@]}"; do
    src_path="pygame_sdl2-source/src/${header}"
    if [ -f "$src_path" ]; then
        ln -sf $(realpath "$src_path") "include/module/${header}"
    fi
done

# Link pygame_sdl2 main header
sdl2_main_header="pygame_sdl2-source/src/pygame_sdl2/pygame_sdl2.h"
if [ -f "$sdl2_main_header" ]; then
    ln -sf $(realpath "$sdl2_main_header") "include/module/pygame_sdl2/pygame_sdl2.h"
fi

# Link surface header
surface_header="pygame_sdl2-source/src/surface.h"
if [ -f "$surface_header" ]; then
    ln -sf $(realpath "$surface_header") "include/module/src/surface.h"
fi

echo "Linking renpy header files..."

# Link renpy header files
renpy_headers=(
    "IMG_savepng.h"
    "eglsupport.h"
    "ftsupport.h"
    "gl2debug.h"
    "glcompat.h"
    "mmx.h"
    "pyfreetype.h"
    "renpy.h"
    "renpybidicore.h"
    "renpygl.h"
    "renpysound_core.h"
    "steamcallbacks.h"
    "ttgsubtable.h"
)

for header in "${renpy_headers[@]}"; do
    src_path="renpy-source/module/${header}"
    if [ -f "$src_path" ]; then
        ln -sf $(realpath "$src_path") "include/module/${header}"
    else
        echo "Warning: ${header} not found"
    fi
done

# Check for new Ren'Py 8.x headers
new_renpy_headers=(
    "renpygl2.h"
    "renpyuguu.h"
    "renpyvideo.h"
)

for header in "${new_renpy_headers[@]}"; do
    src_path="renpy-source/module/${header}"
    if [ -f "$src_path" ]; then
        ln -sf $(realpath "$src_path") "include/module/${header}"
        echo "Found Ren'Py 8.x header: ${header}"
    fi
done

echo "Link process completed!"

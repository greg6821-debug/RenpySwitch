#!/bin/bash

mkdir -p source/module
mkdir -p include/module include/module/pygame_sdl2 include/module/src

# Проверяем существование файлов перед созданием ссылок
check_and_link() {
    if [ -f "$1" ]; then
        ln -sf "$(realpath "$1")" "$2"
        echo "Linked: $2"
    else
        echo "Warning: Source file not found: $1"
    fi
}

# Файлы pygame_sdl2
check_and_link "pygame_sdl2-source/gen-static/pygame_sdl2.color.c" "source/module/pygame_sdl2.color.c"
check_and_link "pygame_sdl2-source/gen-static/pygame_sdl2.controller.c" "source/module/pygame_sdl2.controller.c"
check_and_link "pygame_sdl2-source/gen-static/pygame_sdl2.display.c" "source/module/pygame_sdl2.display.c"
check_and_link "pygame_sdl2-source/gen-static/pygame_sdl2.draw.c" "source/module/pygame_sdl2.draw.c"
check_and_link "pygame_sdl2-source/gen-static/pygame_sdl2.error.c" "source/module/pygame_sdl2.error.c"
check_and_link "pygame_sdl2-source/gen-static/pygame_sdl2.event.c" "source/module/pygame_sdl2.event.c"
check_and_link "pygame_sdl2-source/gen-static/pygame_sdl2.font.c" "source/module/pygame_sdl2.font.c"
check_and_link "pygame_sdl2-source/gen-static/pygame_sdl2.gfxdraw.c" "source/module/pygame_sdl2.gfxdraw.c"
check_and_link "pygame_sdl2-source/gen-static/pygame_sdl2.image.c" "source/module/pygame_sdl2.image.c"
check_and_link "pygame_sdl2-source/gen-static/pygame_sdl2.joystick.c" "source/module/pygame_sdl2.joystick.c"
check_and_link "pygame_sdl2-source/gen-static/pygame_sdl2.key.c" "source/module/pygame_sdl2.key.c"
check_and_link "pygame_sdl2-source/gen-static/pygame_sdl2.locals.c" "source/module/pygame_sdl2.locals.c"
check_and_link "pygame_sdl2-source/gen-static/pygame_sdl2.mixer.c" "source/module/pygame_sdl2.mixer.c"
check_and_link "pygame_sdl2-source/gen-static/pygame_sdl2.mixer_music.c" "source/module/pygame_sdl2.mixer_music.c"
check_and_link "pygame_sdl2-source/gen-static/pygame_sdl2.mouse.c" "source/module/pygame_sdl2.mouse.c"
check_and_link "pygame_sdl2-source/gen-static/pygame_sdl2.power.c" "source/module/pygame_sdl2.power.c"
check_and_link "pygame_sdl2-source/gen-static/pygame_sdl2.pygame_time.c" "source/module/pygame_sdl2.pygame_time.c"
check_and_link "pygame_sdl2-source/gen-static/pygame_sdl2.rect.c" "source/module/pygame_sdl2.rect.c"
check_and_link "pygame_sdl2-source/gen-static/pygame_sdl2.render.c" "source/module/pygame_sdl2.render.c"
check_and_link "pygame_sdl2-source/gen-static/pygame_sdl2.rwobject.c" "source/module/pygame_sdl2.rwobject.c"
check_and_link "pygame_sdl2-source/gen-static/pygame_sdl2.scrap.c" "source/module/pygame_sdl2.scrap.c"
check_and_link "pygame_sdl2-source/gen-static/pygame_sdl2.surface.c" "source/module/pygame_sdl2.surface.c"
check_and_link "pygame_sdl2-source/gen-static/pygame_sdl2.transform.c" "source/module/pygame_sdl2.transform.c"

# Исходные файлы из pygame_sdl2
check_and_link "pygame_sdl2-source/src/SDL2_rotozoom.c" "source/module/SDL2_rotozoom.c"
check_and_link "pygame_sdl2-source/src/SDL_gfxPrimitives.c" "source/module/SDL_gfxPrimitives.c"
check_and_link "pygame_sdl2-source/src/alphablit.c" "source/module/alphablit.c"
check_and_link "pygame_sdl2-source/src/write_jpeg.c" "source/module/write_jpeg.c"
check_and_link "pygame_sdl2-source/src/write_png.c" "source/module/write_png.c"

# Файлы из renpy-source
check_and_link "renpy-source/module/IMG_savepng.c" "source/module/IMG_savepng.c"
check_and_link "renpy-source/module/core.c" "source/module/core.c"
check_and_link "renpy-source/module/egl_none.c" "source/module/egl_none.c"
check_and_link "renpy-source/module/ffmedia.c" "source/module/ffmedia.c"
check_and_link "renpy-source/module/ftsupport.c" "source/module/ftsupport.c"

# Сгенерированные файлы renpy
check_and_link "renpy-source/module/gen-static/_renpy.c" "source/module/_renpy.c"
check_and_link "renpy-source/module/gen-static/_renpybidi.c" "source/module/_renpybidi.c"
check_and_link "renpy-source/module/gen-static/renpy.audio.renpysound.c" "source/module/renpy.audio.renpysound.c"
check_and_link "renpy-source/module/gen-static/renpy.display.accelerator.c" "source/module/renpy.display.accelerator.c"
check_and_link "renpy-source/module/gen-static/renpy.display.matrix.c" "source/module/renpy.display.matrix.c"
check_and_link "renpy-source/module/gen-static/renpy.display.render.c" "source/module/renpy.display.render.c"
check_and_link "renpy-source/module/gen-static/renpy.gl.gldraw.c" "source/module/renpy.gl.gldraw.c"
check_and_link "renpy-source/module/gen-static/renpy.gl.glenviron_shader.c" "source/module/renpy.gl.glenviron_shader.c"
check_and_link "renpy-source/module/gen-static/renpy.gl.glrtt_copy.c" "source/module/renpy.gl.glrtt_copy.c"
check_and_link "renpy-source/module/gen-static/renpy.gl.glrtt_fbo.c" "source/module/renpy.gl.glrtt_fbo.c"
check_and_link "renpy-source/module/gen-static/renpy.gl.gltexture.c" "source/module/renpy.gl.gltexture.c"
check_and_link "renpy-source/module/gen-static/renpy.pydict.c" "source/module/renpy.pydict.c"
check_and_link "renpy-source/module/gen-static/renpy.style.c" "source/module/renpy.style.c"
check_and_link "renpy-source/module/gen-static/renpy.styledata.style_activate_functions.c" "source/module/renpy.styledata.style_activate_functions.c"
check_and_link "renpy-source/module/gen-static/renpy.styledata.style_functions.c" "source/module/renpy.styledata.style_functions.c"
check_and_link "renpy-source/module/gen-static/renpy.styledata.style_hover_functions.c" "source/module/renpy.styledata.style_hover_functions.c"
check_and_link "renpy-source/module/gen-static/renpy.styledata.style_idle_functions.c" "source/module/renpy.styledata.style_idle_functions.c"
check_and_link "renpy-source/module/gen-static/renpy.styledata.style_insensitive_functions.c" "source/module/renpy.styledata.style_insensitive_functions.c"
check_and_link "renpy-source/module/gen-static/renpy.styledata.style_selected_activate_functions.c" "source/module/renpy.styledata.style_selected_activate_functions.c"
check_and_link "renpy-source/module/gen-static/renpy.styledata.style_selected_functions.c" "source/module/renpy.styledata.style_selected_functions.c"
check_and_link "renpy-source/module/gen-static/renpy.styledata.style_selected_hover_functions.c" "source/module/renpy.styledata.style_selected_hover_functions.c"
check_and_link "renpy-source/module/gen-static/renpy.styledata.style_selected_idle_functions.c" "source/module/renpy.styledata.style_selected_idle_functions.c"
check_and_link "renpy-source/module/gen-static/renpy.styledata.style_selected_insensitive_functions.c" "source/module/renpy.styledata.style_selected_insensitive_functions.c"
check_and_link "renpy-source/module/gen-static/renpy.styledata.styleclass.c" "source/module/renpy.styledata.styleclass.c"
check_and_link "renpy-source/module/gen-static/renpy.styledata.stylesets.c" "source/module/renpy.styledata.stylesets.c"
check_and_link "renpy-source/module/gen-static/renpy.text.ftfont.c" "source/module/renpy.text.ftfont.c"
check_and_link "renpy-source/module/gen-static/renpy.text.textsupport.c" "source/module/renpy.text.textsupport.c"
check_and_link "renpy-source/module/gen-static/renpy.text.texwrap.c" "source/module/renpy.text.texwrap.c"

# Дополнительные файлы renpy
check_and_link "renpy-source/module/renpybidicore.c" "source/module/renpybidicore.c"
check_and_link "renpy-source/module/renpysound_core.c" "source/module/renpysound_core.c"
check_and_link "renpy-source/module/ttgsubtable.c" "source/module/ttgsubtable.c"

# Заголовочные файлы pygame_sdl2
check_and_link "pygame_sdl2-source/gen-static/pygame_sdl2.display_api.h" "include/module/pygame_sdl2/pygame_sdl2.display_api.h"
check_and_link "pygame_sdl2-source/gen-static/pygame_sdl2.event.h" "include/module/pygame_sdl2/pygame_sdl2.event.h"
check_and_link "pygame_sdl2-source/gen-static/pygame_sdl2.rwobject_api.h" "include/module/pygame_sdl2/pygame_sdl2.rwobject_api.h"
check_and_link "pygame_sdl2-source/gen-static/pygame_sdl2.surface_api.h" "include/module/pygame_sdl2/pygame_sdl2.surface_api.h"
check_and_link "pygame_sdl2-source/src/SDL2_rotozoom.h" "include/module/SDL2_rotozoom.h"
check_and_link "pygame_sdl2-source/src/SDL_gfxPrimitives.h" "include/module/SDL_gfxPrimitives.h"
check_and_link "pygame_sdl2-source/src/SDL_gfxPrimitives_font.h" "include/module/SDL_gfxPrimitives_font.h"
check_and_link "pygame_sdl2-source/src/pygame_sdl2/pygame_sdl2.h" "include/module/pygame_sdl2/pygame_sdl2.h"
check_and_link "pygame_sdl2-source/src/python_threads.h" "include/module/python_threads.h"
check_and_link "pygame_sdl2-source/src/surface.h" "include/module/src/surface.h"
check_and_link "pygame_sdl2-source/src/write_jpeg.h" "include/module/write_jpeg.h"
check_and_link "pygame_sdl2-source/src/write_png.h" "include/module/write_png.h"
check_and_link "pygame_sdl2-source/src/sdl_image_compat.h" "include/module/sdl_image_compat.h"

# Заголовочные файлы renpy
check_and_link "renpy-source/module/IMG_savepng.h" "include/module/IMG_savepng.h"
check_and_link "renpy-source/module/eglsupport.h" "include/module/eglsupport.h"
check_and_link "renpy-source/module/ftsupport.h" "include/module/ftsupport.h"
check_and_link "renpy-source/module/gl2debug.h" "include/module/gl2debug.h"
check_and_link "renpy-source/module/glcompat.h" "include/module/glcompat.h"
check_and_link "renpy-source/module/renpy.h" "include/module/renpy.h"
check_and_link "renpy-source/module/renpybidicore.h" "include/module/renpybidicore.h"
check_and_link "renpy-source/module/renpygl.h" "include/module/renpygl.h"
check_and_link "renpy-source/module/renpysound_core.h" "include/module/renpysound_core.h"
check_and_link "renpy-source/module/ttgsubtable.h" "include/module/ttgsubtable.h"

# Новые модули для Ren'Py 8
check_and_link "renpy-source/module/gen-static/renpy.compat.dictviews.c" "source/module/renpy.compat.dictviews.c"
check_and_link "renpy-source/module/gen-static/renpy.gl2.gl2draw.c" "source/module/renpy.gl2.gl2draw.c"
check_and_link "renpy-source/module/gen-static/renpy.gl2.gl2mesh.c" "source/module/renpy.gl2.gl2mesh.c"
check_and_link "renpy-source/module/gen-static/renpy.gl2.gl2mesh2.c" "source/module/renpy.gl2.gl2mesh2.c"
check_and_link "renpy-source/module/gen-static/renpy.gl2.gl2mesh3.c" "source/module/renpy.gl2.gl2mesh3.c"
check_and_link "renpy-source/module/gen-static/renpy.gl2.gl2model.c" "source/module/renpy.gl2.gl2model.c"
check_and_link "renpy-source/module/gen-static/renpy.gl2.gl2polygon.c" "source/module/renpy.gl2.gl2polygon.c"
check_and_link "renpy-source/module/gen-static/renpy.gl2.gl2shader.c" "source/module/renpy.gl2.gl2shader.c"
check_and_link "renpy-source/module/gen-static/renpy.gl2.gl2texture.c" "source/module/renpy.gl2.gl2texture.c"
check_and_link "renpy-source/module/gen-static/renpy.uguu.gl.c" "source/module/renpy.uguu.gl.c"
check_and_link "renpy-source/module/gen-static/renpy.uguu.uguu.c" "source/module/renpy.uguu.uguu.c"
check_and_link "renpy-source/module/gen-static/renpy.lexersupport.c" "source/module/renpy.lexersupport.c"
check_and_link "renpy-source/module/gen-static/renpy.display.quaternion.c" "source/module/renpy.display.quaternion.c"

# Для Ren'Py 8.3.7 могут быть дополнительные файлы
check_and_link "renpy-source/module/gen-static/renpy.encryption.c" "source/module/renpy.encryption.c"
check_and_link "renpy-source/module/gen-static/renpy.execution.c" "source/module/renpy.execution.c" 2>/dev/null || true
check_and_link "renpy-source/module/gen-static/renpy.arguments.c" "source/module/renpy.arguments.c" 2>/dev/null || true

echo "Linking completed!"

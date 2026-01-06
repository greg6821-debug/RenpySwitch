init python:
    # Загрузка всех .rpym модулей для Python 3.9
    import renpy
    
    for x in renpy.list_files(True):
        if x.endswith(".rpym"):
            try:
                module_name = x.rsplit(".", 1)[0].replace("/", ".")
                renpy.load_module(module_name)
            except Exception as e:
                print(f"Failed to load module {x}: {e}")
                pass

# Для совместимости с Python 3.9
init python hide:
    # Настройки для Switch
    config.save_directory = "/switch/renpy/saves"
    config.window_title = "Ren'Py 8.3.7 for Switch"
    config.screen_width = 1280
    config.screen_height = 720
    config.gl_resize = True
    
    # Оптимизации для Switch
    config.cache_surfaces = True
    config.image_cache_size = 256
    config.predictive_page_size = 32

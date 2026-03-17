project('animationwindow', ['c', 'cpp'], version: '0.01', default_options: ['cpp_std=c++20', 'c_std=c11', 'default_library=static', 'buildtype=debugoptimized'])

if host_machine.system() == 'windows'
    # Prefer normal dependency resolution on Windows so MSYS2 packages work in
    # CI and local builds. Meson can still fall back to vendored SDL wraps.
    sdl2_dep = dependency('sdl2')

    sdl2image_dep = dependency('sdl2_image', required: false)
    if not sdl2image_dep.found()
        sdl2image_dep = dependency('SDL2_image')
    endif

    sdl2mixer_dep = dependency('SDL2_mixer', required: false)
    if not sdl2mixer_dep.found()
        sdl2mixer_dep = dependency('sdl2_mixer')
    endif
elif host_machine.system() == 'emscripten'
    # Emscripten provides SDL2 as built-in ports; we declare them as empty
    # dependencies since the compiler/linker flags are set at the top level.
    sdl2_dep = declare_dependency(
        compile_args: ['-sUSE_SDL=2'],
        link_args: ['-sUSE_SDL=2']
    )
    sdl2image_dep = declare_dependency(
        compile_args: ['-sUSE_SDL_IMAGE=2', '-sSDL2_IMAGE_FORMATS=["png"]'],
        link_args: ['-sUSE_SDL_IMAGE=2', '-sSDL2_IMAGE_FORMATS=["png"]']
    )
    sdl2mixer_dep = declare_dependency(
        compile_args: ['-sUSE_SDL_MIXER=2'],
        link_args: ['-sUSE_SDL_MIXER=2']
    )
else
    sdl2_dep = dependency('sdl2')
    sdl2image_dep = dependency('sdl2_image')
    sdl2mixer_dep = dependency('SDL2_mixer')
endif


build_files = [
    'src/internal/FontCache.cpp', 
    'src/internal/KeyboardKeyConverter.cpp',
    'src/internal/nuklear_implementation.cpp',
    'src/widgets/Button.cpp',
    'src/widgets/RadioButton.cpp',
    'src/widgets/CheckBox.cpp',
    'src/widgets/TextInput.cpp',
    'src/widgets/TextBox.cpp',
    'src/widgets/DropdownList.cpp',
    'src/widgets/Slider.cpp',
    'src/AnimationWindow.cpp', 
    'src/Color.cpp', 
    'src/Image.cpp', 
    'src/Audio.cpp', 
    'src/Widget.cpp']
incdir = include_directories('include')
animationwindow = static_library('animationwindow', build_files, include_directories: incdir, dependencies: [sdl2_dep, sdl2image_dep, sdl2mixer_dep], install: true)
install_subdir('include', install_dir: '.')
install_subdir('src', install_dir: '.')

animationwindow_dep = declare_dependency(
  link_with: animationwindow,
  include_directories: incdir,
  dependencies: [sdl2_dep, sdl2image_dep, sdl2mixer_dep]
)

import('pkgconfig').generate(animationwindow)

# ex = executable('prog', 'src/test.cpp', include_directories: incdir, dependencies: [sdl2_dep, imgui_dep], link_with: animationwindow)

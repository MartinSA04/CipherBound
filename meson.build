project('GUI Project', 'cpp',
  version : '0.1',
  default_options : [
    'warning_level=2',
    'cpp_std=c++20',
    'c_std=c11',
    'default_library=static'
  ])

animationwindow_dep = dependency('animationwindow', fallback: ['animationwindow', 'animationwindow_dep'])
std_lib_facilities_dep = dependency('std_lib_facilities', fallback: ['std_lib_facilities', 'std_lib_facilities_dep'])

compiler_flags = [
  '-Wconversion',
  '-fdiagnostics-color=always',
  '-Werror=return-type'
]

if host_machine.system() != 'emscripten'
  compiler_flags += ['-static']
endif

src = [
  'src/main.cpp',
  'src/core/Session.cpp',
  'src/core/GameMode.cpp',
  'src/core/StoryManager.cpp',
  'src/core/CutsceneRunner.cpp',
  'src/core/modes/OverworldMode.cpp',
  'src/core/modes/overworld/Movement.cpp',
  'src/core/modes/overworld/Interaction.cpp',
  'src/core/modes/overworld/FlowChecks.cpp',
  'src/core/modes/MenuMode.cpp',
  'src/core/modes/menu/Render.cpp',
  'src/core/modes/BattleMode.cpp',
  'src/core/modes/battlemode/Update.cpp',
  'src/core/modes/battlemode/Render.cpp',
  'src/core/modes/battlemode/CaptureAnimation.cpp',
  'src/core/modes/BattleIntroMode.cpp',
  'src/core/modes/TitleScreenMode.cpp',
  'src/core/modes/TransitionMode.cpp',
  'src/core/modes/PartyMode.cpp',
  'src/core/modes/BagMode.cpp',
  'src/core/modes/SaveMode.cpp',
  'src/core/modes/DialogueMode.cpp',
  'src/core/modes/DialogueChoiceMode.cpp',
  'src/core/modes/PCBoxMode.cpp',
  'src/core/modes/bag/BrowsingState.cpp',
  'src/core/modes/bag/TargetState.cpp',
  'src/core/modes/bag/MessageState.cpp',
  'src/core/modes/CutSceneMode.cpp',
  'src/core/modes/DaemondexMode.cpp',
  'src/state/Entity.cpp',
  'src/state/Daemon.cpp',
  'src/state/player/Player.cpp',
  'src/state/NPC.cpp',
  'src/state/Map.cpp',
  'src/state/World.cpp',
  'src/state/world/MapLoading.cpp',
  'src/state/world/NPCs.cpp',
  'src/state/world/Encounters.cpp',
  'src/state/player/DaemonDex.cpp',
  'src/state/player/EventFlags.cpp',
  'src/state/player/Inventory.cpp',
  'src/state/Movement.cpp',
  'src/state/player/Party.cpp',
  'src/state/player/WalkAnimation.cpp',
  'src/data/Pokedex.cpp',
  'src/battle/Battle.cpp',
  'src/battle/TypeChart.cpp',
  'src/save/SaveManager.cpp',
  'src/audio/MusicManager.cpp',
  'src/audio/SoundManager.cpp',
  'src/ui/Renderer.cpp',
  'src/ui/InputManager.cpp',
  'src/ui/OverworldRenderer.cpp',
  'src/ui/GameUI.cpp',
  'src/ui/gameui/MenuScreens.cpp',
  'src/ui/gameui/Dialogue.cpp',
  'src/ui/gameui/Battle.cpp',
  'src/ui/SpriteFont.cpp',
]

if host_machine.system() == 'windows'
  ld_flags = ['-static', '-lstdc++exp']
elif host_machine.system() == 'emscripten'
  ld_flags = [
    '-sUSE_SDL=2',
    '-sUSE_SDL_IMAGE=2',
    '-sSDL2_IMAGE_FORMATS=["png"]',
    '-sUSE_SDL_MIXER=2',
    '-sSDL2_MIXER_FORMATS=["mp3"]',
    '-sALLOW_MEMORY_GROWTH=1',
    '-sINITIAL_MEMORY=67108864',
    '-sNO_DISABLE_EXCEPTION_CATCHING',
    '-lidbfs.js',
    '--preload-file=' + meson.project_source_root() / 'assets' + '@' + '/assets',
    '--shell-file=' + meson.project_source_root() / 'shell.html',
    '-o', 'index.html',
  ]

  compiler_flags += [
    '-sUSE_SDL=2',
    '-sUSE_SDL_IMAGE=2',
    '-sUSE_SDL_MIXER=2',
    '-fexceptions',
  ]
else
  ld_flags = []
endif

exe = executable(
  'program',
  src,
  dependencies : [std_lib_facilities_dep, animationwindow_dep],
  cpp_args : compiler_flags,
  link_args : ld_flags
)
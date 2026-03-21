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

common_src = [
  'src/common/FilePaths.cpp',
]

story_src = [
  'src/story/StoryManager.cpp',
]

cutscene_src = [
  'src/cutscene/CutsceneLoader.cpp',
  'src/cutscene/CutscenePlayback.cpp',
  'src/cutscene/CutsceneRunner.cpp',
  'src/cutscene/CutsceneFormat.cpp',
  'src/cutscene/CutsceneWorldOps.cpp',
]

app_src = [
  'src/app/Session.cpp',
  'src/app/SessionCoordinator.cpp',
  'src/app/GameMode.cpp',
  'src/app/modes/OverworldMode.cpp',
  'src/app/modes/overworld/Movement.cpp',
  'src/app/modes/overworld/Interaction.cpp',
  'src/app/modes/overworld/FlowChecks.cpp',
  'src/app/modes/MenuMode.cpp',
  'src/app/modes/menu/Render.cpp',
  'src/app/modes/TitleScreenMode.cpp',
  'src/app/modes/TransitionMode.cpp',
  'src/app/modes/PartyMode.cpp',
  'src/app/modes/BagMode.cpp',
  'src/app/modes/ShopMode.cpp',
  'src/app/modes/SaveMode.cpp',
  'src/app/modes/DialogueMode.cpp',
  'src/app/modes/DialogueChoiceMode.cpp',
  'src/app/modes/PCBoxMode.cpp',
  'src/app/modes/bag/BrowsingState.cpp',
  'src/app/modes/bag/TargetState.cpp',
  'src/app/modes/bag/MessageState.cpp',
  'src/app/modes/CutSceneMode.cpp',
  'src/app/modes/DaemondexMode.cpp',
]

game_data_src = [
  'src/game_data/Pokedex.cpp',
]

state_src = [
  'src/state/Entity.cpp',
  'src/state/Daemon.cpp',
  'src/state/player/Player.cpp',
  'src/state/NPC.cpp',
  'src/state/Map.cpp',
  'src/state/World.cpp',
  'src/state/world/MapLoading.cpp',
  'src/state/world/MapFormat.cpp',
  'src/state/world/NPCs.cpp',
  'src/state/world/Encounters.cpp',
  'src/state/player/DaemonDex.cpp',
  'src/state/player/EventFlags.cpp',
  'src/state/player/Inventory.cpp',
  'src/state/Movement.cpp',
  'src/state/player/Party.cpp',
  'src/state/player/WalkAnimation.cpp',
]

battle_core_src = [
  'src/battle/Battle.cpp',
  'src/battle/BattleCapture.cpp',
  'src/battle/BattleEventQueue.cpp',
  'src/battle/BattleAI.cpp',
  'src/battle/BattleRules.cpp',
  'src/battle/TrainerApproachCutscene.cpp',
  'src/battle/BattleTurnResolver.cpp',
  'src/battle/TypeChart.cpp',
]

battle_mode_src = [
  'src/battle/mode/BattleMode.cpp',
  'src/battle/mode/Update.cpp',
  'src/battle/mode/Render.cpp',
  'src/battle/mode/CaptureAnimation.cpp',
  'src/battle/mode/BattleIntroMode.cpp',
]

battle_ui_src = [
  'src/battle/ui/BattleRenderer.cpp',
  'src/battle/ui/BattlePresentationState.cpp',
]

save_src = [
  'src/save/SaveFormat.cpp',
  'src/save/SaveManager.cpp',
]

audio_src = [
  'src/audio/MusicManager.cpp',
  'src/audio/SoundManager.cpp',
]

ui_src = [
  'src/ui/Renderer.cpp',
  'src/ui/InputManager.cpp',
  'src/ui/OverworldRenderer.cpp',
  'src/ui/GameUI.cpp',
  'src/ui/gameui/MenuScreens.cpp',
  'src/ui/gameui/Dialogue.cpp',
  'src/ui/gameui/StatusWidgets.cpp',
  'src/ui/SpriteFont.cpp',
]

src = ['src/main.cpp']
src += common_src
src += story_src
src += cutscene_src
src += app_src
src += game_data_src
src += state_src
src += battle_core_src
src += battle_mode_src
src += battle_ui_src
src += save_src
src += audio_src
src += ui_src

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

test_bash = find_program('bash')
doxygen_prog = find_program('doxygen', required : false)
dot_prog = find_program('dot', required : false)

string_utils_test = executable(
  'string_utils_test',
  'tests/common/StringUtilsTest.cpp'
)

test('string_utils', string_utils_test)

mode_request_test = executable(
  'mode_request_test',
  'tests/ModeRequestTest.cpp'
)

test('mode_request', mode_request_test)

story_action_test = executable(
  'story_action_test',
  'tests/story/StoryActionTest.cpp'
)

test('story_action', story_action_test)

text_parse_test = executable(
  'text_parse_test',
  'tests/common/TextParseTest.cpp'
)

test('text_parse', text_parse_test)

save_format_test = executable(
  'save_format_test',
  [
    'tests/SaveFormatTest.cpp',
    'src/game_data/Pokedex.cpp',
    'src/state/Daemon.cpp',
    'src/save/SaveFormat.cpp',
  ],
)

test('save_format', save_format_test)

cutscene_format_test = executable(
  'cutscene_format_test',
  [
    'tests/cutscene/CutsceneFormatTest.cpp',
    'src/cutscene/CutsceneFormat.cpp',
  ],
)

test('cutscene_format', cutscene_format_test)

cutscene_loader_test = executable(
  'cutscene_loader_test',
  [
    'tests/cutscene/CutsceneLoaderTest.cpp',
    'src/cutscene/CutsceneLoader.cpp',
    'src/cutscene/CutsceneFormat.cpp',
    'src/common/FilePaths.cpp',
  ],
)

test('cutscene_loader', cutscene_loader_test)

file_paths_test = executable(
  'file_paths_test',
  [
    'tests/common/FilePathsTest.cpp',
    'src/cutscene/CutsceneFormat.cpp',
    'src/common/FilePaths.cpp',
  ],
)

test('file_paths', file_paths_test)

cutscene_playback_test = executable(
  'cutscene_playback_test',
  [
    'tests/cutscene/CutscenePlaybackTest.cpp',
    'src/cutscene/CutscenePlayback.cpp',
  ],
)

test('cutscene_playback', cutscene_playback_test)

cutscene_world_ops_test = executable(
  'cutscene_world_ops_test',
  [
    'tests/cutscene/CutsceneWorldOpsTest.cpp',
    'src/cutscene/CutsceneWorldOps.cpp',
    'src/state/Daemon.cpp',
    'src/state/Entity.cpp',
    'src/state/Map.cpp',
    'src/state/Movement.cpp',
    'src/state/NPC.cpp',
    'src/state/World.cpp',
    'src/state/world/NPCs.cpp',
    'src/state/player/DaemonDex.cpp',
    'src/state/player/EventFlags.cpp',
    'src/state/player/Inventory.cpp',
    'src/state/player/Party.cpp',
    'src/state/player/Player.cpp',
    'src/state/player/WalkAnimation.cpp',
  ],
)

test('cutscene_world_ops', cutscene_world_ops_test)

map_format_test = executable(
  'map_format_test',
  [
    'tests/MapFormatTest.cpp',
    'src/state/world/MapFormat.cpp',
  ],
)

test('map_format', map_format_test)

battle_event_queue_test = executable(
  'battle_event_queue_test',
  [
    'tests/BattleEventQueueTest.cpp',
    'src/battle/BattleEventQueue.cpp',
  ],
)

test('battle_event_queue', battle_event_queue_test)

battle_animation_utils_test = executable(
  'battle_animation_utils_test',
  'tests/BattleAnimationUtilsTest.cpp'
)

test('battle_animation_utils', battle_animation_utils_test)

battle_presentation_state_test = executable(
  'battle_presentation_state_test',
  [
    'tests/BattlePresentationStateTest.cpp',
    'src/battle/ui/BattlePresentationState.cpp',
  ],
)

test('battle_presentation_state', battle_presentation_state_test)

battle_capture_test = executable(
  'battle_capture_test',
  [
    'tests/BattleCaptureTest.cpp',
    'src/battle/BattleCapture.cpp',
    'src/state/Daemon.cpp',
  ],
)

test('battle_capture', battle_capture_test)

battle_turn_resolver_test = executable(
  'battle_turn_resolver_test',
  [
    'tests/BattleTurnResolverTest.cpp',
    'src/battle/BattleTurnResolver.cpp',
    'src/battle/BattleAI.cpp',
    'src/battle/BattleRules.cpp',
    'src/battle/TypeChart.cpp',
    'src/game_data/Pokedex.cpp',
    'src/state/Daemon.cpp',
  ],
)

test('battle_turn_resolver', battle_turn_resolver_test)

battle_ai_test = executable(
  'battle_ai_test',
  [
    'tests/BattleAITest.cpp',
    'src/battle/BattleAI.cpp',
    'src/battle/TypeChart.cpp',
    'src/state/Daemon.cpp',
  ],
)

test('battle_ai', battle_ai_test)

battle_rules_test = executable(
  'battle_rules_test',
  [
    'tests/BattleRulesTest.cpp',
    'src/battle/BattleRules.cpp',
    'src/battle/TypeChart.cpp',
    'src/state/Daemon.cpp',
  ],
)

test('battle_rules', battle_rules_test)

battle_flow_test = executable(
  'battle_flow_test',
  [
    'tests/BattleFlowTest.cpp',
    'src/battle/Battle.cpp',
    'src/battle/BattleCapture.cpp',
    'src/battle/BattleEventQueue.cpp',
    'src/battle/BattleAI.cpp',
    'src/battle/BattleRules.cpp',
    'src/battle/BattleTurnResolver.cpp',
    'src/battle/TypeChart.cpp',
    'src/game_data/Pokedex.cpp',
    'src/state/Daemon.cpp',
    'src/state/Entity.cpp',
    'src/state/Map.cpp',
    'src/state/Movement.cpp',
    'src/state/NPC.cpp',
    'src/state/player/DaemonDex.cpp',
    'src/state/player/EventFlags.cpp',
    'src/state/player/Inventory.cpp',
    'src/state/player/Party.cpp',
    'src/state/player/Player.cpp',
    'src/state/player/WalkAnimation.cpp',
  ],
)

test('battle_flow', battle_flow_test)

trainer_approach_cutscene_test = executable(
  'trainer_approach_cutscene_test',
  [
    'tests/TrainerApproachCutsceneTest.cpp',
    'src/battle/TrainerApproachCutscene.cpp',
    'src/state/Daemon.cpp',
    'src/state/Entity.cpp',
    'src/state/Map.cpp',
    'src/state/Movement.cpp',
    'src/state/NPC.cpp',
    'src/state/player/DaemonDex.cpp',
    'src/state/player/EventFlags.cpp',
    'src/state/player/Inventory.cpp',
    'src/state/player/Party.cpp',
    'src/state/player/Player.cpp',
    'src/state/player/WalkAnimation.cpp',
  ],
)

test('trainer_approach_cutscene', trainer_approach_cutscene_test)

map_model_test = executable(
  'map_model_test',
  [
    'tests/MapModelTest.cpp',
    'src/state/Map.cpp',
  ],
)

test('map_model', map_model_test)

daemon_exp_test = executable(
  'daemon_exp_test',
  [
    'tests/DaemonExpTest.cpp',
    'src/state/Daemon.cpp',
  ],
)

test('daemon_exp', daemon_exp_test)

state_model_test = executable(
  'state_model_test',
  [
    'tests/StateModelTest.cpp',
    'src/state/Daemon.cpp',
    'src/state/Entity.cpp',
    'src/state/Map.cpp',
    'src/state/Movement.cpp',
    'src/state/NPC.cpp',
    'src/state/player/DaemonDex.cpp',
    'src/state/player/EventFlags.cpp',
    'src/state/player/Inventory.cpp',
    'src/state/player/Party.cpp',
    'src/state/player/Player.cpp',
    'src/state/player/WalkAnimation.cpp',
  ],
)

test('state_model', state_model_test)

world_respawn_test = executable(
  'world_respawn_test',
  [
    'tests/WorldRespawnTest.cpp',
    'src/state/Daemon.cpp',
    'src/state/Entity.cpp',
    'src/state/Map.cpp',
    'src/state/Movement.cpp',
    'src/state/NPC.cpp',
    'src/state/World.cpp',
    'src/state/world/NPCs.cpp',
    'src/state/player/DaemonDex.cpp',
    'src/state/player/EventFlags.cpp',
    'src/state/player/Inventory.cpp',
    'src/state/player/Party.cpp',
    'src/state/player/Player.cpp',
    'src/state/player/WalkAnimation.cpp',
  ],
)

test('world_respawn', world_respawn_test)

world_generate_test = executable(
  'world_generate_test',
  [
    'tests/WorldGenerateTest.cpp',
    'src/common/FilePaths.cpp',
    'src/game_data/Pokedex.cpp',
    'src/state/Daemon.cpp',
    'src/state/Entity.cpp',
    'src/state/Map.cpp',
    'src/state/Movement.cpp',
    'src/state/NPC.cpp',
    'src/state/World.cpp',
    'src/state/world/Encounters.cpp',
    'src/state/world/MapFormat.cpp',
    'src/state/world/MapLoading.cpp',
    'src/state/world/NPCs.cpp',
    'src/state/player/DaemonDex.cpp',
    'src/state/player/EventFlags.cpp',
    'src/state/player/Inventory.cpp',
    'src/state/player/Party.cpp',
    'src/state/player/Player.cpp',
    'src/state/player/WalkAnimation.cpp',
  ],
)

test('world_generate', world_generate_test)

save_manager_roundtrip_test = executable(
  'save_manager_roundtrip_test',
  [
    'tests/SaveManagerRoundTripTest.cpp',
    'src/game_data/Pokedex.cpp',
    'src/save/SaveFormat.cpp',
    'src/save/SaveManager.cpp',
    'src/state/Daemon.cpp',
    'src/state/Entity.cpp',
    'src/state/Map.cpp',
    'src/state/Movement.cpp',
    'src/state/NPC.cpp',
    'src/state/World.cpp',
    'src/state/world/NPCs.cpp',
    'src/state/player/DaemonDex.cpp',
    'src/state/player/EventFlags.cpp',
    'src/state/player/Inventory.cpp',
    'src/state/player/Party.cpp',
    'src/state/player/Player.cpp',
    'src/state/player/WalkAnimation.cpp',
  ],
)

test('save_manager_roundtrip', save_manager_roundtrip_test)

world_npc_ownership_test = executable(
  'world_npc_ownership_test',
  [
    'tests/WorldNpcOwnershipTest.cpp',
    'src/state/Daemon.cpp',
    'src/state/Entity.cpp',
    'src/state/Map.cpp',
    'src/state/Movement.cpp',
    'src/state/NPC.cpp',
    'src/state/World.cpp',
    'src/state/world/NPCs.cpp',
    'src/state/player/DaemonDex.cpp',
    'src/state/player/EventFlags.cpp',
    'src/state/player/Inventory.cpp',
    'src/state/player/Party.cpp',
    'src/state/player/Player.cpp',
    'src/state/player/WalkAnimation.cpp',
  ],
)

test('world_npc_ownership', world_npc_ownership_test)

test(
  'tooling_smoke',
  test_bash,
  args : [
    meson.project_source_root() / 'tests' / 'check_tooling.sh',
    meson.project_source_root(),
    meson.project_build_root(),
  ],
)

if doxygen_prog.found() and dot_prog.found()
  test(
    'docs_smoke',
    test_bash,
    args : [
      meson.project_source_root() / 'tests' / 'check_docs.sh',
      meson.project_source_root(),
    ],
  )
endif

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Test)

add_executable(test_gpuinfo
  test_gpuinfo.cpp
  ${CMAKE_SOURCE_DIR}/src/gpuinfo.cpp
)
target_include_directories(test_gpuinfo PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_gpuinfo PRIVATE Qt6::Core Qt6::Test)
if(WIN32)
  target_link_libraries(test_gpuinfo PRIVATE dxgi dxguid)
endif()
add_test(NAME gpuinfo COMMAND test_gpuinfo)

add_executable(test_mix_transition_order
  test_mix_transition_order.cpp
  ${CMAKE_SOURCE_DIR}/src/models/mixtransitionorder.cpp
)
target_include_directories(test_mix_transition_order PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_mix_transition_order PRIVATE Qt6::Core Qt6::Test PkgConfig::mlt++)
add_test(NAME mix_transition_order COMMAND test_mix_transition_order)

add_executable(test_marker_generator
  test_marker_generator.cpp
  ${CMAKE_SOURCE_DIR}/src/rhythm/beatgrid.cpp
  ${CMAKE_SOURCE_DIR}/src/rhythm/markergenerator.cpp
)
target_include_directories(test_marker_generator PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_marker_generator PRIVATE Qt6::Core Qt6::Gui Qt6::Test PkgConfig::mlt++)
add_test(NAME marker_generator COMMAND test_marker_generator)

add_executable(test_beat_grid
  test_beat_grid.cpp
  ${CMAKE_SOURCE_DIR}/src/rhythm/beatgrid.cpp
)
target_include_directories(test_beat_grid PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_beat_grid PRIVATE Qt6::Core Qt6::Test)
add_test(NAME beat_grid COMMAND test_beat_grid)

add_executable(test_midi_source
  test_midi_source.cpp
  ${CMAKE_SOURCE_DIR}/src/rhythm/midisource.cpp
)
target_include_directories(test_midi_source PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_midi_source PRIVATE Qt6::Core Qt6::Test)
add_test(NAME midi_source COMMAND test_midi_source)

add_executable(test_pattern
  test_pattern.cpp
  ${CMAKE_SOURCE_DIR}/src/rhythm/pattern.cpp
)
target_include_directories(test_pattern PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_pattern PRIVATE Qt6::Core Qt6::Test PkgConfig::mlt++)
add_test(NAME pattern COMMAND test_pattern)

add_executable(test_rhythm_source
  test_rhythm_source.cpp
  ${CMAKE_SOURCE_DIR}/src/rhythm/beatgrid.cpp
  ${CMAKE_SOURCE_DIR}/src/rhythm/midisource.cpp
  ${CMAKE_SOURCE_DIR}/src/rhythm/rhythmsource.cpp
)
target_include_directories(test_rhythm_source PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_rhythm_source PRIVATE Qt6::Core Qt6::Test)
add_test(NAME rhythm_source COMMAND test_rhythm_source)

add_executable(test_note_map
  test_note_map.cpp
  ${CMAKE_SOURCE_DIR}/src/rhythm/notemap.cpp
)
target_include_directories(test_note_map PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_note_map PRIVATE Qt6::Core Qt6::Gui Qt6::Test)
add_test(NAME note_map COMMAND test_note_map)

add_executable(test_clip_placer
  test_clip_placer.cpp
  ${CMAKE_SOURCE_DIR}/src/rhythm/clipplacer.cpp
  ${CMAKE_SOURCE_DIR}/src/rhythm/midisource.cpp
  ${CMAKE_SOURCE_DIR}/src/rhythm/notemap.cpp
)
target_include_directories(test_clip_placer PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_clip_placer PRIVATE Qt6::Core Qt6::Gui Qt6::Test)
add_test(NAME clip_placer COMMAND test_clip_placer)

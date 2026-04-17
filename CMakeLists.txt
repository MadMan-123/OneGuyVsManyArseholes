cmake_minimum_required(VERSION 3.15)
project(Game C CXX)
get_filename_component(DRUID_PROJECT_NAME "${CMAKE_SOURCE_DIR}" NAME)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

set(DEPS_DIR "${CMAKE_SOURCE_DIR}/deps")

include_directories(
    ${DEPS_DIR}/include
    ${DEPS_DIR}/include/GL
    ${DEPS_DIR}/include/SDL3
)

file(GLOB GAME_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/src/*.c
)
list(FILTER GAME_SOURCES EXCLUDE REGEX "main\\.cpp$")

add_library(game SHARED ${GAME_SOURCES})
target_compile_definitions(game PRIVATE GAME_EXPORT DRUID_SYSTEM_EXPORT)

if(MSVC)
    target_link_libraries(game PRIVATE
        ${DEPS_DIR}/lib/druid.lib
        ${DEPS_DIR}/lib/glew32.lib
        ${DEPS_DIR}/lib/libSDL3.dll.a
        opengl32 gdi32 user32 winmm
    )
else()
    target_link_libraries(game PRIVATE
        ${DEPS_DIR}/lib/libdruid.dll.a
        ${DEPS_DIR}/lib/libglew.dll.a
        ${DEPS_DIR}/lib/libSDL3.dll.a
        opengl32 gdi32 user32 winmm
    )
endif()

add_executable(game_standalone ${CMAKE_SOURCE_DIR}/src/main.cpp)
target_link_libraries(game_standalone PRIVATE
    game
    ${DEPS_DIR}/lib/libdruid.dll.a
    ${DEPS_DIR}/lib/libglew.dll.a
    ${DEPS_DIR}/lib/libSDL3.dll.a
    opengl32 gdi32 user32 winmm
)
target_compile_definitions(game_standalone PRIVATE DRUID_APP_TITLE=\"${DRUID_PROJECT_NAME}\")
set_target_properties(game_standalone PROPERTIES OUTPUT_NAME "${DRUID_PROJECT_NAME}")

add_custom_command(TARGET game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${DEPS_DIR}/bin/libdruid.dll       $<TARGET_FILE_DIR:game>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${DEPS_DIR}/bin/glew32.dll         $<TARGET_FILE_DIR:game>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${DEPS_DIR}/bin/SDL3.dll           $<TARGET_FILE_DIR:game>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${DEPS_DIR}/bin/libassimp-6.dll    $<TARGET_FILE_DIR:game>
)

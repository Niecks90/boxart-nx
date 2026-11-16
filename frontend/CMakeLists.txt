cmake_minimum_required(VERSION 3.13)

set(PLATFORM_SWITCH ON CACHE BOOL "" FORCE)
set(USE_DEKO3D OFF CACHE BOOL "" FORCE)
set(USE_SDL2 ON CACHE BOOL "" FORCE)

set(BOREALIS_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/borealis/library)

include(${BOREALIS_LIBRARY}/cmake/commonOption.cmake)
include(${BOREALIS_LIBRARY}/cmake/toolchain.cmake)

project(boxart-nx C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(VERSION_MAJOR "1")
set(VERSION_MINOR "0")
set(VERSION_ALTER "2")
set(PROJECT_AUTHOR "boxart-nx contributors")
set(PROJECT_TITLEID "010000000000B0A7")
set(PROJECT_ICON ${CMAKE_CURRENT_SOURCE_DIR}/borealis/resources/img/demo_icon.jpg)
set(PROJECT_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/borealis/resources)

add_subdirectory(borealis/library)

set(CUSTOM_PORTLIBS /home/nlacarre/thirdparty/portlibs/switch/opt/devkitpro/portlibs/switch)

set(FRONTEND_SOURCES
    src/main.cpp
    src/inventory.cpp
    src/jpeg_pipeline.cpp
    src/steamgriddb.cpp
    src/game_cell.cpp
    src/main_activity.cpp
    src/cover_select_activity.cpp
    ${BOREALIS_LIBRARY}/lib/platforms/switch/switch_wrapper.c
)

add_executable(boxart-nx ${FRONTEND_SOURCES})

target_include_directories(boxart-nx PRIVATE
    src
    include
    ${BOREALIS_LIBRARY}/include
    ${BOREALIS_LIBRARY}/include/borealis/extern
    ${CUSTOM_PORTLIBS}/include
    ${DEVKITPRO}/portlibs/switch/include
    ${DEVKITPRO}/libnx/include
)

target_link_directories(boxart-nx PRIVATE
    ${CUSTOM_PORTLIBS}/lib
    ${DEVKITPRO}/portlibs/switch/lib
    ${DEVKITPRO}/libnx/lib
)

target_link_libraries(boxart-nx PRIVATE
    borealis
    turbojpeg
    curl
    webp
    z
    SDL2
    EGL
    glapi
    drm_nouveau
    nx
    m
)

# RomFS and NRO generation
add_custom_target(boxart-nx.nro ALL DEPENDS boxart-nx
    COMMAND ${NX_NACPTOOL_EXE} --create "boxart-nx" "${PROJECT_AUTHOR}" "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_ALTER}" boxart-nx.nacp --titleid=${PROJECT_TITLEID}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_RESOURCES} ${CMAKE_CURRENT_BINARY_DIR}/resources
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/resources/font
    COMMAND ${NX_ELF2NRO_EXE} $<TARGET_FILE:boxart-nx> boxart-nx.nro --icon=${PROJECT_ICON} --nacp=boxart-nx.nacp --romfsdir=${CMAKE_CURRENT_BINARY_DIR}/resources
)

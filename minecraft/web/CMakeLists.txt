cmake_minimum_required(VERSION 3.20)
project(minecraft_web LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../handheld)
set(SRC ${ROOT}/src)

execute_process(
  COMMAND python3 -c
"from pathlib import Path
mk = Path(r'${ROOT}/project/android_java/jni/Android.mk').read_text()
start = mk.index('LOCAL_SRC_FILES :=')
end = mk.index('LOCAL_CFLAGS :=', start)
block = mk[start:end].split(':=', 1)[1]
out = []
for raw in block.splitlines():
    s = raw.strip()
    if not s: continue
    if s.endswith('\\\\'): s = s[:-1].strip()
    if not s or not s.startswith('../../../src/'): continue
    if s.endswith('/main.cpp') or s.endswith('/main_android_java.cpp') or s.endswith('/network/RakNetInstance.cpp'):
        continue
    out.append(str(Path(r'${SRC}') / s.replace('../../../src/', '')))
print(';'.join(out))"
  OUTPUT_VARIABLE FULL_CLIENT_SOURCES_RAW
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(FULL_CLIENT_SOURCES ${FULL_CLIENT_SOURCES_RAW})

list(APPEND FULL_CLIENT_SOURCES
  ${SRC}/client/OptionsFile.cpp
  ${SRC}/client/gui/screens/ArmorScreen.cpp
  ${SRC}/client/gui/screens/crafting/CraftingFilters.cpp
  ${SRC}/client/gui/screens/crafting/StonecutterScreen.cpp
  ${SRC}/client/renderer/entity/FallingTileRenderer.cpp
  ${SRC}/raknet/BitStream.cpp
  ${SRC}/raknet/RakNetTypes.cpp
  ${SRC}/raknet/RakString.cpp
  ${SRC}/raknet/GetTime.cpp
  ${SRC}/world/level/tile/StairTile.cpp
  ${SRC}/world/level/tile/HeavyTile.cpp
  ${SRC}/world/item/ArmorItem.cpp
  ${SRC}/world/item/crafting/ArmorRecipes.cpp
  ${SRC}/raknet/Itoa.cpp
  ${SRC}/raknet/RakMemoryOverride.cpp
  ${SRC}/raknet/SimpleMutex.cpp
)

file(GLOB EXTRA_GUI_COMPONENTS "${SRC}/client/gui/components/*.cpp")
list(APPEND FULL_CLIENT_SOURCES ${EXTRA_GUI_COMPONENTS})

list(REMOVE_DUPLICATES FULL_CLIENT_SOURCES)

add_executable(minecraft
  src/main.cpp src/keyboard.cpp src/network.cpp src/texture.cpp
  ${FULL_CLIENT_SOURCES}
)

target_include_directories(minecraft PRIVATE ${SRC})

target_compile_definitions(minecraft PRIVATE NO_EGL OPENGL_ES PRE_ANDROID23)

target_compile_options(minecraft PRIVATE "SHELL:-sUSE_LIBPNG=1")

set_target_properties(minecraft PROPERTIES SUFFIX ".html")

target_link_options(minecraft PRIVATE
  "SHELL:-sWASM=1"
  "SHELL:-lwebsocket.js"
  "SHELL:-sLEGACY_GL_EMULATION=1"
  "SHELL:-sGL_UNSAFE_OPTS=0"
  "SHELL:-sMIN_WEBGL_VERSION=1"
  "SHELL:-sMAX_WEBGL_VERSION=1"
  "SHELL:-sALLOW_MEMORY_GROWTH=1"
  "SHELL:-sASSERTIONS=1"
  "SHELL:-sNO_EXIT_RUNTIME=1"
  "SHELL:-sUSE_LIBPNG=1"
  "SHELL:-sEXPORTED_FUNCTIONS=['_main']"
  "SHELL:--preload-file ${ROOT}/data@/data"
)
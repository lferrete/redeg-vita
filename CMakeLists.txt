# Resident Evil: Degeneration - PS Vita port
# Port code Copyright (C) 2026 Leandro Ferrete; see AUTHORS.md and LICENSE.

cmake_minimum_required(VERSION 3.14)

if (NOT DEFINED ENV{VITASDK} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  message(FATAL_ERROR "Please define VITASDK env var to point to your SDK path or set CMAKE_TOOLCHAIN_FILE.")
endif ()

set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "Toolchain file")

project(redeg C CXX)

include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "Resident Evil: Degeneration")
set(VITA_TITLEID "REDEGEN01")
set(VITA_VPKNAME "redeg")
set(VITA_VERSION "01.01")
set(VITA_MKSFOEX_FLAGS "-d ATTRIBUTE2=12")

set(DATA_PATH "ux0:data/redeg/" CACHE STRING "Path to data (with trailing /)")
add_definitions(-DDATA_PATH="${DATA_PATH}")

option(DEBUG_SOLOADER "Enable verbose loader/ObjC logs" ON)
if (DEBUG_SOLOADER)
  add_definitions(-DDEBUG_SOLOADER)
endif()

add_definitions(-D_GNU_SOURCE -D__POSIX_VISIBLE=999999)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wl,--allow-multiple-definition -O2 -g -ffast-math -mfloat-abi=softfp -Wno-deprecated -fno-strict-aliasing")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -std=gnu++20 -Wno-write-strings -Wno-psabi")

set(VITAGL_DIR ${CMAKE_SOURCE_DIR}/lib/vitaGL)
set(VITAGL_LIB ${VITAGL_DIR}/libvitaGL.a)
set(VITAGL_MAKE_FLAGS "SOFTFP_ABI=1 NO_DEBUG=1 LOG_ERRORS=1 HAVE_SHADER_CACHE=1 NO_SPLASHSCREEN=1")
separate_arguments(VITAGL_MAKE_ARGS UNIX_COMMAND "${VITAGL_MAKE_FLAGS}")

add_custom_command(
    OUTPUT ${VITAGL_LIB}
    COMMAND ${CMAKE_MAKE_PROGRAM} ${VITAGL_MAKE_ARGS}
    WORKING_DIRECTORY ${VITAGL_DIR}
    COMMENT "Building vendorized vitaGL..."
)
add_custom_target(vitaGL_build DEPENDS ${VITAGL_LIB})

add_executable(${CMAKE_PROJECT_NAME}
    source/main.c
    source/port_author.h
    source/macho/macho.c
    source/objc/objc_runtime.c
    source/objc/objc_foundation.c
    source/objc/objc_uikit.c
    source/objc/objc_egl.c
    source/shims/libc.c
    source/shims/cf.c
    source/shims/audio.c
    source/shims/misc.c
    source/shims/symbols.c
    source/shims/gl_syms.c
    source/utils/logger.c
    source/utils/glutil.c
)

add_dependencies(${CMAKE_PROJECT_NAME} vitaGL_build)

target_include_directories(${CMAKE_PROJECT_NAME} SYSTEM PUBLIC ${CMAKE_SOURCE_DIR}/lib ${VITAGL_DIR}/source)
target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/source)

target_link_libraries(${CMAKE_PROJECT_NAME}
    -Wl,--whole-archive pthread -Wl,--no-whole-archive
    -Wl,--start-group ${VITAGL_LIB} stdc++ -Wl,--end-group
    vitashark
    SceShaccCgExt
    taihen_stub
    SceShaccCg_stub
    z
    m
    ScePgf_stub
    mathneon
    kubridge_stub
    SceHid_stub
    SceTouch_stub
    SceMotion_stub
    SceAppMgr_stub
    SceAppUtil_stub
    SceAudio_stub
    SceAudioIn_stub
    SceCommonDialog_stub
    SceDisplay_stub
    SceKernelDmacMgr_stub
    SceFios2_stub
    SceCtrl_stub
    SceGxm_stub
    ScePower_stub
    SceVshBridge_stub
)

vita_create_self(eboot.bin ${CMAKE_PROJECT_NAME} UNSAFE NOASLR)

add_custom_target(vpk ALL
    COMMAND powershell -NoProfile -ExecutionPolicy Bypass -File
            "${CMAKE_SOURCE_DIR}/tools/make_vpk.ps1"
            -BuildDir "${CMAKE_BINARY_DIR}"
            -OutFile "${CMAKE_BINARY_DIR}/${VITA_VPKNAME}.vpk"
    DEPENDS eboot.bin-self
    COMMENT "Packing ${VITA_VPKNAME}.vpk"
)

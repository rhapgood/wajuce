# wajuce — Native library build
# Consumed by Flutter's plugin system for Android, Windows, Linux.
# macOS/iOS use CocoaPods which invokes this transitively.
#
# Build modes:
#   Default: links to the iPlug2-backed native WebAudio engine
#   WAJUCE_STUB_ONLY=ON: uses the C stub for analysis/testing

cmake_minimum_required(VERSION 3.22)

project(wajuce_library VERSION 0.1.0 LANGUAGES C CXX)

option(WAJUCE_STUB_ONLY "Build with C stubs only (no native audio runtime)" OFF)

if(WAJUCE_STUB_ONLY)
    # ---- Stub build ----
    add_library(wajuce SHARED
        "wajuce.c"
    )
    target_compile_definitions(wajuce PUBLIC DART_SHARED_LIB)
    set_target_properties(wajuce PROPERTIES
        PUBLIC_HEADER wajuce.h
        OUTPUT_NAME "wajuce"
    )
else()
    # ---- Full native build ----
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../native/engine
                     ${CMAKE_CURRENT_BINARY_DIR}/engine)

    # WAIPlugEngine provides all extern "C" symbols.
    # We create a shared library that re-exports those via linking.
    add_library(wajuce SHARED
        "wajuce.h"  # header-only source entry; symbols come from WAIPlugEngine
    )
    if(APPLE)
        target_link_libraries(wajuce PRIVATE WAIPlugEngine)
        target_link_options(wajuce PRIVATE
            "LINKER:-force_load,$<TARGET_FILE:WAIPlugEngine>")
    elseif(MSVC)
        # link.exe won't emit a DLL for a header-only target; give it one object
        # to anchor. The engine's dllexport'd wajuce_* symbols are re-exported
        # via /WHOLEARCHIVE below.
        target_sources(wajuce PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/wajuce_msvc_anchor.cpp")
        target_link_libraries(wajuce PRIVATE WAIPlugEngine)
        target_link_options(wajuce PRIVATE
            "/WHOLEARCHIVE:$<TARGET_FILE:WAIPlugEngine>")
    else()
        target_link_libraries(wajuce PRIVATE
            "-Wl,--whole-archive" WAIPlugEngine "-Wl,--no-whole-archive")
    endif()
    set_target_properties(wajuce PROPERTIES
        PUBLIC_HEADER wajuce.h
        OUTPUT_NAME "wajuce"
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        LINKER_LANGUAGE CXX
    )
    target_compile_definitions(wajuce PUBLIC DART_SHARED_LIB)

    # Android 15+ / Google Play require 64-bit native libraries to load on
    # devices with a 16 KB memory page size. Align ELF LOAD segments to 16 KB so
    # libwajuce.so (and everything statically linked into it) is compatible.
    # NDK r28+ does this by default; the explicit flag keeps the build correct
    # under older NDKs too. See https://developer.android.com/guide/practices/page-sizes
    if(ANDROID)
        target_link_options(wajuce PRIVATE "-Wl,-z,max-page-size=16384")
    endif()
endif()

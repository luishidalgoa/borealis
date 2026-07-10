cmake_minimum_required(VERSION 3.6)

project(BOREALIS_DEMO)

# armeabi-v7a requires cpufeatures library
# include(AndroidNdkModules)
# android_ndk_import_module_cpufeatures()

set(SDL_SHARED ON CACHE BOOL "Build a shared SDL3 library" FORCE)
set(SDL_STATIC OFF CACHE BOOL "Build a static SDL3 library" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "Build the SDL3 test library" FORCE)
set(SDL_TESTS OFF CACHE BOOL "Build the SDL3 tests" FORCE)
set(USE_SDL2 OFF CACHE BOOL "Use SDL2" FORCE)
set(USE_SDL3 ON CACHE BOOL "Use SDL3" FORCE)

# SDL3 sources are in a subfolder named "SDL"
add_subdirectory(SDL)

# Compilation of companion libraries
#add_subdirectory(SDL_image)
#add_subdirectory(SDL_mixer)
#add_subdirectory(SDL_ttf)


SET(PLATFORM_ANDROID ON CACHE BOOL "")
SET(LIBROMFS_PREBUILT_GENERATOR "${CMAKE_CURRENT_SOURCE_DIR}/borealis/libromfs-generator" CACHE STRING "")

if (NOT EXISTS "${LIBROMFS_PREBUILT_GENERATOR}")
    message(FATAL_ERROR "libromfs-generator has not been built, please refer to borealis/build_libromfs_generator.sh for more information")
endif()

add_subdirectory(borealis)

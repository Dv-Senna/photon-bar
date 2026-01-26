cmake_minimum_required(VERSION 3.30)

include(FetchContent)

project(photon-bar
	VERSION 0.0.1
	LANGUAGES CXX
)


add_subdirectory(vendors/xdg-shell)
add_subdirectory(vendors/wlr-layer-shell-unstable-v1)
add_subdirectory(vendors/flex)

file(GLOB_RECURSE SOURCE_FILES src/*.cppm)
add_library(photon-bar-modules)
target_sources(photon-bar-modules PUBLIC FILE_SET CXX_MODULES FILES ${SOURCE_FILES})
target_compile_features(photon-bar-modules PUBLIC cxx_std_26)
target_link_libraries(photon-bar-modules PUBLIC
	wayland-client
	xdg-shell::xdg-shell
	wlr-layer-shell-unstable-v1::wlr-layer-shell-unstable-v1
	flex::flex-reflection
	flex::flex-enums
)

add_executable(photon-bar src/main.cpp)
target_compile_features(photon-bar PRIVATE cxx_std_26)
target_link_libraries(photon-bar PRIVATE photon-bar-modules)

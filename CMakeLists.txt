cmake_minimum_required(VERSION 3.30)

include(FetchContent)

project(photon-bar
	VERSION 0.0.1
	LANGUAGES CXX
)


add_subdirectory(vendors/xdg-shell)
add_subdirectory(vendors/wlr-layer-shell-unstable-v1)
add_subdirectory(vendors/flex)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp)
add_executable(photon-bar ${SOURCE_FILES})
target_compile_features(photon-bar PRIVATE cxx_std_26)
target_link_libraries(photon-bar PRIVATE
	wayland-client
	xdg-shell::xdg-shell
	wlr-layer-shell-unstable-v1::wlr-layer-shell-unstable-v1
	flex::flex-reflection
	flex::flex-enums
)
target_include_directories(photon-bar PRIVATE src)

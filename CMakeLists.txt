cmake_minimum_required(VERSION 3.30)

project(photon-bar
	VERSION 0.0.1
	LANGUAGES CXX
)


add_subdirectory(vendors/xdg-shell)
add_subdirectory(vendors/wlr-layer-shell-unstable-v1)
add_subdirectory(vendors/glad)

find_package(OpenGL COMPONENTS EGL REQUIRED)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp)
add_executable(photon-bar ${SOURCE_FILES})
target_compile_features(photon-bar PRIVATE cxx_std_26)
target_link_libraries(photon-bar PRIVATE
	wayland-client
	wayland-egl
	xdg-shell::xdg-shell
	wlr-layer-shell-unstable-v1::wlr-layer-shell-unstable-v1
	OpenGL::EGL
	glad::glad
)
target_include_directories(photon-bar PRIVATE src)

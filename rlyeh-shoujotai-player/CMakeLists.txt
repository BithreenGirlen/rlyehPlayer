cmake_minimum_required (VERSION 3.20)
project ("rlyeh-shoujotai-player")

set(TARGET_NAME "rlyeh-shoujotai-player")

set(DEPS_DIR "${CMAKE_CURRENT_LIST_DIR}/../deps")
set(SPINE_GENERIC_RUNTIME_DIR "${DEPS_DIR}/spine-cpp-4.2")
set(NLOHMANN_JSON_DIR "${DEPS_DIR}/nlohmann")

# Copy setup file
if (NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/setting.txt")
	file(COPY "${DEPS_DIR}/setting.txt" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
endif()

add_executable (
	${TARGET_NAME}
	"dpiAwareness.manifest"
	"main.cpp"
	"main_window.cpp" "main_window.h"
	"d3d11_core.cpp" "d3d11_core.h"
	"d3d11_renderer2d.cpp" "d3d11_renderer2d.h"
	"d2_text_writer.cpp" "d2_text_writer.h"
	"win_dialogue.cpp" "win_dialogue.h"
	"win_filesystem.cpp" "win_filesystem.h"
	"win_text.cpp" "win_text.h"
	"win_clock.cpp" "win_clock.h"
	"win_font.cpp" "win_font.h"
	"mf_media_player.cpp" "mf_media_player.h"
	"rlyeh.cpp" "rlyeh.h"
	"static_string.h"
	"path_utility.h"
	"text_utility.h"
	"${NLOHMANN_JSON_DIR}/json.hpp"
	"native-ui/window_menu.cpp" "native-ui/window_menu.h"
	"native-ui/dialogue_controls.cpp" "native-ui/dialogue_controls.h"
	"native-ui/dialogue_template.cpp" "native-ui/dialogue_template.h"
	"native-ui/font_setting_dialogue.cpp" "native-ui/font_setting_dialogue.h"
	"native-ui/media_setting_dialogue.cpp" "native-ui/media_setting_dialogue.h"
	"native-ui/window_menu.cpp" "native-ui/window_menu.h"
	"d3d11_spine/d3d11_spine_blend_mode.h"
	"d3d11_spine/d3d11_spine_extension.cpp" "d3d11_spine/d3d11_spine_extension.h"
	"d3d11_spine/d3d11_spine_texture_loader.cpp" "d3d11_spine/d3d11_spine_texture_loader.h"
	"d3d11_spine/d3d11_spine_drawable.cpp" "d3d11_spine/d3d11_spine_drawable.h"
	"d3d11_spine/spine_loader.cpp" "d3d11_spine/spine_loader.h"
	"d3d11_spine/spine_player.cpp" "d3d11_spine/spine_player.h"
	"d3d11_spine/spine_file_verifier.cpp" "d3d11_spine/spine_file_verifier.h"
	"d3d11_spine/spine_shared_ptr.h"
	"d3d11_spine/spine_unique_ptr.h"
	"d3d11_spine/d3d11_spine_player.cpp" "d3d11_spine/d3d11_spine_player.h"
)

target_include_directories(
	${TARGET_NAME} PRIVATE
	"${SPINE_GENERIC_RUNTIME_DIR}/include"
)

target_link_libraries(
	${TARGET_NAME} PRIVATE
	"spine-cpp-4.2"
)

target_compile_definitions(
	${TARGET_NAME} PRIVATE
	UNICODE _UNICODE
	SPINE_42
)

set_target_properties(${TARGET_NAME} PROPERTIES WIN32_EXECUTABLE 1)

if(MSVC)
	target_compile_definitions(${TARGET_NAME} PRIVATE -D_CRT_SECURE_NO_WARNINGS)
	target_compile_options(${TARGET_NAME} PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:/d1trimfile:${CMAKE_SOURCE_DIR}\\>")
	target_link_options(${TARGET_NAME} PRIVATE "/pdbaltpath:%_PDB%")
endif()


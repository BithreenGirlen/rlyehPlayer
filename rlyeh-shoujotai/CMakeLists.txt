cmake_minimum_required(VERSION 3.20)
project("rlyeh-shoujotai")

set(TARGET_NAME "rlyeh-shoujotai")

add_executable(
	${TARGET_NAME} 
	"main.cpp"	
	"file_utility.cpp" "file_utility.h"
	"json_minimal.cpp" "json_minimal.h"
	"win_http_session.cpp" "win_http_session.h"
)


target_compile_definitions(${TARGET_NAME} PRIVATE UNICODE _UNICODE)

target_precompile_headers(${TARGET_NAME} PRIVATE
	<string>
	<vector>
	<algorithm>
)

if(MSVC)
	target_compile_definitions(${TARGET_NAME} PRIVATE -D_CRT_SECURE_NO_WARNINGS)
	target_compile_options(${TARGET_NAME} PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:/d1trimfile:${CMAKE_SOURCE_DIR}\\>")
	target_link_options(${TARGET_NAME} PRIVATE "/pdbaltpath:%_PDB%")
endif()

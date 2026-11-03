cmake_minimum_required (VERSION 3.20)
project ("rlyeh-shoujotai-deps")

set(SPINE_VERSION "4.2")
set(SPINE_RUNTIME_LANGUAGE "spine-cpp")
set(SPINE_RUNTIME_DIR ${CMAKE_CURRENT_LIST_DIR}/${SPINE_RUNTIME_LANGUAGE}-${SPINE_VERSION})

set(NLOHMANN_JSON_VERSION "3.12.0")
set(NLOHMANN_JSON_DIR "${CMAKE_CURRENT_LIST_DIR}/nlohmann")

set(TEMP_DIR ${CMAKE_CURRENT_BINARY_DIR}/temp)

function(download_and_extract HOST_URL ARCHIVE_NAME)
	if (NOT EXISTS "${TEMP_DIR}/${ARCHIVE_NAME}.zip")
		file(DOWNLOAD "${HOST_URL}" "${TEMP_DIR}/${ARCHIVE_NAME}.zip" SHOW_PROGRESS)
	endif()

	if (NOT EXISTS "${TEMP_DIR}/${ARCHIVE_NAME}")
		file(ARCHIVE_EXTRACT INPUT "${TEMP_DIR}/${ARCHIVE_NAME}.zip" DESTINATION "${TEMP_DIR}")
	endif()
endfunction()

# Download Spine generic runtime
if (NOT EXISTS "${SPINE_RUNTIME_DIR}/src")
	set (SPINE_RUNTIME_URL "https://github.com/EsotericSoftware/spine-runtimes/archive/refs/heads/${SPINE_VERSION}.zip")
	set (SPINE_FOLDER_NAME "spine-runtimes-${SPINE_VERSION}")
	download_and_extract("${SPINE_RUNTIME_URL}" "${SPINE_FOLDER_NAME}")

	file(COPY "${TEMP_DIR}/${SPINE_FOLDER_NAME}/${SPINE_RUNTIME_LANGUAGE}/${SPINE_RUNTIME_LANGUAGE}/src" DESTINATION "${SPINE_RUNTIME_DIR}")
	file(COPY "${TEMP_DIR}/${SPINE_FOLDER_NAME}/${SPINE_RUNTIME_LANGUAGE}/${SPINE_RUNTIME_LANGUAGE}/include" DESTINATION "${SPINE_RUNTIME_DIR}")
endif()

# Donwload JSON for Modern C++
if (NOT EXISTS "${NLOHMANN_JSON_DIR}")
	set(SINGLE_HEADER_FILENAME "json.hpp")
	set(NLOHMANN_JSON_URL "https://github.com/nlohmann/json/releases/download/v${NLOHMANN_JSON_VERSION}/${SINGLE_HEADER_FILENAME}")
	file(DOWNLOAD "${NLOHMANN_JSON_URL}" "${NLOHMANN_JSON_DIR}/${SINGLE_HEADER_FILENAME}")
endif()

# Delete temporary files
file(REMOVE_RECURSE "${TEMP_DIR}")

# Build Spine generic runtime as static library
add_subdirectory ("${SPINE_RUNTIME_DIR}")

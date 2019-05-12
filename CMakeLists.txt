# Chatsounds Preprocessor CMakeLists
# (c) PotcFdk, 2017 - 2019, BSD license
# Other CMake modules are in ./cmake/ with their respective licenses.

# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of the <organization> nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

cmake_minimum_required (VERSION 2.6)
project (chatsounds-preprocessor)

SET (CMAKE_CXX_STANDARD 17)
SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pthread")
SET (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")
SET (CMAKE_EXE_LINKER_FLAGS "-Wl,--rpath=.,--rpath=lib")

message (STATUS "        ______            ,                            _   __")
message (STATUS "       / ___  |          /|                           | | /  \\\\")
message (STATUS "      / /   | |__   __ _| |_ ___  ___  _   _ _ __   __| |/ /\\ \\\\")
message (STATUS "     / /    | '_ \\ / _` | __/ __|/ _ \\| | | | `_ \\ / _` |\\ \\\\\\ \\\\")
message (STATUS "    ( (     | | | | (_| | |_\\__ \\ (_) | |_| | | | | (_| | \\ \\\\\\//")
message (STATUS "     \\ \\    |_| |_|\\__,_|\\______/\\___/ \\__,_|_| |_|\\__,/ \\ \\ \\\\")
message (STATUS "      \\ \\____________                                   \\ \\/ //")
message (STATUS "       \\____________ \\'\"`-._,-'\"`-._,-'\"`-._,-'\"`-._,-'\"`\\__//")
message (STATUS "         ____  | |__) ) __ ___ _ __  _ __ ___   ___ ___  ___ ___  ___  _ __")
message (STATUS "        (____) |  ___/ '__/ _ \\ '_ \\| '__/ _ \\ / __/ _ \\/ __/ __|/ _ \\| '__|")
message (STATUS "               | |   | | |  __/ |_) | | | (_) | (_(  __/\\__ \\__ \\ (_) | |")
message (STATUS "         _____/ /    | |  \\___| .__/|_|  \\___/ \\___\\________/___/\\___/| |")
message (STATUS "        (______/     | |      | |                            ^potcfdk | |")
message (STATUS "                      \\|      |_|                                     |/")

set (CP_VERSION_MAJOR 0)
set (CP_VERSION_MINOR 0)
set (CP_VERSION_PATCH 0)

set (Boost_NO_BOOST_CMAKE ON) # https://gitlab.kitware.com/cmake/cmake/issues/18865

list (APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

include (GetGitRevisionDescription)

git_describe(CP_GIT_BASE_VERSION --tags --match "v[0-9]\\.[0-9]\\.[0-9]*" --abbrev=0)

message (STATUS "Found closest base version tag: ${CP_GIT_BASE_VERSION}")

STRING (REGEX REPLACE "^v([0-9]+)\\.[0-9]+\\.[0-9]+.*" "\\1" CP_VERSION_MAJOR "${CP_GIT_BASE_VERSION}")
STRING (REGEX REPLACE "^v[0-9]+\\.([0-9]+)\\.[0-9]+.*" "\\1" CP_VERSION_MINOR "${CP_GIT_BASE_VERSION}")
STRING (REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" CP_VERSION_PATCH "${CP_GIT_BASE_VERSION}")
STRING (REGEX REPLACE "^v${CP_VERSION_MAJOR}\\.${CP_VERSION_MINOR}\\.${CP_VERSION_PATCH}" "" CP_VERSION_DISTRIBUTION ${CP_GIT_BASE_VERSION})

message (STATUS "Assembled internal version variables <${CP_VERSION_MAJOR}>.<${CP_VERSION_MINOR}>.<${CP_VERSION_PATCH}>-[${CP_VERSION_DISTRIBUTION}]")

git_describe(CP_GIT_EXT_VERSION --tags --match "v[0-9]\\.[0-9]\\.[0-9]*")

if (NOT "${CP_GIT_BASE_VERSION}" STREQUAL "${CP_GIT_EXT_VERSION}")
	STRING (REGEX REPLACE ".*\\-([^-]+)\\-([^-]+)$" "\\1+\\2" GIT_EXT_SUFFIX ${CP_GIT_EXT_VERSION})
	SET (CP_VERSION_DISTRIBUTION "${CP_VERSION_DISTRIBUTION}+${GIT_EXT_SUFFIX}")

	message (STATUS "Extended internal version variables <${CP_VERSION_MAJOR}>.<${CP_VERSION_MINOR}>.<${CP_VERSION_PATCH}>-[${CP_VERSION_DISTRIBUTION}]")
endif()

if (NOT REL)
	SET (CP_VERSION_DISTRIBUTION "${CP_VERSION_DISTRIBUTION}+unofficial")
	SET (CP_VERSION_OFFICIAL false)
	message (STATUS "Extended internal version variables <${CP_VERSION_MAJOR}>.<${CP_VERSION_MINOR}>.<${CP_VERSION_PATCH}>-[${CP_VERSION_DISTRIBUTION}]")
else()
	SET(CP_VERSION_OFFICIAL true)
endif()

if (NOT "${CP_VERSION_DISTRIBUTION}" STREQUAL "")
	SET (CP_HAS_VERSION_DISTRIBUTION true)
else()
	SET (CP_HAS_VERSION_DISTRIBUTION false)
endif()

include_directories("${PROJECT_SOURCE_DIR}")

configure_file (
	"${PROJECT_SOURCE_DIR}/version.hpp.in"
	"${PROJECT_BINARY_DIR}/version.hpp"
)

configure_file (
	"${PROJECT_SOURCE_DIR}/res/windows-fileinfo.rc.in"
	"${PROJECT_BINARY_DIR}/windows-fileinfo.rc"
)

include_directories("${PROJECT_BINARY_DIR}")

find_package(Boost 1.50.0 REQUIRED COMPONENTS system serialization program_options)
include_directories(${Boost_INCLUDE_DIRS})

if (IS_DIRECTORY "${PROJECT_SOURCE_DIR}/libs/ffmpeg/include")
	if (IS_DIRECTORY "${PROJECT_SOURCE_DIR}/libs/ffmpeg/lib")
		include_directories ("${PROJECT_SOURCE_DIR}/libs/ffmpeg/include")

		# check if static libs exist
		if ((EXISTS "${PROJECT_SOURCE_DIR}/libs/ffmpeg/lib/libavformat.a")
			AND (EXISTS "${PROJECT_SOURCE_DIR}/libs/ffmpeg/lib/libavcodec.a")
			AND (EXISTS "${PROJECT_SOURCE_DIR}/libs/ffmpeg/lib/libavutil.a"))
				message (STATUS "Using local static ffmpeg libs.")
				set (FFMPEG_avformat_LIBRARY "${PROJECT_SOURCE_DIR}/libs/ffmpeg/lib/libavformat.a")
				set (FFMPEG_avcodec_LIBRARY "${PROJECT_SOURCE_DIR}/libs/ffmpeg/lib/libavcodec.a")
				set (FFMPEG_avutil_LIBRARY "${PROJECT_SOURCE_DIR}/libs/ffmpeg/lib/libavutil.a")
		# check if dynamic libs exist
		elseif ((EXISTS "${PROJECT_SOURCE_DIR}/libs/ffmpeg/lib/libavformat.dll.a")
			AND (EXISTS "${PROJECT_SOURCE_DIR}/libs/ffmpeg/lib/libavcodec.dll.a")
			AND (EXISTS "${PROJECT_SOURCE_DIR}/libs/ffmpeg/lib/libavutil.dll.a"))
			message (STATUS "Using local dynamic ffmpeg libs.")
			set (FFMPEG_avformat_LIBRARY "${PROJECT_SOURCE_DIR}/libs/ffmpeg/lib/libavformat.dll.a")
			set (FFMPEG_avcodec_LIBRARY "${PROJECT_SOURCE_DIR}/libs/ffmpeg/lib/libavcodec.dll.a")
			set (FFMPEG_avutil_LIBRARY "${PROJECT_SOURCE_DIR}/libs/ffmpeg/lib/libavutil.dll.a")
		endif ()

		set (FFMPEG_INTERNAL ON)
	endif (IS_DIRECTORY "${PROJECT_SOURCE_DIR}/libs/ffmpeg/lib")
endif (IS_DIRECTORY "${PROJECT_SOURCE_DIR}/libs/ffmpeg/include")

if (NOT FFMPEG_INTERNAL)
	find_package(FFMPEG REQUIRED)

	if (NOT "${FFMPEG_INCLUDE_DIR_AVFORMAT}" STREQUAL "")
		include_directories ("${FFMPEG_INCLUDE_DIR_AVFORMAT}/..")
	endif (NOT "${FFMPEG_INCLUDE_DIR_AVFORMAT}" STREQUAL "")
endif (NOT FFMPEG_INTERNAL)

# https://stackoverflow.com/a/54290906
try_compile(CAN_COMPILE_WITHOUT_STDCPPFS "${CMAKE_BINARY_DIR}/temp"
	"${CMAKE_SOURCE_DIR}/cmake/filesystem.cpp"
	CMAKE_FLAGS CMAKE_CXX_FLAGS)

IF (NOT CAN_COMPILE_WITHOUT_STDCPPFS)
	find_package(CPPexperimental)

	if (NOT CPPEXPERIMENTAL_FOUND)
		SET(CPPEXPERIMENTAL_LIBRARIES "stdc++fs")
	endif()
ENDIF()

add_executable(chatsounds-preprocessor main.cpp "${PROJECT_BINARY_DIR}/windows-fileinfo.rc")
target_link_libraries(chatsounds-preprocessor ${CPPEXPERIMENTAL_LIBRARIES} ${Boost_LIBRARIES} ${FFMPEG_avformat_LIBRARY} ${FFMPEG_avcodec_LIBRARY} ${FFMPEG_avutil_LIBRARY} ${BCRYPT_LIBRARY})

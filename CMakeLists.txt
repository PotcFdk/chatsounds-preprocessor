# Chatsounds Preprocessor CMakeLists
# (c) PotcFdk, 2017, BSD license
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

configure_file (
	"${PROJECT_SOURCE_DIR}/version.h.in"
	"${PROJECT_BINARY_DIR}/version.h"
)

include_directories("${PROJECT_BINARY_DIR}")

find_package(Boost 1.50.0 REQUIRED COMPONENTS system filesystem serialization)
find_package(FFMPEG REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})

if (NOT "${FFMPEG_INCLUDE_DIR_AVFORMAT}" STREQUAL "")
	include_directories (${FFMPEG_INCLUDE_DIR_AVFORMAT})
endif()

add_executable(chatsounds-preprocessor main.cpp)

target_link_libraries(chatsounds-preprocessor ${Boost_LIBRARIES} ${FFMPEG_avformat_LIBRARY} ${FFMPEG_avcodec_LIBRARY} ${FFMPEG_avutil_LIBRARY})

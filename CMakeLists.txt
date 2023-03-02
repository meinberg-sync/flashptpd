#
#  Copyright (C) 2023 Thomas Behn <thomas.behn@meinberg.de>
#
#  =============================================================================
#
#  Permission is hereby granted, free of charge, to any person obtaining
#  a copy of this software and associated documentation files (the “Software”),
#  to deal in the Software without restriction, including without limitation
#  the rights to use, copy, modify, merge, publish, distribute, sublicense,
#  and/or sell copies of the Software, and to permit persons to whom the Software
#  is furnished to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice shall be included
#  in all copies or substantial portions of the Software.
#
#  THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS
#  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
#  IN THE SOFTWARE.
#
#  =============================================================================

cmake_minimum_required (VERSION 3.0)

project (flashptp LANGUAGES C CXX)

set (FLASH_PTP_NAME		 		"flashPTP")
set (FLASH_PTP_DAEMON    		"flashptpd")
set (FLASH_PTP_MAJOR   	 		1)
set (FLASH_PTP_MINOR        	0)
set (FLASH_PTP_PATCH        	0)
set (FLASH_PTP_EXTRA        	"")

set (FLASH_PTP_VERSION      	"${FLASH_PTP_MAJOR}.${FLASH_PTP_MINOR}.${FLASH_PTP_PATCH}")
if (NOT "${FLASH_PTP_EXTRA}" STREQUAL "")
    set (FLASH_PTP_VERSION  	"${FLASH_PTP_VERSION}-${FLASH_PTP_EXTRA}")
endif ()

set (STD_LIBRARIES				"")

if (NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE "debug")
else ()
	string (TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE)
endif ()

set (CMAKE_CXX_STANDARD 		17)
set (CMAKE_C_STANDARD 			99)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "debug")
    add_definitions (-D_DEBUG)
endif ()

set (JSON_BuildTests OFF CACHE INTERNAL "")
add_subdirectory (json)

set (FLASH_PTP_INC_PATH 		"${CMAKE_CURRENT_SOURCE_DIR}/include")
set (FLASH_PTP_SRC_PATH 		"${CMAKE_CURRENT_SOURCE_DIR}/src")
				
include_directories (
	${CMAKE_CURRENT_SOURCE_DIR}
	${FLASH_PTP_INC_PATH}
)

add_library(
	${PROJECT_NAME}
	${CMAKE_CURRENT_SOURCE_DIR}/cppLog/cppLog.cpp
	${FLASH_PTP_SRC_PATH}/flashptp/adjustment/adjustment.cpp
	${FLASH_PTP_SRC_PATH}/flashptp/adjustment/adjtimex.cpp
	${FLASH_PTP_SRC_PATH}/flashptp/adjustment/pidController.cpp
	${FLASH_PTP_SRC_PATH}/flashptp/calculation/arithmeticMean.cpp
	${FLASH_PTP_SRC_PATH}/flashptp/calculation/calculation.cpp
	${FLASH_PTP_SRC_PATH}/flashptp/client/clientMode.cpp
	${FLASH_PTP_SRC_PATH}/flashptp/client/server.cpp
	${FLASH_PTP_SRC_PATH}/flashptp/common/thread.cpp
	${FLASH_PTP_SRC_PATH}/flashptp/filter/filter.cpp
	${FLASH_PTP_SRC_PATH}/flashptp/filter/luckyPacket.cpp
	${FLASH_PTP_SRC_PATH}/flashptp/filter/medianOffset.cpp
	${FLASH_PTP_SRC_PATH}/flashptp/network/interface.cpp
	${FLASH_PTP_SRC_PATH}/flashptp/network/inventory.cpp
	${FLASH_PTP_SRC_PATH}/flashptp/network/network.cpp
	${FLASH_PTP_SRC_PATH}/flashptp/network/phc.cpp
	${FLASH_PTP_SRC_PATH}/flashptp/network/socket.cpp
	${FLASH_PTP_SRC_PATH}/flashptp/selection/bmca.cpp
	${FLASH_PTP_SRC_PATH}/flashptp/selection/stdDev.cpp
	${FLASH_PTP_SRC_PATH}/flashptp/selection/selection.cpp
	${FLASH_PTP_SRC_PATH}/flashptp/server/listener.cpp
	${FLASH_PTP_SRC_PATH}/flashptp/server/serverMode.cpp
	${FLASH_PTP_SRC_PATH}/flashptp/flashptp.cpp
)

target_compile_definitions (
	${PROJECT_NAME}
	PRIVATE
	FLASH_PTP_NAME="${FLASH_PTP_NAME}"
    FLASH_PTP_VERSION="${FLASH_PTP_VERSION}"
)

target_link_libraries (
	${PROJECT_NAME}
	PRIVATE
	${STD_LIBRARIES}
	pthread
	nlohmann_json::nlohmann_json
)

project (flashptpd LANGUAGES C CXX)

include_directories ("${CMAKE_CURRENT_SOURCE_DIR}/json/include")

add_executable (
    ${PROJECT_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/flashptpd/flashptpd.cpp
)

target_compile_definitions (
	${PROJECT_NAME}
	PRIVATE
	FLASH_PTP_NAME="${FLASH_PTP_NAME}"
	FLASH_PTP_DAEMON="${FLASH_PTP_DAEMON}"
    FLASH_PTP_VERSION="${FLASH_PTP_VERSION}"
)

target_link_libraries (
    ${PROJECT_NAME}
    flashptp
)

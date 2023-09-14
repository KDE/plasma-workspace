# SPDX-FileCopyrightText: 2023 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

include_directories(${CMAKE_CURRENT_BINARY_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_executable(fetchinitialplayertest fetchinitialplayertest.cpp)
target_link_libraries(fetchinitialplayertest
    kmpris
    Qt::Test
)
add_test(NAME fetchinitialplayertest
    COMMAND fetchinitialplayertest
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

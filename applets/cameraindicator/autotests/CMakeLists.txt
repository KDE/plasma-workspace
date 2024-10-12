# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause
if(NOT TARGET PkgConfig::PipeWire)
    return()
endif()

add_executable(pipewiremockcamera videosource.cpp)
target_link_libraries(pipewiremockcamera PkgConfig::PipeWire)

add_executable(pipewirecameraplay videoplay.cpp)
target_link_libraries(pipewirecameraplay PkgConfig::PipeWire)

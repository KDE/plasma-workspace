# SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
# SPDX-License-Identifier: BSD-3-Clause

set(plasma_engine_clipboard_SRCS clipboardengine.cpp clipboardservice.cpp clipboardjob.cpp)
kcoreaddons_add_plugin(plasma_engine_clipboard SOURCES ${plasma_engine_clipboard_SRCS} INSTALL_NAMESPACE "plasma5support/dataengine")
target_link_libraries(plasma_engine_clipboard
    klipper_common
    Plasma::Plasma5Support
)

install(FILES org.kde.plasma.clipboard.operations DESTINATION ${PLASMA5SUPPORT_DATA_INSTALL_DIR}/services)

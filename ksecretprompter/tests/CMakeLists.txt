# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: Marco Martin <notmart@gmail.com>

set(ksecretpromptertest_SRCS
    main.cpp
    promptertest.cpp
    request.cpp
)

add_executable(ksecretpromptertest)

target_sources(ksecretpromptertest PRIVATE
    ${ksecretpromptertest_SRCS}
)

target_link_libraries(ksecretpromptertest
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::DBus
    KF6::I18n
    KF6::WindowSystem
)

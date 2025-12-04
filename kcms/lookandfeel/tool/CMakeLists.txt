# SPDX-FileCopyrightText: 2025 Vlad Zahorodnii <vlad.zahorodnii@kde.org>
# SPDX-License-Identifier: BSD-3-Clause

add_executable(plasma-apply-lookandfeel lnftool.cpp)

kconfig_target_kcfg_file(plasma-apply-lookandfeel
    FILE ${CMAKE_CURRENT_SOURCE_DIR}/../lookandfeelsettings.kcfg
    CLASS_NAME LookAndFeelSettings
    MUTATORS
    NOTIFIERS
)

target_link_libraries(plasma-apply-lookandfeel PRIVATE
    Qt::Widgets
    KF6::ConfigGui
    KF6::I18n
    PW::KLookAndFeel
)

ADD_CUSTOM_TARGET(plasma-apply-lookandfeel_link ALL
                  COMMAND ${CMAKE_COMMAND} -E create_symlink ${KDE_INSTALL_FULL_BINDIR}/plasma-apply-lookandfeel ${CMAKE_CURRENT_BINARY_DIR}/lookandfeeltool)
install( FILES ${CMAKE_CURRENT_BINARY_DIR}/lookandfeeltool DESTINATION ${KDE_INSTALL_FULL_BINDIR}/ )
install( TARGETS plasma-apply-lookandfeel ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} )

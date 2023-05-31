# SPDX-FileCopyrightText: 2024 Nate Graham <nate@kde.org>
# SPDX-License-Identifier: BSD-3-Clause

add_definitions(-DTRANSLATION_DOMAIN=\"kded_donationmessage\")

include_directories (${CMAKE_CURRENT_BINARY_DIR}/..)

kcoreaddons_add_plugin(donationmessage
    SOURCES donationmessage.cpp donationmessage.h
    INSTALL_NAMESPACE "kf6/kded"
)

target_link_libraries(donationmessage
    KF6::ConfigCore
    KF6::DBusAddons
    KF6::I18n
    KF6::KIOGui
    KF6::Notifications
)

ecm_qt_declare_logging_category(donationmessage
    HEADER donationmessage_debug.h
    IDENTIFIER DONATION_MESSAGE
    CATEGORY_NAME org.kde.donationmessage
    DESCRIPTION "KDED module showing yearly request to donate to KDE"
    EXPORT PLASMAWORKSPACE
)

install(
    FILES donationmessage.notifyrc
    DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR}
)

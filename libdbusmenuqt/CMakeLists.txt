set(libdbusmenu_SRCS
dbusmenuimporter.cpp
dbusmenushortcut_p.cpp
dbusmenutypes_p.cpp
utils.cpp
)

add_library(dbusmenuqt STATIC ${libdbusmenu_SRCS})
target_link_libraries(dbusmenuqt
    Qt5::DBus
    Qt5::Widgets
)


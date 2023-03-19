add_library(clockplugin SHARED alignedtimer.cpp clock.cpp plugin.cpp)
target_link_libraries(clockplugin
    PRIVATE
        Qt::Core
        Qt::Qml
        Qt::DBus
)

if (epoll_FOUND)
  # Link against epoll if it is provided by a library rather than the kernel
  if (epoll_LIBRARIES)
    target_include_directories(clockplugin PUBLIC ${epoll_INCLUDE_DIRS})
    target_link_libraries(clockplugin PRIVATE ${epoll_LIBRARIES})
  endif()
endif()

install(TARGETS clockplugin DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/clock)

install(FILES qmldir DESTINATION ${KDE_INSTALL_QMLDIR}/org/kde/plasma/clock)

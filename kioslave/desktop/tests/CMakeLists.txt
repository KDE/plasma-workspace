add_executable(testdesktop kio_desktop_test.cpp)
target_link_libraries(testdesktop KF5::KIOWidgets KF5::Solid Qt5::Test KF5::KDELibs4Support)

ecm_mark_as_test(testdesktop)
add_test(testdesktop testdesktop)


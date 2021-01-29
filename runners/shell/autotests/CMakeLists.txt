include(ECMAddTests)

ecm_add_test(shellrunnertest.cpp TEST_NAME shellrunnertest  LINK_LIBRARIES Qt::Test KF5::Runner)
target_compile_definitions(shellrunnertest PUBLIC -DPLUGIN_BUILD_DIR="${CMAKE_BINARY_DIR}/bin" -DRUNNER_NAME="shell")
add_dependencies(shellrunnertest krunner_shell) # Test depends on the plugin being build

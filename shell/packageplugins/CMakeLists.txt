function(install_package_structure name source)
    kcoreaddons_add_plugin(${name} SOURCES ${source} INSTALL_NAMESPACE "kf6/packagestructure")
    target_link_libraries(${name} KF6::Package)
endfunction()

install_package_structure(plasma_layouttemplate layouttemplate/layouttemplate.cpp)
install_package_structure(plasma_lookandfeel lookandfeel/lookandfeel.cpp)
install_package_structure(wallpaper_images wallpaperimages/wallpaperpackage.cpp)

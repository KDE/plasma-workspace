 
function(install_sensorchart_face name)
    kpackage_install_package(${name} org.kde.ksysguard.${name} sensorapplet ksysguard)
endfunction()

install_sensorchart_face(barchart)
install_sensorchart_face(linechart)
install_sensorchart_face(piechart)
install_sensorchart_face(table)
install_sensorchart_face(textonly)

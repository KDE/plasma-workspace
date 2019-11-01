[Unit]
Description=KDE Session Management Server

[Service]
ExecStart=@CMAKE_INSTALL_FULL_BINDIR@/ksmserver
BusName=org.kde.ksmserver

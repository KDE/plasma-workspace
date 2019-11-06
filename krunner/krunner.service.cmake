[Unit]
Description=KRunner

[Service]
ExecStart=@CMAKE_INSTALL_FULL_BINDIR@/krunner
KillMode=none
BusName=org.kde.krunner

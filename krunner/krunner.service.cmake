[Unit]
Description=KRunner

[Service]
ExecStart=@CMAKE_INSTALL_FULL_BINDIR@/krunner
KillMode=process
BusName=org.kde.krunner

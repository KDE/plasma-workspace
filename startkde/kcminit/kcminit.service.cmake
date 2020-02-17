[Unit]
Description=KDE Config Module Initialization
#kded.service kactivitymanagerd.service

[Service]
ExecStart=@CMAKE_INSTALL_FULL_BINDIR@/kcminit
Restart=no
KillMode=none
Type=forking

[Install]
Alias=plasma-workspace.service

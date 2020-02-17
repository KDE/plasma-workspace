[Unit]
Description=KDE Plasma Workspace
#   Requires=ksmserver.service
#kded.service kactivitymanagerd.service

[Service]
ExecStart=@CMAKE_INSTALL_FULL_BINDIR@/kcminit
Restart=none
KillMode=none
Type=forking

[Install]
Alias=plasma-workspace.service

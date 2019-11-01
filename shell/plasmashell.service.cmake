[Unit]
Description=KDE Plasma Workspace
#   Requires=ksmserver.service
#kded.service kactivitymanagerd.service

[Service]
ExecStart=@CMAKE_INSTALL_FULL_BINDIR@/plasmashell --no-respawn
Restart=on-failure
KillMode=none
BusName=org.kde.plasmashell

[Install]
Alias=plasma-workspace.service

[Unit]
Description=KSplash "ready" Stage
After=plasma-core.target

[Service]
Type=oneshot
ExecStart=@CMAKE_INSTALL_FULL_BINDIR@/qdbus org.kde.KSplash /KSplash org.kde.KSplash.setStage ready

# [Install]
# WantedBy=plasma-workspace.target

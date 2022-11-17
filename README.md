## Plasma Workspace

Plasma Workspace is used as the base for Plasma Desktop, Mobile, and Bigscreen.
It contains shared KCMs, applets as well as multiple libraries.

### TaskManager Library

The Task Manager provides various QAbstractListModel-based model for listing
Windows (TaskManager::AbstractWindowTasksModel), Startup tasks (TaskManager::StartupTasksModel) and Launcher
Job (TaskManager::LauncherTasksModel).

### Workspace Library

libkworkspace provides functions to allow you to interact with the
%KDE session manager (SessionManagement).

### Notification Manager Library

libnotificationmanager is responsible for listing notifications, closing them
and interacting with them in Plasma. This class provides a %Qt model for jobs:
NotificationManager::JobsModel. As well as a %Qt model for notifications and
jobs: NotificationManager::Notifications.

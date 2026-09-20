 ########### install files ###############

 install( FILES
   plasma-development.directory
   plasma-development-translation.directory
   plasma-development-webdevelopment.directory
   plasma-editors.directory
   plasma-education.directory
   plasma-edu-languages.directory
   plasma-edu-mathematics.directory
   plasma-edu-miscellaneous.directory
   plasma-edu-science.directory
   plasma-edu-tools.directory
   plasma-games-arcade.directory
   plasma-games-board.directory
   plasma-games-card.directory
   plasma-games.directory
   plasma-games-kids.directory
   plasma-games-roguelikes.directory
   plasma-games-strategy.directory
   plasma-games-logic.directory
   plasma-graphics.directory
   plasma-help.directory
   plasma-internet.directory
   plasma-internet-terminal.directory
   plasma-main.directory
   plasma-more.directory
   plasma-multimedia.directory
   plasma-network.directory
   plasma-office.directory
   plasma-science.directory
   plasma-settingsmenu.directory
   plasma-system.directory
   plasma-system-terminal.directory
   plasma-toys.directory
   plasma-unknown.directory
   plasma-utilities-accessibility.directory
   plasma-utilities-desktop.directory
   plasma-utilities.directory
   plasma-utilities-file.directory
   plasma-utilities-peripherals.directory
   plasma-utilities-pim.directory
   plasma-utilities-xutils.directory
DESTINATION  ${KDE_INSTALL_DESKTOPDIR} )

install( FILES plasma-applications.menu  DESTINATION  ${KDE_INSTALL_SYSCONFDIR}/xdg/menus )

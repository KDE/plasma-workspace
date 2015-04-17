This is a display manager for SDDM

It installs into ${INSTALL_PREFIX}/sddm/themes
A copy of components is installed into the same path

components is deliberately imported as "components" and not "../components" for this reason

For testing, I recommend symlinking ../components into components
When testing, don't wonder about the white space left of the login promt. This is a simulation of 2 screens,
and the login promt should only appear on primary (right) screen (there is no config dummydata, so the background isn't paintet).

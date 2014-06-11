import QtQuick 2.0

QtObject
{
    function powerOff() {
        console.log("SDDM - POWERING OFF");
    }
    function reboot() {
        console.log("SDDM - REBOOTING");
    }
    function login(user, password, sessionIndex) {
        console.log("SDDM - logging in as ", user, password)
    }
}
import QtQuick 2.0

QtObject
{
    signal loginFailed()
    signal loginSucceeded()

    function powerOff() {
        console.log("SDDM - POWERING OFF");
    }
    function reboot() {
        console.log("SDDM - REBOOTING");
    }
    function suspend() {
        console.log("SDDM - SUSPEND");
    }
    function hibernate() {
        console.log("SDDM - HIBERNATE");
    }
    function hybridSleep() {
        console.log("SDDM - HYBRID SLEEP");
    }


    function login(user, password, sessionIndex) {
        console.log("SDDM - logging in as ", user, password)
        //pretend login failed
        loginFailed()
    }

    property bool canPowerOff: true
    property bool canReboot: true
    property bool canSuspend: true
    property bool canHibernate: true
    property bool canHybridSleep: true
    property string hostname: "MyHostname"

}

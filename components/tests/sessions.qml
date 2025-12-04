import QtQuick
import org.kde.plasma.private.sessions

ListView
{
    width: 500
    height: 500

    model: SessionsModel{}

    delegate: Text {
        text: model.name + " " + model.session + " " + model.displayNumber + " VT" +model.vtNumber
    }

}

import QtQuick 2.15
import org.kde.plasma.private.sessions 2.0

ListView
{
    width: 500
    height: 500

    model: SessionsModel{}

    delegate: Text {
        text: model.name + " " + model.session + " " + model.displayNumber + " VT" +model.vtNumber
    }

}

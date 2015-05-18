import QtQuick 2.0
import QtQuick 2.4

Rectangle {
    id: configureview
    state: "access: insert password"

    Rectangle {
        id: access_view
        anchors.fill: parent
        color: parent.color

        Rectangle
        {
            id: access_view_text
            width: (parent.width - parent.width/5)
            height: (parent.height / 10)
            color: parent.color

            anchors.centerIn: parent
            anchors.verticalCenterOffset: -0.7 * (parent.height / 10);

            Text
            {   property string module_str: dctp_iface.module == "server" ? "серверу" : "клиенту"
                id: access_view_text_entry
                color: "yellow"
                text: "Введите пароль для доступа к " + module_str;
                anchors.centerIn: parent
            }
        }

        TextInputField
        {
            id: access_view_input_field

            width: (parent.width - parent.width/5)
            height: (parent.height / 10)

            anchors.centerIn: parent
            anchors.verticalCenterOffset: 0.7 * (parent.height / 10);

            bkcolor: "blue"
            textcolor: "yellow"

            onReturnPressed:
            {
                dctp_iface.password = text;
            }
        }

    }

    states: [
        State {
            name: "access: insert password"
            PropertyChanges { target: access_view; visible: true; }
        }
    ]


}


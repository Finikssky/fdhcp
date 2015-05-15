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

        Rectangle
        {
            id: access_view_password
            color: "blue"

            width: (parent.width - parent.width/5)
            height: (parent.height / 10)

            anchors.centerIn: parent;
            anchors.verticalCenterOffset: 0.7 * (parent.height / 10);

            TextInput
            {
                id: access_view_password_textinput
                anchors.verticalCenter: parent.verticalCenter

                color: "yellow"
                enabled: true;
                maximumLength: 64;
                echoMode: TextInput.Password
                passwordCharacter: "*"
                autoScroll: true;

                onTextChanged:
                {
                    ensureVisible(1)
                }

                onAccepted:
                {
                    dctp_iface.password = text;
                }

            }

            border
            {
                color: "red"
                width: 1
            }

            MouseArea
            {
                anchors.fill: parent
                onClicked: access_view_password_textinput.focus = true
            }
        }
    }

    states: [
        State {
            name: "access: insert password"
            PropertyChanges { target: access_view; visible: true; }
            PropertyChanges { target: access_view_password_textinput; focus: true; }
            PropertyChanges { target: access_view_password_textinput; text: ""; }
        }
    ]


}


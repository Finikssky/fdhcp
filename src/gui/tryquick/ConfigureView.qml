import QtQuick 2.0
import QtQuick 2.4

Rectangle {
    id: configureview
    state: "access: insert password"

    Rectangle
    {
        id: access_view_insert_password
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
                configureview.state = "access: verify password";
            }
        }

    }

    Rectangle
    {
        id: access_view_verify_password
        anchors.fill: parent
        color: parent.color

        Rectangle
        {
            anchors.centerIn: parent
            color: parent.color
            width: (parent.width - parent.width/5)
            height: (parent.height / 10)

            Text {
                id: access_view_verify_password_text
                anchors.centerIn: parent
                color: "yellow";
                text: "Try access";
            }
        }
    }

    states: [
        State {
            name: "access: insert password"
            PropertyChanges { target: access_view_insert_password; visible: true; }
            PropertyChanges { target: access_view_verify_password; visible: false; }
        },
        State {
            name: "access: verify password"
            PropertyChanges { target: access_view_insert_password; visible: false; }
            PropertyChanges { target: access_view_verify_password; visible: true; }
            StateChangeScript {
                script:
                {
                    dctp_iface.doInThread("tryAccess");
                    //loop_loading.running = true;
                }
            }
        },
        State {
            name: "access: verify end"
            when: dctp_iface.access_status != 0
            PropertyChanges { target: access_view_insert_password; visible: false; }
            PropertyChanges { target: access_view_verify_password; visible: true;  }
            StateChangeScript {
                script:
                {
                    if (dctp_iface.access_status == 1)
                    {
                        access_view_verify_password_text.text = "ACCESS GRANTED";
                        access_view_verify_password_text.color = "lime";
                        access_view_verify_password_text.font.bold = true;
                    }
                    if (dctp_iface.access_status == -1)
                    {
                        access_view_verify_password_text.text = "ACCESS DENIED";
                        access_view_verify_password_text.color = "red";
                        access_view_verify_password_text.font.bold = true;
                    }
                }
            }
        }

    ]


}


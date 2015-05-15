import QtQuick 2.0

Rectangle
{
    id: destview
    state: "choose"

    Rectangle
    {
        id: choise_connect
        anchors.fill: parent
        color: parent.color

        Rectangle
        {
            id: choisetext
            width: (parent.width - parent.width/5)
            height: (parent.height / 10)
            color: parent.color

            anchors.centerIn: parent
            anchors.verticalCenterOffset: -(parent.height / 5);

            Text
            {
                color: "yellow"
                text: "Выберите местонахождение модуля"
                anchors.centerIn: parent
            }
        }

        SButton
        {
            id: local
            text: "local"
            color: "blue"
            textcolor: "yellow"

            anchors.centerIn: parent
            anchors.verticalCenterOffset: -0.7 * (parent.height / 10)

            width: (parent.width - parent.width/5)
            height: (parent.height / 10)

            border {
                 color: "red"
                 width: 1
            }

            onButtonClick:
            {
                dctp_iface.module_ip = "127.0.0.1";
                destview.state = "connecting";
            }
        }

        SButton
        {
            id: remote
            text: "remote"
            color: "blue"
            textcolor: "yellow"

            anchors.centerIn: parent;
            anchors.verticalCenterOffset: 0.7 * (parent.height / 10)

            width: (parent.width - parent.width/5)
            height: (parent.height / 10)

            border {
                 color: "red"
                 width: 1
             }

            onButtonClick:
            {
                remote_ip.visible = true;
                remote_ip_textinput.focus = true;
            }

        }

        Rectangle
        {
            id: remote_ip
            visible: false;
            color: "blue"

            width: (parent.width - parent.width/5)
            height: (parent.height / 10)

            anchors.centerIn: parent;
            anchors.verticalCenterOffset: 0.7 * (parent.height / 10) + remote.height + 10;



            TextInput
            {
                id: remote_ip_textinput
                anchors.centerIn: parent
                readOnly: false;
                enabled: true;
                maximumLength: 16;
                cursorVisible: true;
                color: "yellow"
                font.pixelSize: parent.height * 0.5
                //inputMask: "000.000.000.000"
                validator: RegExpValidator{
                    regExp: /((25[0-5]|2[0-4]\d|[01]?\d\d?)\.){3}(25[0-5]|2[0-4]\d|[01]?\d\d?)/

                }

                onAccepted:
                {
                    dctp_iface.module_ip = text;
                    destview.state = "connecting";
                    remote_ip.visible = false;
                    text = "";
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
                onClicked: remote_ip_textinput.focus = true
            }
        }
    }

    Rectangle
    {
        id: try_connect
        anchors.fill: parent
        color: parent.color

        Rectangle
        {
            anchors.centerIn: parent
            color: parent.color
            width: (parent.width - parent.width/5)
            height: (parent.height / 10)

            Text {
                id: try_connect_text
                anchors.centerIn: parent
                color: "yellow";
                text: qsTr("Connection...")
            }
        }
    }

    states: [
        State {
            name: "choose"
            PropertyChanges { target: choise_connect; visible: true;  }
            PropertyChanges { target: try_connect;    visible: false; }
        },
        State {
            name: "connecting"
            PropertyChanges { target: try_connect;    visible: true;  }
            PropertyChanges { target: choise_connect; visible: false; }
            StateChangeScript {
                script: try_connect_function();
            }
        }
    ]

    Timer {
        property bool cn_status: false;
        id: delay_timer
        interval: 500;
        running: false;
        repeat: false;
        onTriggered: cn_status == false ? destview.state = "choose" : mainwin_fsm.state = "CONFIGUREVIEW";
    }

    function try_connect_function()
    {
        if (-1 == dctp_iface.tryConnect())
        {
           try_connect_text.text = "Sorry, connection failed";
        }
        else
        {
           try_connect_text.text = "Success!";
           delay_timer.cn_status = true;
        }

        delay_timer.running = true;
    }
}


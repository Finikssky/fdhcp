import QtQuick 2.0
import dctpinterface 1.0

Rectangle
{
    id: destview
    state: "choose"

    Rectangle
    {
        id: choise_connect
        anchors.fill: parent
        color: parent.color

        TextField
        {
            id: choisetext
            width: (parent.width - parent.width/5)
            height: (parent.height / 10)
            color: parent.color

            anchors.centerIn: parent
            anchors.verticalCenterOffset: -(parent.height / 5);

            textcolor: "yellow"
            text: "Выберите местонахождение модуля"
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

        TextField
        {
            id: try_connect_text
            anchors.centerIn: parent
            color: parent.color
            width: (parent.width - parent.width/5)
            height: (parent.height / 10)

            textcolor: "yellow";
            text: dctp_iface.conn_status;
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
                script:
                {
                    dctp_iface.conn_status = "Connecting";
                    dctp_iface.doInThread("tryConnect");
                    loop_loading.running = true;
                }
            }
        },
        State {
                name: "connection fail"
                PropertyChanges { target: try_connect;    visible: true;  }
                PropertyChanges { target: choise_connect; visible: false; }
                when: (dctp_iface.conn_status == "Connection failed" || dctp_iface.conn_status == "Successfuly connected") && destview.state == "connecting"
                StateChangeScript {
                    script:
                    {
                        loop_loading.running = false;
                        if (dctp_iface.conn_status == "Connection failed")
                            delay_timer.cn_status = false;
                        else
                            delay_timer.cn_status = true;
                        delay_timer.running = true;
                    }
                }
        }
    ]

    Timer {
        id: loop_loading
        interval: 400
        repeat: true;
        running: false;
        property int dots: 0;

        onTriggered:
        {
            dctp_iface.conn_status = "Connecting";
            for(var i = 0; i < dots; i++) dctp_iface.conn_status += ".";
            dots++;
            dots %= 4;
        }
    }

    Timer {
        property bool cn_status: false;
        id: delay_timer
        interval: 1000;
        running: false;
        repeat: false;
        onTriggered: cn_status == false ? destview.state = "choose" : mainwin_fsm.state = "CONFIGUREVIEW";
    }

}


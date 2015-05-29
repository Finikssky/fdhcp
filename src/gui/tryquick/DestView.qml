import QtQuick 2.0
import dctpinterface 1.0

ScreenView
{
    id: destview
    state: "choose"

    states: [
        State {
            name: "choose"
            PropertyChanges { target: destview; main_entry: choise_connect_block; }
        },
        State {
            name: "connecting"
            PropertyChanges { target: destview; main_entry: try_connect_block; }
        }
    ]

    Component
    {
        id: choise_connect_block

        Rectangle
        {
            id: choise_connect_entry
            color: destview.color

            TextField
            {
                id: choisetext
                width: (parent.width - parent.width/5)
                height: (parent.height / 10)
                color: parent.color

                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: local.top;
                anchors.bottomMargin: 20

                textcolor: "yellow"
                text: {
                    var str = "Выберите местонахождение ";
                    str += (dctp_iface.module == "server") ? "сервера" : "клиента";
                    return str;
                }
            }

            SButton
            {
                id: local
                text: "Локальный"
                color: "transparent"
                textcolor: "yellow"

                anchors.centerIn: parent
                anchors.verticalCenterOffset: -0.6 * height

                width:  0.8 * parent.width
                height: 0.25 * parent.height

                onButtonClick:
                {
                    dctp_iface.module_ip = "127.0.0.1";
                    destview.state = "connecting";
                }
            }

            SButton
            {
                id: remote
                text: "Удаленный"
                color: "transparent"
                textcolor: "yellow"

                anchors.centerIn: parent;
                anchors.verticalCenterOffset: 0.6 * height

                width:  0.8 * parent.width
                height: 0.25 * parent.height

                onButtonClick:
                {
                    remote_ip.visible = true;
                    remote_ip.focus = true;
                }

            }

            TextInputField
            {
                id: remote_ip
                visible: false
                bkcolor: "blue"
                textcolor: "yellow"

                width:  0.8 * parent.width
                height: 0.15 * parent.height

                anchors.centerIn: parent;
                anchors.verticalCenterOffset: 0.6 * height + remote.height + 10;

                maximumLength: 16

                onReturnPressed:
                {
                    dctp_iface.module_ip = text;
                    destview.state = "connecting";
                    remote_ip.visible = false;
                    text = "";
                }

                validator: RegExpValidator { regExp: /((25[0-5]|2[0-4]\d|[01]?\d\d?)\.){3}(25[0-5]|2[0-4]\d|[01]?\d\d?)/; }

                border
                {
                    color: "red"
                    width: 1
                }
            }
        }

    }

    Component
    {
        id: try_connect_block

        Rectangle
        {
            id: try_connect_entry
            color: destview.color

            Image
            {
                id: loader_icon
                height: 0.2 * parent.height
                width: height

                visible: rotate_icon.running
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: try_connect_text.bottom
                anchors.topMargin: height/2

                source: "qrc:/images/rad.png"
            }

            TextField
            {
                id: try_connect_text
                anchors.centerIn: parent
                color: parent.color
                width: 0.8 * parent.height
                height: 0.2 * parent.height

                textcolor: "yellow";
                text: "Попытка соединения";

                Connections
                {
                    target: dctp_iface
                    onConnectSuccess:
                    {
                        rotate_icon.running = false;
                        try_connect_text.text = "Соединение установлено"
                        delay_timer.cn_status = true;
                        delay_timer.running = true;
                    }
                    onConnectFail:
                    {
                        rotate_icon.running = false;
                        try_connect_text.text = "Попытка соединения провалена";
                        delay_timer.cn_status = false;
                        delay_timer.running = true;
                    }
                }

                Connections
                {
                    target: main_view_block.entry
                    onLoaded:
                    {
                        if (main_entry == try_connect_block)
                        {
                            dctp_iface.doInThread("tryConnect");
                            rotate_icon.running = true;
                        }
                    }
                }

                SequentialAnimation
                {
                    id: rotate_icon
                    running: false
                    loops: Animation.Infinite
                    NumberAnimation
                    {
                        target: loader_icon
                        property: "rotation"
                        duration: 1800
                        to: loader_icon.rotation + 360
                    }
                }
            }
        }

    }

    Timer
    {
        property bool cn_status: false;
        id: delay_timer
        interval: 1000;
        running: false;
        repeat: false;
        onTriggered: cn_status == false ? destview.state = "choose" : mainwin_fsm.state = "CONFIGUREVIEW";
    }
}


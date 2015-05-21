import QtQuick 2.0

Rectangle
{
    id: startview
    TextField
    {
        id: choisetext
        width: (parent.width - parent.width/5)
        height: (parent.height / 10)
        color: parent.color

        anchors.centerIn: parent
        anchors.verticalCenterOffset: -(parent.height / 5);

        textcolor: "yellow"
        text: "Выберите модуль для настройки"
    }

    SButton
    {
        id: serverBt
        text: "DHCP сервер"
        color: "blue"

        anchors.centerIn: parent
        anchors.verticalCenterOffset: -0.7 * (parent.height / 10)

        width: (parent.width - parent.width/5)
        height: (parent.height / 10)

        border {
             color: "red"
             width: 1
        }

        onButtonClick: { mainwin_fsm.state = "DESTVIEW"; dctp_iface.module = "server"; }
    }

    SButton
    {
        id: clientBt
        text: "DHCP клиент"
        color: "blue"

        anchors.centerIn: parent;
        anchors.verticalCenterOffset: 0.7 * (parent.height / 10)

        width: (parent.width - parent.width/5)
        height: (parent.height / 10)

        border {
             color: "red"
             width: 1
         }

        onButtonClick: { mainwin_fsm.state = "DESTVIEW"; dctp_iface.module = "client"; }
    }
}


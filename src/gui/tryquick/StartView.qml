import QtQuick 2.0

Rectangle
{
    id: startview

    property bool next;

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
            text: "Выберите модуль для настройки"
            anchors.centerIn: parent
        }
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

        onButtonClick: { startview.next = true; dctp_iface.module = "server"; }
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

        onButtonClick: { startview.next = true; dctp_iface.module = "client"; }
    }
}


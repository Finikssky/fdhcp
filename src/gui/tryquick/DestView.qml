import QtQuick 2.0

Rectangle
{
    property string next;
    id: destview

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

        anchors.centerIn: parent
        anchors.verticalCenterOffset: -0.7 * (parent.height / 10)

        width: (parent.width - parent.width/5)
        height: (parent.height / 10)

        border {
             color: "red"
             width: 1
        }

    }

    SButton
    {
        id: remote
        text: "remote"
        color: "blue"

        anchors.centerIn: parent;
        anchors.verticalCenterOffset: 0.7 * (parent.height / 10)

        width: (parent.width - parent.width/5)
        height: (parent.height / 10)

        border {
             color: "red"
             width: 1
         }

    }
}


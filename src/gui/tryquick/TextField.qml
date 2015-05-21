import QtQuick 2.0

Rectangle
{
    id: textfield
    color: parent.color
    property alias text: _text.text
    property alias font: _text.font
    property alias bkcolor: textfield.color
    property alias textcolor: _text.color
    property alias textHAlign: _text.horizontalAlignment
    property alias textVAlign: _text.verticalAlignment

    Text
    {
        id: _text
        anchors.fill: parent
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter

        text: ""
    }
}


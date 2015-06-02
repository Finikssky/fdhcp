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
    property alias wrapMode: _text.wrapMode

    Text
    {
        id: _text
        anchors.fill: parent
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        anchors.margins: textfield.height / 5;
        wrapMode: Text.NoWrap

        text: ""
    }
}


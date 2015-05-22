import QtQuick 2.0

Rectangle
{
    property alias error_text: _screenview_floor_errblock.text
    property alias main_entry: _screenview_main.entry

    id: _screenview
    anchors.fill: parent
    color: parent.color

    Rectangle
    {
        id: _screenview_main
        height: parent.height * 7/8
        width: parent.width
        color: parent.color

        anchors.top: parent.top

        property Rectangle entry;
    }

    Rectangle
    {
        id: _screenview_floor
        height: parent.height * 1/8
        width: parent.width
        color: parent.color

        anchors.bottom: parent.bottom

        Rectangle
        {
            id: _screenview_floor_back
            height: parent.height
            width: parent.width / 5

            anchors.left: parent.left

        }

       TextField
       {
           id: _screenview_floor_errblock
           height: parent.height
           width: 3 * parent.width / 5

           anchors.horizontalCenter: parent.horizontalCenter
           anchors.margins: 3

           color: parent.color
           textcolor: "yellow"

           text: "Errors here"
        }

        Rectangle
        {
            id: _screenview_floor_main
            height: parent.height
            width: parent.width / 5

            anchors.right: parent.right
        }
    }

}


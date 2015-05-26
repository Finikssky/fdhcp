import QtQuick 2.0

Rectangle
{
    property alias error_text: _screenview_floor_errblock.text
    property alias main_entry: _screenwiew_main_entry.sourceComponent;
    property alias main_view_block: _screenview_main
    signal pressBack();
    signal pressHome();

    id: _screenview
    anchors.fill: parent
    color: parent.color

    Rectangle
    {
        id: _screenview_main
        height: parent.height * 9/10
        width: parent.width
        color: parent.color

        anchors.top: parent.top

        property alias entry: _screenwiew_main_entry

        Loader
        {
            id: _screenwiew_main_entry
            anchors.fill: _screenview_main
            sourceComponent: main_entry
        }

        //border: { color: "red"; width: 5;}
    }

    Rectangle
    {
        id: _screenview_floor
        height: parent.height * 1/10
        width: parent.width
        color: parent.color

        anchors.bottom: parent.bottom

        SButton
        {
            id: _screenview_floor_back
            height: parent.height
            width: parent.width / 5
            color: parent.color

            anchors.left: parent.left

            onButtonClick: pressBack();
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

        SButton
        {
            id: _screenview_floor_main
            height: parent.height
            width: parent.width / 5
            color: parent.color
            anchors.right: parent.right

            onButtonClick: pressHome()
        }
    }

}


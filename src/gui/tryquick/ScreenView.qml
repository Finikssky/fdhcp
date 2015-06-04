import QtQuick 2.0

Rectangle
{
    property alias error_text: _screenview_floor_errblock.text
    property alias main_entry: _screenwiew_main_entry.sourceComponent;
    property alias main_view_block: _screenview_main
    signal pressBack();
    signal pressHome();
    property alias have_back: _screenview_floor_back.visible
    property alias have_home: _screenview_floor_main.visible

    id: _screenview
    anchors.fill: parent
    color: parent.color

    Image { source: "qrc:/images/viewbackground1.jpg"; z: -1; }
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
    }

    Rectangle
    {
        id: _screenview_floor
        height: parent.height * 1/10
        width: parent.width
        color: parent.color

        anchors.bottom: parent.bottom
        opacity: 1

        Timer
        {
            id: clear_error
            running: false
            repeat: false
            interval: 5000
            onTriggered: error_text = ""
        }

        SButton
        {
            id: _screenview_floor_back
            height: parent.height
            width:  height
            br_image: ""
            bk_image: "qrc:/images/arrow_down.png"
            rotation: 90

            anchors.left: parent.left
            anchors.leftMargin: height/5

            onButtonClick: pressBack();
        }

       TextField
       {
           id: _screenview_floor_errblock
           height: parent.height

           anchors.left: _screenview_floor_back.right
           anchors.right: _screenview_floor_main.left
           anchors.margins: 3

           color: parent.color
           textcolor: "yellow"
           wrapMode: Text.WordWrap

           text: ""

           onTextChanged: clear_error.start();
        }

        SButton
        {
            id: _screenview_floor_main
            height: parent.height
            width: height

            anchors.right: parent.right
            anchors.rightMargin: height/5

            bk_image: "qrc:/images/home.png"
            br_image: ""

            onButtonClick: pressHome()
        }
    }

}


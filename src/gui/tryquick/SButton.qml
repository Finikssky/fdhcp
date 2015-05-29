import QtQuick 2.0

Rectangle
{
    id: container
    color: "transparent"
    property alias text: buttonText.text
    property alias textcolor: buttonText.color
    property alias mouse: mouse
    signal buttonClick()

    Image
    {
        width: parent.width
        height: parent.height
        source: "qrc:/images/plate.png"
    }

    BorderImage {
        id: name
        source: "qrc:/images/ramka.png"
        anchors.fill: parent
        horizontalTileMode: BorderImage.Stretch
        verticalTileMode: BorderImage.Stretch
        border.left: 1; border.top: 1
        border.right: 1; border.bottom: 1
        smooth: true
    }

    MouseArea
    {
       id: mouse
       anchors.fill: parent
       onClicked:
       {
            buttonClick();
       }
    }

    Text
    {
        id: buttonText
        anchors.centerIn: parent
        color: "yellow"
    }

    states: [
        State {
          name: "Pressed"
          when: mouse.pressed == true
          PropertyChanges { target: container; color: Qt.lighter(container.color); opacity: 1; }
        },
        State {
          name: "Focused"
          when: container.focus == true
          PropertyChanges { target: buttonText; color: "#FFFFFF" }
        }
      ]

}

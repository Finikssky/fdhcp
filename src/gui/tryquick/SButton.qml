import QtQuick 2.0

Rectangle
{
    id: container
    property alias text: buttonText.text
    property alias textcolor: buttonText.color
    signal buttonClick()

    BorderImage {
        id: name
        source: "qrc:/images/ramka.png"
        width: parent.width; height: parent.height
        horizontalTileMode: BorderImage.Stretch
        verticalTileMode: BorderImage.Stretch
        border.left: 2; border.top: 1
        border.right: 2; border.bottom: 1
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

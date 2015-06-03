import QtQuick 2.0

Rectangle
{
    id: container
    color: "transparent"
    property alias text: buttonText.text
    property alias textcolor: buttonText.color
    property alias mouse: mouse
    property alias bk_image: bk_image.source
    property alias br_image: br_image.source
    signal buttonClick()

    Image
    {
        id: bk_image
        width: parent.width
        height: parent.height
        source: "qrc:/images/plate.png"

        RotationAnimation {
            id: rot_anim
            target: bk_image
            duration: 300
            loops: Animation.Infinite
            running: false
            direction: RotationAnimation.Clockwise
            from: 0
            to: 360
        }
    }

    BorderImage {
        id: br_image
        source: "qrc:/images/ramka.png"
        anchors.fill: parent
        horizontalTileMode: BorderImage.Stretch
        verticalTileMode: BorderImage.Stretch
        border.left: 1; border.top: 1
        border.right: 1; border.bottom: 1
        smooth: true
    }


    function rotation_start()
    {
        rot_anim.start();
    }
    function rotation_stop()
    {
        rot_anim.stop();
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

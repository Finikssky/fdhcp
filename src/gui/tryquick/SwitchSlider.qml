import QtQuick 2.0

Rectangle
{
    id: polzunok_corps
    property alias anim_duration: anchanim.duration
    property alias state: polzunok_entry.state

    Rectangle
    {
          id: polzunok_entry
          width: parent.width / 2
          height: parent.height
          color: "red"

          anchors.left: parent.left
          state: "off"

          states: [
              State {
                  name: "off"
                  AnchorChanges { target: polzunok_entry; anchors.right: undefined; anchors.left: polzunok_corps.left;  }
              },
              State {
                  name: "on"
                  AnchorChanges { target: polzunok_entry; anchors.left: undefined; anchors.right: polzunok_corps.right;}
              }
          ]

          transitions: [
              Transition {
                  AnchorAnimation { id: anchanim; duration: 300 }
              }
          ]

          MouseArea
          {
              anchors.fill: parent;
              onClicked:
              {
                  switch(parent.state)
                  {
                      case "on": parent.state = "off"; break;
                      case "off": parent.state = "on"; break;
                      default: break;
                  }
              }
          }
    }
}


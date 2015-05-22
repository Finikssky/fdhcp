import QtQuick 2.0

Rectangle
{
    id: polzunok_corps
    property alias anim_duration: anchanim.duration
    property alias switch_state: polzunok_entry.state
    property alias switch_color: polzunok_entry.color
    property alias switch_radius: polzunok_entry.radius
    signal switchOff();
    signal switchOn();
    signal switchClick();

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
                  StateChangeScript {
                      script: switchOff();
                  }
              },
              State {
                  name: "on"
                  AnchorChanges { target: polzunok_entry; anchors.left: undefined; anchors.right: polzunok_corps.right;}
                  StateChangeScript {
                      script: switchOn();
                  }
              }
          ]

          transitions: [
              Transition {
                  AnchorAnimation { id: anchanim; duration: 300 }
              }
          ]

    }

    MouseArea
    {
        anchors.fill: parent;
        onClicked:
        {
            switchChange();
            switchClick();
        }
    }

    function switchChange()
    {
        switch (switch_state)
        {
            case "on": switch_state = "off"; break;
            case "off": switch_state = "on"; break;
            default: break;
        }
    }

}


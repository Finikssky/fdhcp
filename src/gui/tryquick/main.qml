import QtQuick 2.4
import QtQuick.Window 2.2
import dctpinterface 1.0

Window
{
    id: mainwin

    width: 480
    height: 720

    visible: true

    //DCTP interface
    DCTP
    {
        id: dctp_iface

        module: "server"
    }

    //graphic container
    Rectangle {
        id: mainwin_entry
        anchors.fill: parent

        StartView
        {
            id: startview
            color: "black";
            visible: true
            anchors.fill: parent
        }

        DestView
        {
            id: destview
            color: "black";
            visible: false
            anchors.fill: parent
        }

        states: [
            State {
              name: "STARTVIEW"
              when: startview.next == true
              PropertyChanges { target: startview; visible: false;}
              PropertyChanges { target: destview;  visible: true;}
            },
            State {
              name: "DESTVIEW"
              when: container.focus == true
            }
          ]
    }

}

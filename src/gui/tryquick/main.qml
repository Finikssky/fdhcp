import QtQuick 2.4
import QtQuick.Window 2.0
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
        module: "server";
        module_ip: "127.0.0.1"
        last_error: "Error"
    }

    //graphic container
    Rectangle {
        id: mainwin_entry
        anchors.fill: parent

        StartView
        {
            id: startview
            color: "transparent";
            anchors.fill: parent
            opacity: 1
            Connections {
                target: dctp_iface
                onLastErrorChanged: startview.error_text = dctp_iface.last_error
            }
        }

        DestView
        {
            id: destview
            color: "transparent";
            anchors.fill: parent
            onPressBack: mainwin_fsm.state = "STARTVIEW"
            onPressHome: mainwin_fsm.state = "STARTVIEW"

            Connections {
                target: dctp_iface
                onLastErrorChanged: destview.error_text = dctp_iface.last_error
            }
        }

        ConfigureView
        {
            id: configureview
            color: "transparent";
            anchors.fill: parent

            onPressBack: mainwin_fsm.state = "DESTVIEW"
            onPressHome: mainwin_fsm.state = "STARTVIEW"

            error_text: dctp_iface.last_error
            Connections
            {
                target: dctp_iface
                onLastErrorChanged: configureview.error_text = dctp_iface.last_error
            }
        }

    }


    Item {
        id: mainwin_fsm
        state: "STARTVIEW";
        states: [
            State {
              name: "STARTVIEW"
              PropertyChanges { target: startview; visible: true;  }
              PropertyChanges { target: destview;  visible: false; }
              PropertyChanges { target: configureview;  visible: false; }
            },
            State {
              name: "DESTVIEW"
              PropertyChanges { target: destview;  visible: true;  }
              PropertyChanges { target: startview; visible: false; }
              PropertyChanges { target: configureview;  visible: false; }
              PropertyChanges { target: destview;  state: "choose"; }
            },
            State {
              name: "CONFIGUREVIEW"
              PropertyChanges { target: configureview;  visible: true; }
              PropertyChanges { target: destview;  visible: false;  }
              PropertyChanges { target: startview; visible: false; }
              PropertyChanges { target: configureview;  state: "access: insert password" } //"access: insert password"; }
            }
          ]
    }
}

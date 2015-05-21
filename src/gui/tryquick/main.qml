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
        property string conn_status: "";
        property int access_status: 0;
        property int cfgupd_status: 0;

        module: "server";
        module_ip: "127.0.0.1";
        onConnectSuccess: conn_status = "Successfuly connected"
        onConnectFail: conn_status = "Connection failed"
        onAccessGranted: { console.log("AG"); access_status = 1;}
        onAccessDenied: { console.log("AD"); access_status = -1; }
        onConfigUpdateSuccess: { console.log("CUS"); cfgupd_status = 1;}
        onConfigUpdateFail: { console.log("CUF"); cfgupd_status = -1; }
    }

    //graphic container
    Rectangle {
        id: mainwin_entry
        anchors.fill: parent

        StartView
        {
            id: startview
            color: "black";
            anchors.fill: parent
        }

        DestView
        {
            id: destview
            color: "black";
            anchors.fill: parent
        }

        ConfigureView
        {
            id: configureview
            color: "black";
            anchors.fill: parent
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
            },
            State {
              name: "CONFIGUREVIEW"
              PropertyChanges { target: configureview;  visible: true; }
              PropertyChanges { target: destview;  visible: false;  }
              PropertyChanges { target: startview; visible: false; }
            }
          ]
    }

}

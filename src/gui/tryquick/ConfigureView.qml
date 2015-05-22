import QtQuick 2.0
import QtQuick 2.4

Rectangle {
    id: configureview
    state: "access: insert password"

    Rectangle
    {
        id: access_view_insert_password
        anchors.fill: parent
        color: parent.color

        Rectangle
        {
            id: access_view_text
            width: (parent.width - parent.width/5)
            height: (parent.height / 10)
            color: parent.color

            anchors.centerIn: parent
            anchors.verticalCenterOffset: -0.7 * (parent.height / 10);

            Text
            {   property string module_str: dctp_iface.module == "server" ? "серверу" : "клиенту"
                id: access_view_text_entry
                color: "yellow"
                text: "Введите пароль для доступа к " + module_str;
                anchors.centerIn: parent
            }
        }

        TextInputField
        {
            id: access_view_input_field

            width: (parent.width - parent.width/5)
            height: (parent.height / 10)

            anchors.centerIn: parent
            anchors.verticalCenterOffset: 0.7 * (parent.height / 10);

            bkcolor: "blue"
            textcolor: "yellow"

            onReturnPressed:
            {
                dctp_iface.password = text;
                configureview.state = "access: verify password";
            }
        }

    }

    Rectangle
    {
        id: access_view_verify_password
        anchors.fill: parent
        color: parent.color

        Rectangle
        {
            anchors.centerIn: parent
            color: parent.color
            width: (parent.width - parent.width/5)
            height: (parent.height / 10)

            Text {
                id: access_view_verify_password_text
                anchors.centerIn: parent
                color: "yellow";
                text: "Try access";
            }
        }
    }

    Rectangle
    {
        id: configure_view_interfaces_list
        anchors.fill: parent
        color: parent.color

        ListView
        {
            id: configure_view_interfaces_list_listview
            anchors.fill: parent
            orientation: ListView.Vertical
            cacheBuffer: 2000
            snapMode: ListView.SnapOneItem
            highlightRangeMode: ListView.ApplyRange

            delegate: InterfaceConfigBlock
                      {
                        height: (configure_view_interfaces_list.height / 10)
                        width: configure_view_interfaces_list.width
                        name: IfaceName

                        header.bkcolor: configure_view_interfaces_list.color;
                        header.text: "Интерфейс " + name
                        header.textcolor: "yellow"
                        header.anchors.margins: 2

                        border { color: "red"; width: 1 }
                      }

            model: ListModel{
                id: configure_view_interfaces_list_model
            }
        }
    }

    states: [
        State {
            name: "access: insert password"
            PropertyChanges { target: access_view_insert_password; visible: true; }
            PropertyChanges { target: access_view_verify_password; visible: false; }
            PropertyChanges { target: configure_view_interfaces_list; visible: false; }
        },
        State {
            name: "access: verify password"
            PropertyChanges { target: access_view_insert_password; visible: false; }
            PropertyChanges { target: access_view_verify_password; visible: true; }
            PropertyChanges { target: configure_view_interfaces_list; visible: false; }
            StateChangeScript {
                script:
                {
                    dctp_iface.doInThread("tryAccess");
                    //loop_loading.running = true;
                }
            }
        },
        State {
            name: "access: verify end"
            when: dctp_iface.access_status != 0
            PropertyChanges { target: access_view_insert_password; visible: false; }
            PropertyChanges { target: access_view_verify_password; visible: true;  }
            PropertyChanges { target: configure_view_interfaces_list; visible: false; }
            StateChangeScript {
                script:
                {
                    if (dctp_iface.access_status == 1)
                    {
                        access_view_verify_password_text.text = "ACCESS GRANTED";
                        access_view_verify_password_text.color = "lime";
                        access_view_verify_password_text.font.bold = true;
                        action_change_state.nextstate = "configure: update config start";
                        action_change_state.running = true;
                    }
                    if (dctp_iface.access_status == -1)
                    {
                        access_view_verify_password_text.text = "ACCESS DENIED";
                        access_view_verify_password_text.color = "red";
                        access_view_verify_password_text.font.bold = true;
                        action_change_state.nextstate = "access: insert password";
                        action_change_state.running = true;
                    }
                }
            }
        },
        State {
            name: "configure: update config start"
            PropertyChanges { target: access_view_insert_password; visible: false; }
            PropertyChanges { target: access_view_verify_password; visible: true;  }
            PropertyChanges { target: configure_view_interfaces_list; visible: false; }
            StateChangeScript
            {
                script:
                {
                    dctp_iface.access_status = 0;
                    dctp_iface.doInThread("tryUpdateConfig");
                }
            }
        },
        State {
            name: "configure: start"
            when: configureview.state == "configure: update config start" && dctp_iface.cfgupd_status == 1
            PropertyChanges { target: access_view_insert_password; visible: false; }
            PropertyChanges { target: access_view_verify_password; visible: false;  }
            PropertyChanges { target: configure_view_interfaces_list; visible: true; }
            StateChangeScript
            {
                script:
                {
                    dctp_iface.cfgupd_status = 0;
                    var if_str_list = dctp_iface.getIfacesList();
                    for (var i = 0; i < if_str_list.length; i++)
                    {
                        configure_view_interfaces_list_model.append({IfaceName: if_str_list[i]})
                        configure_view_interfaces_list_listview.currentIndex = i;
                        configure_view_interfaces_list_listview.currentItem.interface_state = dctp_iface.getIfaceState(configure_view_interfaces_list_listview.currentItem.name);
                    }
                }
            }
        }

    ]

    Timer {
        property string nextstate;
        id: action_change_state
        interval: 400;
        running: false;
        repeat: false;
        onTriggered: configureview.state = nextstate;
    }

}


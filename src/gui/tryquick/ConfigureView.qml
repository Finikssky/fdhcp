import QtQuick 2.4

ScreenView
{
    id: configureview
    state: "access: insert password"

    states: [
        State {
            name: "access: insert password"
            PropertyChanges { target: configureview; main_entry: access_view_component }
        },
        State {
            name: "access: verify password"
            PropertyChanges { target: configureview; main_entry: access_view_try_access_component }
        },
        State {
            name: "configure: update config start"
            PropertyChanges { target: configureview; main_entry: config_view_try_udpcfg_component }
        },
        State {
            name: "configure: start"
            PropertyChanges { target: configureview; main_entry: config_view_main_component }
        }

    ]

    Component
    {
        id: access_view_component

        Rectangle
        {
            id: access_view_insert_password
            anchors.fill: parent
            color: configureview.color

            TextField
            {
                id: access_view_text
                width: (parent.width - parent.width/5)
                height: (parent.height / 10)
                anchors.centerIn: parent
                anchors.verticalCenterOffset: -0.7 * (parent.height / 10);

                color: parent.color
                textcolor: "yellow"
                text:
                {
                    if (dctp_iface.module == "server")
                        return "Введите пароль для доступа к серверу"
                    else
                        return "Введите пароль для доступа к клиенту"
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

                maximumLength: 20

                onReturnPressed:
                {
                    dctp_iface.password = text;
                    configureview.state = "access: verify password";
                }
            }
        }
    }

    Component
    {
        id: access_view_try_access_component

        TextField
        {
            anchors.centerIn: parent;
            width: (parent.width - parent.width/5)
            height: (parent.height / 10)

            color: configureview.color

            textcolor: "yellow"
            text: "Try access"

            Connections
            {
                target: dctp_iface
                onAccessGranted:
                {
                    text = "ACCESS GRANTED";
                    textcolor = "lime";
                    font.bold = true;
                    action_change_state.nextstate = "configure: update config start"
                    action_change_state.running = true;
                }
                onAccessDenied:
                {
                    text = "ACCESS DENIED";
                    textcolor = "red";
                    font.bold = true;
                    action_change_state.nextstate = "access: insert password"
                    action_change_state.running = true;
                }
            }

            Connections
            {
                target: main_view_block.entry
                onLoaded:
                {
                    dctp_iface.doInThread("tryAccess");
                }
            }
        }
    }

    Component
    {
        id: config_view_try_udpcfg_component
        TextField
        {
            anchors.centerIn: parent
            width: (parent.width - parent.width/5)
            height: (parent.height / 10)
            color: configureview.color

            textcolor: "yellow"
            text: "Идет обновление конфигурации"

            Connections
            {
                target: dctp_iface
                onConfigUpdate:
                {
                    console.log("cfgupdated")
                    console.log(status);
                    if (status == "success") text = "Конфигурация успешно загружена"
                    if (status == "fail")    text = "Конфигурация не загружена"
                    action_change_state.nextstate = "configure: start"
                    action_change_state.running = true;
                }
            }
            Connections
            {
                target: main_view_block.entry
                onLoaded: dctp_iface.doInThread("tryUpdateConfig");
            }
        }
    }

    Component
    {
        id: config_view_main_component

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
                        height: (configure_view_interfaces_list_listview.height / 10)
                        width: configure_view_interfaces_list_listview.width
                        name: IfaceName

                        header.bkcolor: configureview.color;
                        header.text: "Интерфейс " + name
                        header.textcolor: "yellow"
                        header.anchors.margins: 2

                        border { color: "red"; width: 1 }
                      }

            model: ListModel
                   {
                        id: configure_view_interfaces_list_model
                   }

            Connections
            {
                target: main_view_block.entry
                onLoaded:
                {
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
    }


    Timer
    {
        property string nextstate;
        id: action_change_state
        interval: 400;
        running: false;
        repeat: false;
        onTriggered: configureview.state = nextstate;
    }
}

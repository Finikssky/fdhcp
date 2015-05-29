import QtQuick 2.0

Rectangle
{
    id: _subnet
    height: childrenRect.height
    property alias header: _subnet_header
    property alias lview_model: _subnets_list_model
    property alias lview: _subnets_list

    TextField
    {
        id: _subnet_header
        color: parent.color
        textcolor: "yellow"
        text: "Подсети интерфейса:"
        textHAlign: Text.AlignLeft
        anchors.top: parent.top

        SButton
        {
            id: add_button
            width: parent.height * 0.5
            height: width
            color: parent.color

            anchors.right: parent.right
            anchors.rightMargin: parent.width/20
            anchors.verticalCenter: parent.verticalCenter

            text: "+"
            textcolor: "yellow"

            onButtonClick:
            {
                _subnets_list_model.append({SA: "", NM: ""});
            }
        }
    }


    ListView
    {
        id: _subnets_list

        width: parent.width
        height: childrenRect.height

        anchors.top: _subnet_header.bottom
        cacheBuffer: 2000
        orientation: ListView.Vertical
        snapMode: ListView.NoSnap

        delegate: Rectangle
        {
            id: slv_delegate_obj
            color: "lightgreen"
            height: childrenRect.height
            property string subnet_address: SA;
            property string netmask: NM;

            property string old_prefix: SA + " " +  NM;
            property string new_prefix: _subnet_prefix.text;
            property alias prefix: _subnet_prefix.text

            state: "hovered"

            states: [
                State {
                    name: "hovered"
                    PropertyChanges {target: _subnet_hoverblock; sourceComponent: undefined; }
                    //PropertyChanges {target: down_button; text: "+"}
                    PropertyChanges {target: slv_delegate_obj; height: _subnet_header_block.height }
                },
                State {
                    name: "unhovered"
                    PropertyChanges {target: _subnet_hoverblock; sourceComponent: _subnet_hoverblock_component; }
                    //PropertyChanges {target: down_button; text: "-"}
                    PropertyChanges {target: slv_delegate_obj; height: _subnet_header_block.height + _subnet_hoverblock.height }
                }
            ]

            Rectangle
            {
                id: _subnet_header_block
                height: _subnet_header.height/2
                width: _subnet_header.width
                anchors.top: parent.top

                SButton
                {
                    id: _subnet_sd_bt
                    text: old_prefix != new_prefix ? "save" : "del"
                    color: "blue"
                    textcolor: "yellow"
                    height: parent.height * 0.7
                    width: height * 2
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: parent.width/20

                    onButtonClick:
                    {
                        if (text == "save")
                        {
                            if (slv_delegate_obj.old_prefix.length != 0)
                               dctp_iface.tryChangeSubnet(interface_block.name, "del", slv_delegate_obj.old_prefix);
                            dctp_iface.tryChangeSubnet(interface_block.name, "add", slv_delegate_obj.new_prefix);

                            slv_delegate_obj.old_prefix = slv_delegate_obj.new_prefix;
                        }
                        else
                        {
                            if (slv_delegate_obj.new_prefix.length != 0)
                                dctp_iface.tryChangeSubnet(interface_block.name, "del", slv_delegate_obj.new_prefix);
                            _subnets_list_model.remove(slv_delegate_obj);
                        }

                    }
                }

                TextInputField
                {
                    id: _subnet_prefix
                    textcolor: "yellow"
                    color: "grey"
                    text: ""

                    height: parent.height
                    anchors.top: parent.top
                    anchors.topMargin: parent.height/20
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: parent.height/20
                    anchors.left: _subnet_sd_bt.right
                    anchors.leftMargin: parent.width/20
                    anchors.right: _subnet_hover_bt.left
                    anchors.rightMargin: parent.width/20
                    radius: height/5

                    maximumLength: 33

                }

                SButton
                {
                    id: _subnet_hover_bt
                    text: "edit"
                    color: "blue"
                    textcolor: "yellow"
                    height: parent.height * 0.7
                    width: height * 2
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: parent.width/20

                    onButtonClick:
                    {
                        if (slv_delegate_obj.state == "hovered")
                            slv_delegate_obj.state = "unhovered"
                        else
                            slv_delegate_obj.state = "hovered"
                    }
                }
            }

            Loader
            {
                  id: _subnet_hoverblock
                  sourceComponent: undefined
                  height: childrenRect.height
                  anchors.top: _subnet_header_block.bottom
                  //anchors.topMargin: sourceComponent == undefined ? 0 : _subnet_header.height / 5;
            }

            Component
            {
                 id: _subnet_hoverblock_component
                 InterfaceConfigARBlock
                 {
                     color: "red"
                     header_height: _subnet_header_block.height * 2
                     header_width: _subnet_header_block.width
                 }
            }
        }

        model: ListModel
        {
            id: _subnets_list_model
        }
    }
/*
*/
}


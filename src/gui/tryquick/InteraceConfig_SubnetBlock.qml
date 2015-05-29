import QtQuick 2.0
ListView
{
    property real header_height;
    property real header_width;
    property string color: "transparent";

    id: _subnets_list

    width: parent.width
    height: contentHeight

    cacheBuffer: 2000
    snapMode: ListView.NoSnap
    orientation: ListView.Vertical

        header: Component
        {
            TextField
            {
                id: _subnet_header
                height: header_height
                width: header_width

                color: _subnets_list.color
                textcolor: "yellow"
                text: "Подсети интерфейса:"
                textHAlign: Text.AlignLeft

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
                        model.append({SA: "", NM: ""});
                    }
                }
            }
        }

        delegate: Rectangle
        {
            id: slv_delegate_obj
            height: childrenRect.height
            property string old_prefix: SA + " " +  NM;
            property string new_prefix: _subnet_prefix.text;
            property int idx: index
            color: "transparent"

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
                    PropertyChanges {target: slv_delegate_obj; color: "#2f0000cf"; }
                }
            ]

            Rectangle
            {
                id: _subnet_header_block
                height: header_height/2
                width: header_width
                anchors.top: parent.top
                color: parent.color

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
                        console.log("ind:", parent.parent.idx);
                        if (text == "save")
                        {
                            if (slv_delegate_obj.old_prefix.length > 1)
                               dctp_iface.tryChSubnetProperty("del", interface_block.name, slv_delegate_obj.old_prefix, "name", "");
                            dctp_iface.tryChSubnetProperty("add", interface_block.name, slv_delegate_obj.new_prefix, "name", "");

                            slv_delegate_obj.old_prefix = slv_delegate_obj.new_prefix;
                        }
                        else
                        {
                            if (slv_delegate_obj.new_prefix.length > 1)
                                dctp_iface.tryChSubnetProperty("del", interface_block.name, slv_delegate_obj.new_prefix, "name", "");
                            _subnets_list_model.remove(slv_delegate_obj.idx);
                        }

                    }
                }

                TextInputField
                {
                    id: _subnet_prefix
                    textcolor: "yellow"
                    color: "grey"
                    text: SA + " " + NM

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
                    textcolor: "yellow"
                    height: parent.height * 0.7
                    width: height * 2
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right
                    anchors.rightMargin: parent.width/20

                    onButtonClick:
                    {
                        if (_subnet_sd_bt.text == "del" && _subnet_prefix.text.length > 15)
                        {
                            _subnets_list.currentIndex = _subnets_list.indexAt(slv_delegate_obj.x, slv_delegate_obj.y)
                            if (slv_delegate_obj.state == "hovered")
                                slv_delegate_obj.state = "unhovered"
                            else
                                slv_delegate_obj.state = "hovered"
                        }
                    }
                }
            }

            Loader
            {
                  id: _subnet_hoverblock
                  sourceComponent: undefined
                  height: childrenRect.height
                  anchors.top: _subnet_header_block.bottom
            }

            Component
            {
                 id: _subnet_hoverblock_component
                 InterfaceConfigARBlock
                 {
                     color: "#2f0000cf"
                     header_height: _subnet_header_block.height * 2
                     header_width: _subnet_header_block.width
                     subnet_name: _subnet_prefix.text

                     Connections
                     {
                         target: _subnet_hoverblock
                         onLoaded:
                         {
                             if (_subnet_hoverblock.sourceComponent != undefined)
                                update_ars();
                         }
                     }

                     function update_ars()
                     {
                         var ars_str_list = dctp_iface.getSubnetProperty(interface_block.name, _subnet_prefix.text, "range");
                         console.log("ARS: ", ars_str_list);
                         for (var i = 0; i < ars_str_list.length; i++)
                         {
                             var ARS = ars_str_list[i].split('-');
                             model.append({SA: ARS[0], EA: ARS[1] })
                         }
                     }
                 }
            }
        }

        model: ListModel
        {
            id: _subnets_list_model
        }

}



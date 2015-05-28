import QtQuick 2.0

Rectangle
{
    id: _subnet
    property alias header: _subnet_header
    height: _subnet_header.height + _subnets_list.height
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
        anchors.left: parent.left
        anchors.right: parent.right
        cacheBuffer: 2000
        orientation: ListView.Vertical
        snapMode: ListView.NoSnap
        highlightRangeMode: ListView.ApplyRange

        delegate: Rectangle
        {
            id: delegate_obj
            color: "lightgreen"

            height: _subnet_header.height/2
            width: _subnet_header.width

            property string subnet_address: SA;
            property string netmask: NM;

            property string old_prefix: SA + " " +  NM;
            property string new_prefix: _subnet_prefix.text;
            property alias prefix: _subnet_prefix.text

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
                        if (parent.old_prefix.length != 0)
                           dctp_iface.tryChangeSubnet(interface_block.name, "del", parent.old_prefix);
                        dctp_iface.tryChangeSubnet(interface_block.name, "add", parent.new_prefix);

                        parent.old_prefix = parent.new_prefix;
                    }
                    else
                    {
                        if (parent.new_prefix.length != 0)
                            dctp_iface.tryChangeSubnet(interface_block.name, "del", parent.new_prefix);
                        _subnets_list_model.remove(parent);
                    }

                }
            }

            TextInputField
            {
                id: _subnet_prefix
                textcolor: "yellow"
                color: "grey"
                text: ""
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


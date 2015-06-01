import QtQuick 2.0

ListView
{
        id: _dns_servers
        width: parent.width
        height: contentHeight
        orientation: ListView.Vertical
        cacheBuffer: 2000
        snapMode: ListView.NoSnap

        property real header_height;
        property real header_width;
        property string subnet_name: "";
        property string color;

        header: Component
        {
            TextField
            {
                id: _dns_header

                height: header_height
                width:  header_width

                color: _dns_servers.color
                textcolor: "yellow"
                text: "Сервера DNS:"
                textHAlign: Text.AlignLeft

                SButton
                {
                    id: _dns_add_button
                    width: parent.height * 0.5
                    height: width

                    anchors.right: parent.right
                    anchors.rightMargin: parent.width/20
                    anchors.verticalCenter: parent.verticalCenter

                    text: "+"
                    textcolor: "yellow"
                    visible: (_dns_servers.count < 3)

                    onButtonClick:
                    {
                        _dns_servers_model.append({DA: ""});
                    }
                }
            }
        }

        delegate: Rectangle
        {
            id: dns_delegate_obj
            height: header_height / 2
            width: header_width
            color: _dns_servers.color

            property string old_address: DA;
            property string new_address: _dns_address.text;
            property int idx: index

            TextInputField
            {
                id: _dns_address
                bkcolor: "blue"
                textcolor: "yellow"

                width: parent.width / 2.5
                height: parent.height * 9/10
                radius: height/2

                anchors.verticalCenter: parent.verticalCenter

                anchors.left: parent.left;
                anchors.leftMargin: 20;

                textHAlign: Text.AlignHCenter

                text: DA

                maximumLength: 16

                validator: RegExpValidator { regExp: /((25[0-5]|2[0-4]\d|[01]?\d\d?)\.){3}(25[0-5]|2[0-4]\d|[01]?\d\d?)/; }
            }

            SButton
            {
                id: dns_del_address_bt
                color: "blue"
                textcolor: "yellow"
                text: parent.old_address != parent.new_address ? "save" : "del"

                height: parent.height * 0.7
                width: height

                anchors.verticalCenter: parent.verticalCenter

                anchors.right: parent.right;
                anchors.rightMargin: 20;

                onButtonClick:
                {
                    console.log("ind:", dns_delegate_obj.idx);
                    if (text == "save")
                    {
                        if (_dns_address.text.length == 0)
                        {
                            error_text = "Пожалуйста заполните адрес DNS-cервера";
                            return;
                        }

                        if (dns_delegate_obj.old_address.length > 0)
                           dctp_iface.tryChSubnetProperty("del", interface_block.name, _dns_servers.subnet_name, "dns-server", dns_delegate_obj.old_address);

                        if (dns_delegate_obj.new_address.length > 1)
                           dctp_iface.tryChSubnetProperty("add", interface_block.name, _dns_servers.subnet_name, "dns-server", dns_delegate_obj.new_address);

                        dns_delegate_obj.old_address = dns_delegate_obj.new_address;
                    }
                    else
                    {
                        if (dns_delegate_obj.new_address.length > 1)
                            dctp_iface.tryChSubnetProperty("del", interface_block.name,  _dns_servers.subnet_name, "dns-server", dns_delegate_obj.new_address);
                        _dns_servers.model.remove(dns_delegate_obj.idx);
                    }

                }
            }
        }

        model: ListModel
        {
            id: _dns_servers_model;
        }

}


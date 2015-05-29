import QtQuick 2.0

ListView
{
        id: _address_ranges
        width: parent.width
        height: contentHeight
        orientation: ListView.Vertical
        cacheBuffer: 2000
        snapMode: ListView.NoSnap
  //      highlightRangeMode: ListView.ApplyRange

        property real header_height;
        property real header_width;
        property string subnet_name: "";
        property string color;

        header: Component
        {
            TextField
            {
                id: _ar_header

                height: header_height
                width:  header_width

                color: _address_ranges.color
                textcolor: "yellow"
                text: "Диапазоны адресов:"
                textHAlign: Text.AlignLeft

                SButton
                {
                    id: add_button
                    width: parent.height * 0.5
                    height: width

                    anchors.right: parent.right
                    anchors.rightMargin: parent.width/20
                    anchors.verticalCenter: parent.verticalCenter

                    text: "+"
                    textcolor: "yellow"

                    onButtonClick:
                    {
                        _address_ranges_model.append({SA: "", EA: ""});
                    }
                }
            }
        }

        delegate: Rectangle
        {
            id: ar_delegate_obj
            height: header_height / 2
            width: header_width
            color: _address_ranges.color

            property string old_range: SA + "-" + EA;
            property string new_range: _start_address.text + "-" + _end_address.text;
            property int idx: index
            
            TextInputField
            {
                id: _start_address
                bkcolor: "blue"
                textcolor: "yellow"

                width: parent.width / 2.5
                height: parent.height * 9/10
                radius: height/2

                anchors.verticalCenter: parent.verticalCenter

                anchors.left: parent.left;
                anchors.leftMargin: 20;

                textHAlign: Text.AlignHCenter

                text: SA

                maximumLength: 16

                onReturnPressed:
                {
                    _end_address.focus = true
                }

                validator: RegExpValidator { regExp: /((25[0-5]|2[0-4]\d|[01]?\d\d?)\.){3}(25[0-5]|2[0-4]\d|[01]?\d\d?)/; }
            }

            TextField
            {
                anchors.left: _start_address.right
                anchors.right: _end_address.left
                anchors.verticalCenter: parent.verticalCenter
                height: parent.height * 9/10

                textcolor: "yellow"
                text: "-"
            }

            TextInputField
            {
                id: _end_address
                bkcolor: "blue"
                textcolor: "yellow"

                width: parent.width / 2.5
                height: parent.height * 9/10
                radius: height/2

                anchors.verticalCenter: parent.verticalCenter

                anchors.right: del_range_bt.left;
                anchors.rightMargin: 20;

                textHAlign: Text.AlignHCenter

                maximumLength: 16

                text: EA

                onReturnPressed:
                {
                    _end_address.focus = true
                }

                validator: RegExpValidator { regExp: /((25[0-5]|2[0-4]\d|[01]?\d\d?)\.){3}(25[0-5]|2[0-4]\d|[01]?\d\d?)/; }
            }

            SButton
            {
                id: del_range_bt
                color: "blue"
                textcolor: "yellow"
                text: parent.old_range != parent.new_range ? "save" : "del"

                height: parent.height * 0.7
                width: height

                anchors.verticalCenter: parent.verticalCenter

                anchors.right: parent.right;
                anchors.rightMargin: 20;


                onButtonClick:
                {
                    console.log("ind:", ar_delegate_obj.idx);
                    if (text == "save")
                    {
                        if (_start_address.text.length == 0)
                        {
                            error_text = "Пожалуйста заполните стартовый адрес диапазона";
                            return;
                        }

                        if (_end_address.text.length == 0)
                        {
                            error_text = "Пожалуйста заполните конечный адрес диапазона";
                            return;
                        }

                        if (ar_delegate_obj.old_range.length > 1)
                           dctp_iface.tryChSubnetProperty("del", interface_block.name, _address_ranges.subnet_name, "range", ar_delegate_obj.old_range);

                        if (ar_delegate_obj.new_range.length > 1)
                           dctp_iface.tryChSubnetProperty("add", interface_block.name, _address_ranges.subnet_name, "range", ar_delegate_obj.new_range);

                        ar_delegate_obj.old_range = ar_delegate_obj.new_range;
                    }
                    else
                    {
                        if (ar_delegate_obj.new_range.length > 1)
                            dctp_iface.tryChSubnetProperty("del", interface_block.name,  _address_ranges.subnet_name, "range", ar_delegate_obj.new_range);
                        _address_ranges_model.remove(ar_delegate_obj.idx);
                    }

                }
            }
        }

        model: ListModel
        {
            id: _address_ranges_model;
        }

}


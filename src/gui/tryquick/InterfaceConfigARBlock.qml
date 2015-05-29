import QtQuick 2.0

ListView
{
        id: _address_ranges
        width: parent.width
        height: contentHeight
        orientation: ListView.Vertical
        cacheBuffer: 2000
        snapMode: ListView.NoSnap
        highlightRangeMode: ListView.ApplyRange

        property real header_height;
        property real header_width;
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
            id: delegate_obj
            height: _ar_header.height / 2
            width: _ar_header.width
            color: interface_block.color

            property string start_address: SA;
            property string end_address: EA;

            property bool need_apply: false;
            
            TextInputField
            {
                id: _start_address
                bkcolor: "lightblue"
                textcolor: "yellow"

                width: parent.width / 2.5
                height: parent.height * 9/10
                radius: height/2

                anchors.verticalCenter: parent.verticalCenter

                anchors.left: parent.left;
                anchors.leftMargin: 20;

                text: parent.start_address

                maximumLength: 16

                onReturnPressed:
                {
                    _end_address.focus = true
                }

                onTextfieldChanged:
                {
                    delegate_obj.need_apply = (_end_address.text.length != 0 && _start_address.text.length != 0);
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

                maximumLength: 16

                text: parent.end_address

                onReturnPressed:
                {
                    _end_address.focus = true
                }

                onTextfieldChanged:
                {
                    delegate_obj.need_apply = (_end_address.text.length != 0 && _start_address.text.length != 0);
                }

                validator: RegExpValidator { regExp: /((25[0-5]|2[0-4]\d|[01]?\d\d?)\.){3}(25[0-5]|2[0-4]\d|[01]?\d\d?)/; }
            }

            SButton
            {
                id: del_range_bt
                color: "blue"
                textcolor: "yellow"
                text: { delegate_obj.need_apply == true ? "sav" : "del" }

                height: parent.height * 0.7
                width: height

                anchors.verticalCenter: parent.verticalCenter

                anchors.right: parent.right;
                anchors.rightMargin: 20;

                onButtonClick:
                {
                    if (delegate_obj.need_apply)
                    {
                        dctp_iface.Range("add", (_start_address.text + "-" + _end_address.text));
                        delegate_obj.need_apply = false;
                    }
                    else
                    {
                        dctp_iface.Range("del", (_start_address.text + "-" + _end_address.text) )
                        _address_ranges_model.remove(parent);
                    }
                }
            }
        }

        model: ListModel
        {
            id: _address_ranges_model;
        }

}


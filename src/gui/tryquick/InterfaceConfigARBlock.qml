import QtQuick 2.0

Rectangle
{
    id: _contaner
    color: parent.color
    height: _header.height + _address_ranges.height
    property alias header: _header

    TextField
    {
        id: _header
        color: parent.color
        textcolor: "yellow"
        text: "Диапазоны адресов:"
        textHAlign: Text.AlignLeft
        anchors.top: _contaner.top

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
                _address_ranges_model.append({SA: "192.168.1.0", EA: "192.168.1.2"});
            }
        }
    }

    ListView
    {
        id: _address_ranges
        anchors.top: _header.bottom
        orientation: ListView.Vertical
        cacheBuffer: 300
        //snapMode: ListView.SnapToItem
        highlightRangeMode: ListView.ApplyRange

        delegate: Rectangle
        {
            height: _header.height / 2
            width: _header.width
            color: interface_block.color

            property string start_address: SA;
            property string end_address: EA;
            
            TextInputField
            {
                id: _start_address
                bkcolor: "lightblue"
                textcolor: "yellow"

                width: parent.width / 2.5
                height: parent.height

                anchors.left: parent.left;
                anchors.leftMargin: 20;

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
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                textcolor: "yellow"
                text: "-"
            }

            TextInputField
            {
                id: _end_address
                bkcolor: "blue"
                textcolor: "yellow"

                width: parent.width / 2.5
                height: parent.height

                anchors.right: parent.right;
                anchors.rightMargin: 20;

                maximumLength: 16

                onReturnPressed:
                {
                    _end_address.focus = true
                }

                validator: RegExpValidator { regExp: /((25[0-5]|2[0-4]\d|[01]?\d\d?)\.){3}(25[0-5]|2[0-4]\d|[01]?\d\d?)/; }
            }
        }

        model: ListModel {
            id: _address_ranges_model
        }
    }
}


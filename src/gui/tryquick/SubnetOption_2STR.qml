import QtQuick 2.0

ListView
{
        id: _s_opt_2str_list
        width: parent.width
        height: contentHeight
        orientation: ListView.Vertical
        cacheBuffer: 2000
        snapMode: ListView.NoSnap

        property real header_height;
        property real header_width;
        property string header_text: "";
        property string subnet_name: "";
        property string opt_name: "";
        property string opt_1error: "Пожалуйста, заполните строку"
        property string opt_2error: opt_1error;
        property string opt_1type: "WORD";
        property string opt_2type: opt_1type;
        property string divider: "-"
        property int max_elements: 0;
        property int max_1length: 0;
        property int max_2length: max_1length;
        property string color;

        header: Component
        {
            TextField
            {
                id: _s_opt_2str_header

                height: header_height
                width:  header_width

                color: _s_opt_2str_list.color
                textcolor: "yellow"
                text: header_text
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
                    visible: _s_opt_2str_list.count < max_elements;

                    onButtonClick:
                    {
                        _s_opt_2str_list_model.append({S1: "", S2: ""});
                    }
                }
            }
        }

        delegate: Rectangle
        {
            id: _s_opt_2str_delegate
            height: header_height / 2
            width: header_width
            color: _s_opt_2str_list.color

            property string old_string: S1 + divider + S2;
            property string new_string: _s_opt_2str_1.text + divider + _s_opt_2str_2.text;
            property int idx: index

            TextInputField
            {
                id: _s_opt_2str_1
                bkcolor: "blue"
                textcolor: "yellow"

                width: parent.width / 2.5
                height: parent.height * 9/10
                radius: height/2

                anchors.verticalCenter: parent.verticalCenter

                anchors.left: parent.left;
                anchors.leftMargin: 20;

                textHAlign: Text.AlignHCenter

                text: S1

                maximumLength: max_1length

                validator: RegExpValidator { regExp: choise_regular_expressions(opt_1type); }
            }

            TextField
            {
                anchors.left: _s_opt_2str_1.right
                anchors.right: _s_opt_2str_2.left
                anchors.verticalCenter: parent.verticalCenter
                height: parent.height * 9/10

                color: parent.color
                bkcolor: "transparent"
                textcolor: "yellow"
                text: divider
            }

            TextInputField
            {
                id: _s_opt_2str_2
                bkcolor: "blue"
                textcolor: "yellow"

                width: parent.width / 2.5
                height: parent.height * 9/10
                radius: height/2

                anchors.verticalCenter: parent.verticalCenter

                anchors.right: _s_opt_2str_sdel_bt.left;
                anchors.rightMargin: 20;

                textHAlign: Text.AlignHCenter

                maximumLength: 16

                text: S2

                onReturnPressed:
                {
                    _s_opt_2str_2.focus = true
                }

                validator: RegExpValidator { regExp: choise_regular_expressions(opt_2type); }
            }

            SButton
            {
                id: _s_opt_2str_sdel_bt
                color: "blue"
                textcolor: "yellow"
                text: parent.old_string != parent.new_string ? "save" : "del"

                height: parent.height * 0.7
                width: height

                anchors.verticalCenter: parent.verticalCenter

                anchors.right: parent.right;
                anchors.rightMargin: 20;


                onButtonClick:
                {
                    console.log("ind:", _s_opt_2str_delegate.idx);
                    if (text == "save")
                    {
                        if (_s_opt_2str_1.text.length == 0)
                        {
                            error_text = opt_1error;
                            return;
                        }

                        if (_s_opt_2str_2.text.length == 0)
                        {
                            error_text = opt_2error;
                            return;
                        }

                        if (_s_opt_2str_delegate.old_string.length > divider.length)
                           dctp_iface.tryChSubnetProperty("del", interface_block.name, _s_opt_2str_list.subnet_name, _s_opt_2str_list.opt_name, _s_opt_2str_delegate.old_string);

                        if (_s_opt_2str_delegate.new_string.length > divider.length)
                           dctp_iface.tryChSubnetProperty("add", interface_block.name, _s_opt_2str_list.subnet_name, _s_opt_2str_list.opt_name, _s_opt_2str_delegate.new_string);

                        _s_opt_2str_delegate.old_string = _s_opt_2str_delegate.new_string;
                    }
                    else
                    {
                        if (_s_opt_2str_delegate.new_string.length > divider.length)
                            dctp_iface.tryChSubnetProperty("del", interface_block.name,  _s_opt_2str_list.subnet_name, _s_opt_2str_list.opt_name, _s_opt_2str_delegate.new_string);
                        _s_opt_2str_list_model.remove(_s_opt_2str_delegate.idx);
                    }

                }
            }
        }

        model: ListModel
        {
            id: _s_opt_2str_list_model;
        }

        function update_config()
        {
            var str2_list = dctp_iface.getSubnetProperty(interface_block.name, _subnet_prefix.text, opt_name);
            console.log("OPTION ", opt_name, ": ", str2_list);
            for (var i = 0; i < str2_list.length; i++)
            {
                var pair = str2_list[i].split(divider);
                _s_opt_2str_list_model.append({ S1: pair[0], S2: pair[1] })
            }
        }

        function choise_regular_expressions(type)
        {
            var regexp;
            if (type === "IP")
                regexp = /((25[0-5]|2[0-4]\d|[01]?\d\d?)\.){3}(25[0-5]|2[0-4]\d|[01]?\d\d?)/;
            else if (type == "INT")
                regexp = /\d/;
            else
                regexp = undefined;

            return regexp;
        }
}


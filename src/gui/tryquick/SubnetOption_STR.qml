import QtQuick 2.0

ListView
{
        id: _s_opt_str_list
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
        property string opt_error: "Пожалуйста, заполните строку"
        property string opt_type: "WORD";
        property int max_elements: 0;
        property int max_length: 0;
        property string color;


        header: Component
        {
            TextField
            {
                id: _s_opt_str_header

                height: header_height
                width:  header_width

                color: _s_opt_str_list.color
                textcolor: "yellow"
                text: _s_opt_str_list.header_text
                textHAlign: Text.AlignLeft

            }
        }

        footer: Component
        {
            Rectangle
            {
                height: header_height / 2
                width: header_width
                color: _s_opt_str_list.color

                SButton
                {
                    id: _s_opt_str_add_button
                    width: parent.height * 0.8
                    height: width
                    color: "transparent"
                    br_image: ""
                    bk_image: "qrc:/images/delete.png"
                    rotation: 45

                    anchors.centerIn: parent
                    visible: (_s_opt_str_list.count < _s_opt_str_list.max_elements)

                    onButtonClick:
                    {
                        _s_opt_str_list_model.append({STR: ""});
                    }

                }
            }
        }

        delegate: Rectangle
        {
            id: _s_opt_str_delegate
            height: header_height / 2
            width: header_width
            color: _s_opt_str_list.color

            property string old_string: STR;
            property string new_string: _s_opt_str_address.text;
            property int idx: index

            TextInputField
            {
                id: _s_opt_str_address
                bkcolor: "blue"
                textcolor: "yellow"

                width: parent.width / 2.5
                height: parent.height * 9/10
                radius: height/2

                anchors.verticalCenter: parent.verticalCenter

                anchors.left: parent.left;
                anchors.leftMargin: 20;

                textHAlign: Text.AlignHCenter

                text: STR

                maximumLength: _s_opt_str_list.max_length;

                validator: RegExpValidator { id: str_valid;  regExp: choise_regular_expressions(opt_type); }
            }

            SButton
            {
                id: _s_opt_str_sdel_bt
                bk_image: parent.old_string != parent.new_string ? "qrc:/images/save.png" : "qrc:/images/delete.png"
                br_image: ""
                property string mode: parent.old_string != parent.new_string ? "save" : "del"

                height: parent.height * 0.7
                width: height

                anchors.verticalCenter: parent.verticalCenter

                anchors.right: parent.right;
                anchors.rightMargin: 20;

                onButtonClick:
                {
                    console.log("ind:", _s_opt_str_delegate.idx);
                    var rc;
                    if (mode == "save")
                    {
                        if (_s_opt_str_address.text.length == 0)
                        {
                            error_text = opt_error;
                            return;
                        }

                        if (_s_opt_str_delegate.old_string.length > 0)
                        {
                            rc = dctp_iface.tryChSubnetProperty("del", interface_block.name, _s_opt_str_list.subnet_name, _s_opt_str_list.opt_name, _s_opt_str_delegate.old_string);
                            if (rc == -1) return;
                        }
                        if (_s_opt_str_delegate.new_string.length > 0)
                        {
                           rc = dctp_iface.tryChSubnetProperty("add", interface_block.name, _s_opt_str_list.subnet_name, _s_opt_str_list.opt_name, _s_opt_str_delegate.new_string);
                           if (rc == -1)
                           {
                               if (_s_opt_str_delegate.old_string.length > 0)
                                   dctp_iface.tryChSubnetProperty("add", interface_block.name, _s_opt_str_list.subnet_name, _s_opt_str_list.opt_name, _s_opt_str_delegate.old_string);
                               return;
                           }
                        }
                        _s_opt_str_delegate.old_string = _s_opt_str_delegate.new_string;
                    }
                    else
                    {
                        if (_s_opt_str_delegate.new_string.length > 1)
                        {
                            rc = dctp_iface.tryChSubnetProperty("del", interface_block.name,  _s_opt_str_list.subnet_name, _s_opt_str_list.opt_name, _s_opt_str_delegate.new_string);
                            if (rc == -1) return;
                        }
                        _s_opt_str_list.model.remove(_s_opt_str_delegate.idx);
                    }

                }
            }
        }

        model: ListModel
        {
            id: _s_opt_str_list_model;
        }

        onOpt_typeChanged: choise_regular_expressions(opt_type);

        function update_config()
        {
             var str_list = dctp_iface.getSubnetProperty(interface_block.name, _s_opt_str_list.subnet_name, _s_opt_str_list.opt_name);
             console.log("OPTION ", opt_name, ": ", str_list);
             for (var i = 0; i < str_list.length; i++)
             {
                 _s_opt_str_list_model.append({STR: str_list[i]})
             }
        }

        function choise_regular_expressions(type)
        {
            var regexp;
            if (type === "IP")
                regexp = /((25[0-5]|2[0-4]\d|[01]?\d\d?)\.){3}(25[0-5]|2[0-4]\d|[01]?\d\d?)/;
            else if (type == "INT")
                regexp = /(\d){6}/;
            else
                regexp = undefined;

            return regexp;
        }
}


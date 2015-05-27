import QtQuick 2.0

Rectangle
{
      id: interface_block
      property alias header: interface_block_header
      property alias interface_state: switcher.switch_state;
      property string name: ""
      state: "hovered"
      height: { return hoverblock.sourceComponent != undefined ? (interface_block_header.height + hoverblock.height) :  interface_block_header.height }

      states: [
          State {
              name: "hovered"
              PropertyChanges {target: hoverblock; sourceComponent: undefined; }
              PropertyChanges {target: down_button; text: "+"}
          },
          State {
              name: "unhovered"
              PropertyChanges {target: hoverblock; sourceComponent: hoverblock_component; }
              PropertyChanges {target: down_button; text: "-"}
          }

      ]

      TextField
      {
          id: interface_block_header
          width: parent.width
          height: parent.height

          SButton
          {
              id: down_button
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
                  if (interface_block.state == "hovered")
                      interface_block.state = "unhovered"
                  else
                      interface_block.state = "hovered"
              }
          }

          SwitchSlider
          {
              id: switcher
              width: parent.height * 0.8
              height: parent.height * 0.4
              color: "blue"
              anchors.left: parent.left
              anchors.leftMargin: parent.width/20
              anchors.verticalCenter: parent.verticalCenter
              radius: height/2;
              switch_radius: radius

              onSwitchOn:
              {
                  switch_color = "lime";
              }
              onSwitchOff:
              {
                  switch_color = "red";
              }

              Connections
              {
                  target: dctp_iface
                  onIfaceChangeStateFail:
                  {
                      if (name == signal_arg1)
                      {
                          console.log("error:", dctp_iface.last_error);
                          switcher.switchChange();
                      }
                  }
              }

              onSwitchClick:
              {
                  dctp_iface.tryChangeInterfaceState(name, switch_state);
              }
          }
      }

      Loader
      {
            id: hoverblock
            sourceComponent: undefined
            anchors.top: interface_block_header.bottom
      }

      Component
      {
            id: hoverblock_component
            InterfaceConfigARBlock
            {
                id: address_ranges
                color: interface_block.color
                header.height: interface_block_header.height
                header.width: interface_block_header.width

                Connections
                {
                    target: hoverblock
                    onLoaded:
                    {
                        update_ARs();
                    }
                }

                function update_ARs()
                {
                    var ar_str_list = dctp_iface.getAddressRanges(interface_block.name);
                    console.log("ARS: ", ar_str_list);
                    for (var i = 0; i < ar_str_list.length; i++)
                    {
                        var AR = ar_str_list[i].split('-');
                        view.model.append({SA: AR[0], EA: AR[1] })
                    }
                }
            }
      }
}


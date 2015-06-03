import QtQuick 2.0

Rectangle
{
      id: interface_block
      height: childrenRect.height
      property alias header: interface_block_header
      property alias interface_state: switcher.switch_state;
      property string name: ""
      state: "hovered"

      states: [
          State {
              name: "hovered"
              PropertyChanges {target: hoverblock; sourceComponent: undefined; }
              PropertyChanges {target: down_button; rotation: 0 }
          },
          State {
              name: "unhovered"
              PropertyChanges {target: hoverblock; sourceComponent: hoverblock_component; }
              PropertyChanges {target: down_button; rotation: 180}
          }
      ]

      transitions: [
          Transition {
              from: "hovered"
              to: "unhovered"
              RotationAnimation { running: true; property: "rotation"; target: down_button; from: 0; to: 180; duration: 100; }
          },
          Transition {
              from: "unhovered"
              to: "hovered"
              RotationAnimation { running: true; property: "rotation"; target: down_button; from: 180; to: 0; duration: 100; }
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
              width: parent.height * 0.8
              height: width
              color: parent.color

              anchors.right: parent.right
              anchors.rightMargin: parent.width/20
              anchors.verticalCenter: parent.verticalCenter
              br_image: ""
              bk_image: "qrc:/images/arrow_down.png"

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
            anchors.topMargin: sourceComponent == undefined ? 0 : interface_block_header.height/5
            height: childrenRect.height
      }

      Component
      {
            id: hoverblock_component
            InteraceConfig_SubnetBlock
            {
                id: subnets
                color: interface_block.color
                header_height: interface_block_header.height
                header_width: interface_block_header.width

                Connections
                {
                    target: hoverblock
                    onLoaded:
                    {
                        update_subnets();
                    }
                }

                function update_subnets()
                {
                    var sub_str_list = dctp_iface.getSubnets(interface_block.name);
                    console.log("SUBS: ", sub_str_list);
                    for (var i = 0; i < sub_str_list.length; i++)
                    {
                        var AR = sub_str_list[i].split(' ');
                        model.append({SA: AR[0], NM: AR[1] })
                    }
                }
            }
      }
}


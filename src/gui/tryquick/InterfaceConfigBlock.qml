import QtQuick 2.0

Rectangle
{
      id: interface_block
      property alias header: interface_block_header
      property alias interface_state: switcher.switch_state;
      property string name: ""

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
}


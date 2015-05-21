import QtQuick 2.0

Rectangle
{
      id: interface_block
      property alias header: interface_block_header

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

              onStateChanged: {
                    //console.log("state ", state);
                   // dctp_iface.doInThread();
              }
          }
      }
}


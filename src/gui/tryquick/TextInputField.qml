import QtQuick 2.4

Rectangle
{
    id: container
    property alias text: edit.text
    property alias font: edit.font
    property alias textcolor: edit.color
    property alias bkcolor: container.color
    property alias maximumLength: edit.maximumLength
    property alias readonly: edit.readOnly
    property alias validator: edit.validator
    property alias text_mask: edit.inputMask
    property alias textHAlign: edit.horizontalAlignment
    signal returnPressed()

    Flickable
    {
         id: flick

         width: container.width
         height: container.height
         anchors.centerIn: container
         contentWidth: edit.contentWidth
         contentHeight: edit.contentHeight
         clip: true
         interactive: false
         leftMargin: height/10
         rightMargin: height/10
         bottomMargin: height/20
         topMargin: height/20

         function ensureVisible(r)
         {
             if (contentX >= r.x)
                 contentX = r.x;
             else if (contentX + width <= r.x+r.width)
                 contentX = r.x+r.width-width;
             if (contentY >= r.y)
                 contentY = r.y;
             else if (contentY + height <= r.y+r.height)
                 contentY = r.y+r.height-height;
         }

         TextInput
         {
             id: edit
             width: container.width
             height: container.height

             verticalAlignment: Text.AlignVCenter
             font.pixelSize: height - height/3

             readOnly: false;
             enabled: true;

             onAccepted:
             {
                 returnPressed();
             }

             onTextChanged:
             {
                 ensureVisible(cursorRectangle);
             }

           }
     }
}

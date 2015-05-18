import QtQuick 2.4

Rectangle
{
    id: container
    property alias textcolor: input_area.color
    property alias bkcolor: container.color
    readonly property string text: (input_area.fl_left_window + input_area.fl_center_window + input_area.fl_right_window)
    property alias font: input_area.font
    signal returnPressed()

    TextInput
    {
        id: input_area
        width: parent.width
        height: parent.height

        anchors.fill: parent
        anchors.margins: height/10
        verticalAlignment: TextInput.AlignVCenter
        font.pixelSize: height - height/5

        property string fl_left_window: ""
        property string fl_right_window: ""
        property string fl_center_window:  ""
        property int fl_len: 5;

        onAccepted:
        {
            returnPressed();
        }

        onContentWidthChanged:
        {
            if (text.length > fl_len)
            {
                fl_center_window = text.substring(text.length - fl_len, fl_len + 1);
                fl_left_window += text.substring(0, text.length - fl_len);
                text = fl_center_window;
            }

            if (fl_len > text.length)
            {
                console.log(2);
                if (fl_left_window.length > 0)
                    fl_center_window = fl_left_window[fl_left_window.length - 1] + text;
                else
                    fl_center_window = text;

                if (fl_center_window.length < fl_len && fl_right_window.length > 0)
                {
                    fl_center_window += fl_right_window[0];
                    fl_right_window = fl_right_window.substring(1, fl_right_window.length);
                }

                fl_left_window = fl_left_window.substring(0, fl_left_window.length - 1);
                text = fl_center_window;
            }

            console.log("left:", fl_left_window);
            console.log("center:", fl_center_window);
            console.log("right:", fl_right_window);
        }

        onCursorPositionChanged:
        {

        }

        Keys.onPressed:
        {

            if (event.key == Qt.Key_Left)
            {
                if (input_area.cursorPosition == 0)
                {
                    console.log("shift left");
                    if (input_area.fl_left_window.length == 0) return;
                    else
                    {
                        input_area.fl_right_window = input_area.fl_center_window[input_area.fl_center_window.length - 1] + input_area.fl_right_window;
                        input_area.fl_center_window = input_area.fl_left_window[input_area.fl_left_window.length - 1] + input_area.fl_center_window.substring(0, input_area.fl_center_window.length - 1);
                        input_area.fl_left_window = input_area.fl_left_window.substring(0, input_area.fl_left_window.length - 1);
                        input_area.text = input_area.fl_center_window;

                        console.log("left:", input_area.fl_left_window);
                        console.log("center:", input_area.fl_center_window);
                        console.log("right:", input_area.fl_right_window);
                    }
                }
            }

            if (event.key == Qt.Key_Right)
            {
                if (input_area.cursorPosition == input_area.text.length)
                {
                    console.log("shift right");
                    if (input_area.fl_right_window.length == 0) return;
                    else
                    {
                        input_area.fl_left_window += input_area.fl_center_window[0];
                        input_area.fl_center_window = input_area.fl_center_window.substring(1, input_area.fl_center_window.length) + input_area.fl_right_window[0];
                        input_area.fl_right_window = input_area.fl_right_window.substring(1, input_area.fl_right_window.length);
                        input_area.text = input_area.fl_center_window;

                        console.log("left:", input_area.fl_left_window);
                        console.log("center:", input_area.fl_center_window);
                        console.log("right:", input_area.fl_right_window);
                    }
                }
            }
        }

   }
}

import QtQuick 2.0

ScreenView
{
    id: subnet_view
    property string name: ""
    main_entry: _main
    visible: false

    Component
    {
        id: _main
        Rectangle
        {
            anchors.fill: parent

            SButton
            {
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 20

                height: parent.height / 10
                width: height * 2

                textcolor: "yellow"
                text: "Готово"
            }
        }
    }
}


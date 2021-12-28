import QtQuick 2.15

Rectangle {
    id: root
    implicitWidth: 600
    implicitHeight: 300
    color: "#292f37"
    border.color: 'grey'

    property string imgSource: ""
    property real gradientValue: 0.0
    property int digitalValue: 0
    property int sliderValue: 0
    property int overlayX: 0
    property int overlayY: 0

    signal startProcessing()

    Image{
        id: img
        anchors.top: root.top
        anchors.left: root.left
        source: Qt.resolvedUrl(root.imgSource)
    }

    Overlay{
        id: overlay
        anchors.top: root.top
        anchors.left: root.left
        width: 600
        height: 300
    }

    MouseArea {
        anchors.fill: root
        propagateComposedEvents: false
    }
}

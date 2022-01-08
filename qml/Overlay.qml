import QtQuick 2.0

Rectangle {
    id: root
    implicitWidth: 600
    implicitHeight: 300
    color: 'transparent'

//    property alias myText: numericValue.text
//    property int numericTextValue: 0
//    property real shape: 0.0
//    property real slider: 0.0
    property bool animationRunning: false

    onColorChanged: {
        console.log("color set");
    }

//    onNumericTextValueChanged: {
//        console.log("numericTextValue set");
//    }

//    onShapeChanged: {
//        gradientTopTransparent.position = OVERLAY_SHAPE;
//        gradientTop.position = OVERLAY_SHAPE + 0.01;
//        gradientTop.color.a = 1.0 - gradientTop.position;
//        console.log("shape set");
//    }

    Timer {
        id: numericValueTimer
        interval: 1000; running: root.animationRunning; repeat: true
        onTriggered: {
            js.generateNumericValue();
        }
    }

    Timer {
        id: gradientTimer
        interval: 300; running: root.animationRunning; repeat: true
        onTriggered: {
            js.generateGradientPosition();
        }
    }

    Item{
        id: js

        function generateNumericValue(){
            root.numeric = Math.floor(Math.random() * 999);
        }

        function generateGradientPosition(){
            root.shape = clamp(Math.random(), 0.0, 0.7);
        }

        function clamp(n, min, max) {
          if(n > max){ return max; }
          else if(n < min){ return min }
          else{ return n; }
        }
    }

    Image{
        id: frame
        anchors.fill: root
        source: "img/overlay_frame.png"
    }

    Rectangle{
        id: gradientRect
        anchors.bottom: numericValue.bottom
        anchors.bottomMargin: 30
        anchors.left: root.left
        anchors.leftMargin: 90
        width: 76
        height: 96
        gradient: Gradient {
            GradientStop {
                id: gradientTopTransparent
                position: 1.0 - OVERLAY_SHAPE
                color: 'transparent'
            }
            GradientStop {
                id: gradientTop
                position: 1.0 - OVERLAY_SHAPE + 0.01
                color.r: 0.939
                color.g: 0.459
                color.b: 0.006
                color.a: OVERLAY_SHAPE
            }
            GradientStop {
                id: gradientBottom
                position: 1.0
                color: '#00f07502'
            }
        }
    }

    Text{
        id: numericValue
        anchors.top: root.top
        anchors.topMargin: 30
        anchors.right: root.right
        anchors.rightMargin: 74
        horizontalAlignment: Text.AlignRight
        width: 200
        text: OVERLAY_NUMERIC
        font.pointSize: 100
        color: '#f07502'
    }

    Rectangle{
        id: range
        anchors.bottom: root.bottom
        anchors.bottomMargin: 70
        anchors.left: root.left
        anchors.leftMargin: 90
        width: 420
        height: 4
        color: '#f07502'
    }

    Image{
        id: pointer
        anchors.left: root.left
        anchors.leftMargin: 72 + (420 * OVERLAY_SLIDER)
        anchors.bottom: range.bottom
        anchors.bottomMargin: 5
        source: "img/overlay_pointer.png"

        SequentialAnimation on anchors.leftMargin {
            loops: Animation.Infinite
            PropertyAnimation {
                duration: 500
                to: 492 }
            PropertyAnimation {
                duration: 500
                to: 72 }
        }
    }
}

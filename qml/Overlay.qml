import QtQuick 2.0

Item {
    id: root

    Timer {
        id: numericValueTimer
        interval: 300; running: true; repeat: true
        onTriggered: {
            numericValue.text = js.generateNumericValue().toString();
        }
    }

    Timer {
        id: gradientTimer
        interval: 1000; running: true; repeat: true
        onTriggered: {
            gradientTopTransparent.position = js.generateGradientPosition();
            gradientTop.position = gradientTopTransparent.position + 0.01;
            gradientTop.color.a = 1.0 - gradientTop.position;
        }
    }

    Item{
        id: js

        function generateNumericValue(){
            return Math.floor(Math.random() * 999);
        }

        function generateGradientPosition(){
            return clamp(Math.random(), 0.0, 0.7);
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
        source: "Img/overlay_frame.png"
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
                position: 0.0; color: 'transparent'
            }
            GradientStop {
                id: gradientTop
                position: 0.01; color: '#fff07502'
            }
            GradientStop {
                id: gradientBottom
                position: 1.0; color: '#00f07502'
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
        text: '487'
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
        anchors.leftMargin: 72
        anchors.bottom: range.bottom
        anchors.bottomMargin: 5
        source: "Img/overlay_pointer.png"

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

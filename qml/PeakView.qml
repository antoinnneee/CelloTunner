import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: "#2d2d2d"
    radius: 4
    
    property var peaks: []
    property double maxFrequency: 1100 // Maximum frequency to display
    property double minFrequency: 50   // Minimum frequency to display
    
    // Title
    Label {
        id: title
        text: "Frequency Peaks"
        color: "#ffffff"
        font.pixelSize: 14
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.margins: 8
    }

    // Peak visualization
    Item {
        anchors.fill: parent
        anchors.topMargin: 30
        anchors.bottomMargin: 30
        anchors.leftMargin: 40
        anchors.rightMargin: 10

        // Frequency axis markers
        Row {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 20
            spacing: parent.width / 5

            Repeater {
                model: 6
                Label {
                    text: Math.round(minFrequency + (maxFrequency - minFrequency) * index / 5) + "Hz"
                    color: "#9e9e9e"
                    font.pixelSize: 10
                }
            }
        }

        // Amplitude axis markers
        Column {
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            width: 35
            spacing: parent.height / 4

            Repeater {
                model: 5
                Label {
                    text: (100 - index * 25) + "%"
                    color: "#9e9e9e"
                    font.pixelSize: 10
                }
            }
        }

        // Peak bars
        Repeater {
            model: peaks
            Rectangle {
                property double freq: modelData.frequency
                property double amp: modelData.amplitude
                
                x: (freq - minFrequency) / (maxFrequency - minFrequency) * parent.width
                y: parent.height * (1 - amp)
                width: 4
                height: parent.height * amp
                radius: 2
                
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#4CAF50" }
                    GradientStop { position: 0.7; color: "#FFC107" }
                    GradientStop { position: 1.0; color: "#FF5722" }
                }
                Rectangle {
                    color:"red"
                    width: 4
                    height: 4
                    radius: 2
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                // Frequency label
                Label {
                    text: Math.round(freq) + "Hz"
                    color: "#ffffff"
                    font.pixelSize: 10
                    anchors.bottom: parent.top
                    anchors.horizontalCenter: parent.horizontalCenter
                    rotation: -45
                    visible: index < 3 // Show only top 3 frequencies
                }
            }
        }

        // Grid lines
        Canvas {
            anchors.fill: parent
            onPaint: {
                var ctx = getContext("2d");
                ctx.strokeStyle = "#444444";
                ctx.lineWidth = 1;

                // Horizontal lines
                for (var i = 0; i <= 4; i++) {
                    ctx.beginPath();
                    ctx.moveTo(0, i * height / 4);
                    ctx.lineTo(width, i * height / 4);
                    ctx.stroke();
                }

                // Vertical lines
                for (var j = 0; j <= 5; j++) {
                    ctx.beginPath();
                    ctx.moveTo(j * width / 5, 0);
                    ctx.lineTo(j * width / 5, height);
                    ctx.stroke();
                }
            }
        }
    }
} 

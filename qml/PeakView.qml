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
    property bool logarithmicScale: true // Default to logarithmic scale
    onLogarithmicScaleChanged: {
        grid.clear_canvas();
        grid.requestPaint();
    }
    
    // Helper functions for logarithmic scale
    function logScale(value) {
        return Math.log(value / minFrequency) / Math.log(maxFrequency / minFrequency);
    }

    function getFrequencyPosition(freq) {
        var freqPos = logarithmicScale ?
                    logScale(freq) * graphArea.width :
                    (freq - minFrequency) / (maxFrequency - minFrequency) * graphArea.width;
        return freqPos;
    }
    function getFrequencyPositionLabel(freq) {
        var freqPos = logarithmicScale ?
                    logScale(freq) * graphArea.width :
                    (freq - minFrequency) / (maxFrequency - minFrequency) * graphArea.width;
        return freqPos;
    }

    function getFrequencyAtPosition(pos) {
        if (logarithmicScale) {
            let logMin = Math.log(minFrequency);
            let logMax = Math.log(maxFrequency);
            return Math.exp(logMin + (pos / graphArea.width) * (logMax - logMin));
        }
        return minFrequency + (pos / graphArea.width) * (maxFrequency - minFrequency);
    }
    
    ColumnLayout {
        anchors.fill: parent
        spacing: 4
        
        // Header with title and scale switch
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 8
            spacing: 10

            Label {
                text: "Frequency Peaks"
                color: "#ffffff"
                font.pixelSize: 14
                Layout.fillWidth: true
            }

            Label {
                text: "Log Scale"
                color: "#9e9e9e"
                font.pixelSize: 12
            }

            Switch {
                id: switchLog
                checked: root.logarithmicScale
                onCheckedChanged: logarithmicScale = switchLog.checked
                Material.accent: Material.Purple
                visible: false
            }
        }

        // Peak visualization
        Item {
            id: graphArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 8
            Layout.leftMargin: 40
            Layout.rightMargin: 10
            Layout.bottomMargin: 30
            // Frequency axis markers
            Row {
                id: rowFreqLabel
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                height: 20
                width: parent.width

                Repeater {
                    model: logarithmicScale ? 
                        [50, 100, 200, 500, 1000] :
                        [50, 250, 450, 650, 850, 1050]

                    Label {
                        x: getFrequencyPositionLabel(modelData)
                        text: modelData + "Hz"
                        color: "#9e9e9e"
                        font.pixelSize: 10
                    }
                }
            }

            // Amplitude axis markers
            ColumnLayout {
                id: ampAxisMarker
                anchors.top: parent.top
                anchors.topMargin: -15
                anchors.bottom: parent.bottom
                anchors.bottomMargin: -12
                anchors.left: parent.left
                anchors.leftMargin: -15

                width: 35

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
                    property double freq: modelData.frequency || 0
                    property double amp: modelData.amplitude || 0
                    property int harmonics: modelData.harmonicCount || 0
                    
                    x: getFrequencyPosition(freq)
                    y: parent.height * (1 - amp)
                    width: 4
                    height: parent.height * amp
                    radius: 2
                    
                    gradient: Gradient {
                        GradientStop { 
                            position: 0.0
                            color: harmonics > 0 ? "#4CAF50" : "#455A64"  // Green if has harmonics, grey if not
                        }
                        GradientStop { 
                            position: 0.7
                            color: harmonics > 0 ? "#FFC107" : "#78909C"
                        }
                        GradientStop { 
                            position: 1.0
                            color: "#FF5722"
                        }
                    }

                    // Peak dot
                    Rectangle {
                        color: "#FF5722"
                        width: 4
                        height: 4
                        radius: 2
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottom: parent.top
                    }

                    // Peak information
                    Column {
                        anchors.bottom: parent.top
                        anchors.bottomMargin: 6
                        anchors.horizontalCenter: parent.horizontalCenter
                        spacing: 2

                        Label {
                            text: Math.round(freq) + " Hz"
                            color: "#ffffff"
                            font.pixelSize: 10
                            anchors.horizontalCenter: parent.horizontalCenter
                        }

                        Label {
                            visible: harmonics > 0
                            text: harmonics + (harmonics === 1 ? " harmonic" : " harmonics")
                            color: "#4CAF50"  // Green color for harmonics
                            font.pixelSize: 9
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                }
            }

            // Grid lines
            Canvas {
                id: grid
                anchors.fill: parent
                function clear_canvas() {
                            var ctx = getContext("2d");
                            ctx.reset();
                }
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
                    if (logarithmicScale) {
                        console.log("draw vertical line")
                        let freqs = [50, 100, 200, 500, 1000];
                        freqs.forEach(freq => {
                            let x = getFrequencyPosition(freq);
                            ctx.beginPath();
                            ctx.moveTo(x, 0);
                            ctx.lineTo(x, height);
                            ctx.stroke();
                        });
                    } else {
                        for (var j = 0; j <= 5; j++) {
                            ctx.beginPath();
                            ctx.moveTo(j * width / 5, 0);
                            ctx.lineTo(j * width / 5, height);
                            ctx.stroke();
                        }
                    }
                }
            }

            // Hover frequency indicator
            Rectangle {
                id: frequencyIndicator
                width: 2
                height: parent.height
                color: "#ffffff"
                opacity: 0.5
                visible: false

                // Frequency label
                Rectangle {
                    color: "#2d2d2d"
                    border.color: "#ffffff"
                    border.width: 1
                    radius: 4
                    height: freqLabel.height + 8
                    width: freqLabel.width + 16
                    anchors.bottom: parent.top
                    anchors.horizontalCenter: parent.horizontalCenter

                    Label {
                        id: freqLabel
                        color: "#ffffff"
                        font.pixelSize: 12
                        anchors.centerIn: parent
                    }
                }
            }

            // Mouse area for frequency detection
            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                
                onPositionChanged: (mouse) => {
                    if (containsMouse) {
                        let freq = getFrequencyAtPosition(mouse.x);
                        freqLabel.text = Math.round(freq) + " Hz";
                        frequencyIndicator.x = mouse.x;
                        frequencyIndicator.visible = true;
                    }
                }
                
                onExited: {
                    frequencyIndicator.visible = false;
                }
            }
        }
    }
}

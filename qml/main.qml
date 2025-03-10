import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import Qt.labs.platform
import QtQuick.Controls.Material
import QtCore

ApplicationWindow {
    id: root
    width: 640
    height: 800
    visible: true
    title: "Cello Tuner"
    
    MicrophonePermission {
        id: microphonePermission

        onStatusChanged: {
            console.log("Microphone permission changed:", status)
            if (status == Qt.PermissionStatus.Granted) {
                tuner.reload()
            }
        }

        Component.onCompleted: {
            if (status != Qt.PermissionStatus.Granted) {
                console.log("Microphone permission not granted")
                microphonePermission.request()
            }
        }
    }
    // Settings storage
    Settings {
        id: appSettings
        property int sampleRate: 44100
        property int bufferSize: 8112
        property int maxPeaks: 10
        property double referenceA: 440.0
        property double dbThreshold: -70.0
    }

    // Load settings when app starts
    Component.onCompleted: {
        tuner.sampleRate = appSettings.sampleRate
        tuner.bufferSize = appSettings.bufferSize
        tuner.maxPeaks = appSettings.maxPeaks
        tuner.referenceA = appSettings.referenceA
        tuner.dbThreshold = appSettings.dbThreshold
    }

    // Add property change monitoring
    Connections {
        target: tuner
        function onSignalLevelChanged() {
        }
        function onCurrentNoteChanged() {
        }
    }
    
    Material.theme: Material.Dark
    Material.accent: Material.Purple
    color: "#1a1a1a"  // Dark background

    // Settings dialog
    SettingsDialog {
        id: settingsDialog
        width: (root.width < 400) ? root.width : root.width * 0.8
    }

    // Donation dialog
    DonationDialog {
        id: donationDialog
        width: (root.width < 400) ? root.width : root.width * 0.8
    }

    // Settings button in the header
    header: ToolBar {
        Material.background: "#2d2d2d"
        RowLayout {
            anchors.fill: parent
            Label {
                text: "Cello Tuner"
                font.pixelSize: 20
                Layout.leftMargin: 20
                color: "#ffffff"
            }
            Item { Layout.fillWidth: true }
            ToolButton {
                icon.source: "qrc:/icons/settings.png"  // You'll need to add this icon to your resources
                icon.color: "#ffffff"
                onClicked: settingsDialog.open()
                text: "Settings"
                display: AbstractButton.TextBesideIcon
                Layout.rightMargin: 10
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 30

        // Signal Level Meter
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 45  // Increased height to accommodate top label
            color: "#2d2d2d"
            radius: 4

            // Current level indicator
            Label {
                text: tuner.signalLevel.toFixed(1) + " dB"
                color: "#ffffff"
                font.pixelSize: 10
                anchors.bottom: levelMeter.top
                anchors.bottomMargin: 2
                x: {
                    let dbLevel = Math.max(-70, Math.min(0, tuner.signalLevel));
                    let percentage = (dbLevel + 70) / 70;
                    return Math.min(Math.max(0, parent.width * percentage - width/2), parent.width - width);
                }
            }

            Rectangle {
                id: levelMeter
                height: parent.height - 19  // Adjusted for top label
                anchors.left: parent.left
                anchors.leftMargin: 2
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 12  // Space for bottom labels
                radius: 2
                
                // Convert dBFS to width percentage (-70dB to 0dB range)
                width: {
                    let dbLevel = Math.max(-70, Math.min(0, tuner.signalLevel));
                    let percentage = (dbLevel + 70) / 70;
                    return parent.width * percentage;
                }
                
                // Color gradient based on level
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#4CAF50" }
                    GradientStop { position: 0.7; color: "#FFC107" }
                    GradientStop { position: 0.9; color: "#FF5722" }
                }

                Behavior on width {
                    SmoothedAnimation {
                        duration: 50
                    }
                }
            }

            // Level markers
            Row {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: parent.height - 12
                spacing: parent.width / 6
                Repeater {
                    model: 6
                    Rectangle {
                        width: 1
                        height: parent.height
                        color: "#666666"
                    }
                }
            }

            // dB labels
            Row {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 12
                spacing: parent.width / 6
                Repeater {
                    model: ["-70", "-60", "-50", "-40", "-30", "-20", "-10", "0"]
                    Label {
                        text: modelData
                        color: "#666666"
                        font.pixelSize: 10
                    }
                }
            }

            // Threshold indicator and handle
            Item {
                id: thresholdHandle
                width: 20
                height: 20
                x: {
                    let threshold = Math.max(-70, Math.min(0, tuner.dbThreshold));
                    let percentage = (threshold + 70) / 70;
                    return parent.width * percentage - width/2;
                }
                y: parent.height 

                // Triangle handle
                Canvas {
                    anchors.fill: parent
                    onPaint: {
                        var ctx = getContext("2d");
                        ctx.reset();
                        ctx.beginPath();
                        ctx.moveTo(width/2, 0);  // Point at top
                        ctx.lineTo(0, 10);      // Bottom left
                        ctx.lineTo(width, 10);   // Bottom right
                        ctx.closePath();
                        ctx.fillStyle = "#ffffff";
                        ctx.fill();
                    }
                }

                // Make the handle draggable
                MouseArea {
                    anchors.fill: parent
                    drag {
                        target: parent
                        axis: Drag.XAxis
                        minimumX: -width/2
                        maximumX: parent.parent.width - width/2
                    }
                    onPositionChanged: {
                        if (drag.active) {
                            let percentage = (thresholdHandle.x + width/2) / parent.parent.width;
                            let newThreshold = percentage * 70 - 70;
                            tuner.dbThreshold = newThreshold;
                            appSettings.dbThreshold = newThreshold;
                        }
                    }
                }

                // Threshold value tooltip
                Label {
                    text: tuner.dbThreshold.toFixed(1) + " dB"
                    color: "#ffffff"
                    font.pixelSize: 10
                    anchors.top: parent.bottom
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }

            // Threshold line
            Rectangle {
                width: 2
                height: parent.height - 19  // Match levelMeter height
                color: "#FFFFFF"
                opacity: 0.5
                x: thresholdHandle.x + thresholdHandle.width/2
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 2
            }
        }

        // Signal level display
        Label {
            visible: false
            Layout.fillWidth: true
            text: "Level: " + tuner.signalLevel.toFixed(1) + " dBFS"
            font.pixelSize: 14
            color: "#9e9e9e"
            horizontalAlignment: Text.AlignHCenter
        }

        // Note display
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 180
            Layout.preferredHeight: 180
            radius: width / 2
            color: {
                if (tuner.signalLevel <= tuner.dbThreshold) return color  // Grey when signal too low
                else if (Math.abs(tuner.cents) < 5) return "#4CAF50"  // Green when in tune
                else if (Math.abs(tuner.cents) < 15) return "#FFC107"  // Yellow when close
                else return "#455A64"  // Blue-grey when far off
            }
            opacity: tuner.signalLevel <= tuner.dbThreshold ? 0.5 : 1.0  // Dim when signal too low
            
            Label {
                anchors.centerIn: parent
                text: tuner.currentNote
                font.pixelSize: 72
                font.bold: true
                color: "white"
                opacity: parent.opacity
            }

            // Add indicator text when signal is too low
            Label {
                anchors.top: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                text: tuner.signalLevel <= tuner.dbThreshold ? "Signal too weak" : ""
                color: "#9e9e9e"
                font.pixelSize: 14
                visible: text !== ""
            }
        }

        // Tuning meter
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            color: "#2d2d2d"
            radius: 8

            Rectangle {
                id: tuningIndicator
                width: 4
                height: parent.height - 10
                radius: 2
                color: "white"
                anchors.verticalCenter: parent.verticalCenter
                x: parent.width / 2 + (tuner.cents * parent.width / 100)
                
                Behavior on x {
                    SmoothedAnimation {
                        duration: 150
                    }
                }
            }

            // Center marker
            Rectangle {
                width: 2
                height: parent.height
                color: "#4CAF50"
                anchors.centerIn: parent
            }

            // Tuning scale markers
            Row {
                anchors.centerIn: parent
                spacing: parent.width / 10
                Repeater {
                    model: 9
                    Rectangle {
                        width: 1
                        height: 20
                        color: "#666666"
                    }
                }
            }
        }

        // Frequency and tuning information
        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 8

            // Frequency display
            Label {
                Layout.alignment: Qt.AlignHCenter
                text: tuner.frequency.toFixed(1) + " Hz"
                font.pixelSize: 24
                color: "#ffffff"
            }

            // Cents deviation
            Label {
                Layout.alignment: Qt.AlignHCenter
                text: (tuner.cents >= 0 ? "+" : "") + tuner.cents.toFixed(1) + " cents"
                font.pixelSize: 18
                color: "#9e9e9e"
            }

            // Tuning guidance
            Label {
                Layout.alignment: Qt.AlignHCenter
                text: {
                    if (Math.abs(tuner.cents) < 5) return "In tune! ✓"
                    else if (tuner.cents > 0) return "Lower the pitch ↓"
                    else return "Raise the pitch ↑"
                }
                font.pixelSize: 20
                color: {
                    if (Math.abs(tuner.cents) < 5) return "#4CAF50"
                    else if (Math.abs(tuner.cents) < 15) return "#FFC107"
                    else return "#FF5722"
                }
            }
        }

        // Peak visualization
        PeakView {
            Layout.fillWidth: true
            Layout.preferredHeight: 200
            peaks: tuner.peaks
        }
    }
}

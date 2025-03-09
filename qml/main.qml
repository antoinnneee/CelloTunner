import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import Qt.labs.platform
import QtQuick.Controls.Material

ApplicationWindow {
    id: root
    width: 640
    height: 800
    visible: true
    title: "Cello Tuner"
    
    // Add property change monitoring
    Connections {
        target: tuner
        function onSignalLevelChanged() {
            console.log("QML: Signal level updated:", tuner.signalLevel)
        }
        function onCurrentNoteChanged() {
            console.log("QML: Note changed:", tuner.currentNote)
        }
    }
    
    Material.theme: Material.Dark
    Material.accent: Material.Purple
    color: "#1a1a1a"  // Dark background

    // Settings dialog
    Dialog {
        id: settingsDialog
        title: "Settings"
        modal: true
        standardButtons: Dialog.Close
        width: 400

        anchors.centerIn: parent

        footer: DialogButtonBox {
            Button {
                text: qsTr("Apply")
                DialogButtonBox.buttonRole: DialogButtonBox.ApplyRole
                highlighted: true
                onClicked: {
                    tuner.sampleRate = parseInt(sampleRateCombo.currentText)
                    tuner.bufferSize = parseInt(bufferSizeCombo.currentText)
                }
            }
            Button {
                text: qsTr("Close")
                DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
                onClicked: settingsDialog.close()
            }
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 20

            Label {
                text: "Sample Rate"
                color: "#ffffff"
            }

            RowLayout {
                Layout.fillWidth: true
                ComboBox {
                    id: sampleRateCombo
                    Layout.fillWidth: true
                    model: {
                        // Standard sample rates
                        let rates = [8000, 11025, 16000, 22050, 32000, 44100, 48000, 88200, 96000, 176400, 192000];
                        // Filter rates that are above maximum supported rate
                        return rates.filter(rate => rate <= tuner.maximumSampleRate).map(rate => rate.toString());
                    }
                    currentIndex: {
                        let idx = model.indexOf(tuner.sampleRate.toString());
                        return idx >= 0 ? idx : 0;
                    }
                    Material.foreground: "white"
                    Material.background: "#2d2d2d"
                }
                Label {
                    text: "Hz"
                    color: "#9e9e9e"
                }
            }

            Label {
                text: "Buffer Size"
                color: "#ffffff"
            }

            RowLayout {
                Layout.fillWidth: true
                ComboBox {
                    id: bufferSizeCombo
                    Layout.fillWidth: true
                    model: ["1024", "2048", "4096", "8112", "16384"]
                    currentIndex: {
                        let idx = model.indexOf(tuner.bufferSize.toString());
                        return idx >= 0 ? idx : 0;
                    }
                    Material.foreground: "white"
                    Material.background: "#2d2d2d"
                }
                Label {
                    text: "samples"
                    color: "#9e9e9e"
                }
            }

            // Current values display
            ColumnLayout {
                Layout.topMargin: 20
                Layout.fillWidth: true
                spacing: 5

                Label {
                    text: "Current Settings:"
                    color: "#9e9e9e"
                    font.bold: true
                }

                Label {
                    text: "Sample Rate: " + tuner.sampleRate + " Hz"
                    color: "#9e9e9e"
                }

                Label {
                    text: "Maximum Sample Rate: " + tuner.maximumSampleRate + " Hz"
                    color: "#9e9e9e"
                }

                Label {
                    text: "Buffer Size: " + tuner.bufferSize + " samples"
                    color: "#9e9e9e"
                }

                Label {
                    text: "Latency: " + (tuner.bufferSize / tuner.sampleRate * 1000).toFixed(1) + " ms"
                    color: "#9e9e9e"
                }
            }
        }
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
            Layout.preferredHeight: 30
            color: "#2d2d2d"
            radius: 4

            Rectangle {
                id: levelMeter
                height: parent.height - 4
                anchors.left: parent.left
                anchors.leftMargin: 2
                anchors.verticalCenter: parent.verticalCenter
                radius: 2
                
                // Convert dBFS to width percentage (-60dB to 0dB range)
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

            // Threshold indicator
            Rectangle {
                width: 2
                height: parent.height
                color: "#FFFFFF"
                opacity: 0.5
                x: {
                    let threshold = Math.max(-70, Math.min(0, tuner.dbThreshold));
                    let percentage = (threshold + 70) / 70;
                    return parent.width * percentage;
                }
            }

            // Level markers
            Row {
                anchors.fill: parent
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
                anchors.fill: parent
                spacing: parent.width / 6
                Repeater {
                    model: ["-70", "-60", "-50", "-40", "-30", "-20", "-10", "0"]
                    Label {
                        text: modelData
                        color: "#666666"
                        font.pixelSize: 10
                        anchors.bottom: parent.bottom
                    }
                }
            }
        }

        // Signal level and threshold controls
        RowLayout {
            Layout.fillWidth: true
            spacing: 20

            Label {
                text: "Level: " + tuner.signalLevel.toFixed(1) + " dBFS"
                font.pixelSize: 14
                color: "#9e9e9e"
            }

            Label {
                text: "Threshold:"
                font.pixelSize: 14
                color: "#9e9e9e"
            }

            Slider {
                Layout.fillWidth: true
                from: -70
                to: 0
                stepSize: 1
                value: tuner.dbThreshold
                onValueChanged: tuner.dbThreshold = value

                background: Rectangle {
                    x: parent.leftPadding
                    y: parent.topPadding + parent.availableHeight / 2 - height / 2
                    implicitWidth: 200
                    implicitHeight: 4
                    width: parent.availableWidth
                    height: implicitHeight
                    radius: 2
                    color: "#2d2d2d"

                    Rectangle {
                        width: parent.width * (parent.parent.value + 70) / 70
                        height: parent.height
                        color: Material.accent
                        radius: 2
                    }
                }

                handle: Rectangle {
                    x: parent.leftPadding + parent.visualPosition * parent.availableWidth - width / 2
                    y: parent.topPadding + parent.availableHeight / 2 - height / 2
                    implicitWidth: 16
                    implicitHeight: 16
                    radius: 8
                    color: parent.pressed ? Material.accent : "#f0f0f0"
                    border.color: Material.accent
                }
            }

            Label {
                text: tuner.dbThreshold.toFixed(1) + " dB"
                font.pixelSize: 14
                color: "#9e9e9e"
                Layout.minimumWidth: 60
            }
        }

        // Note display
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 200
            Layout.preferredHeight: 200
            radius: width / 2
            color: {
                if (tuner.signalLevel <= tuner.dbThreshold) return "#455A64"  // Grey when signal too low
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

        // Frequency display
        Label {
            Layout.alignment: Qt.AlignHCenter
            text: tuner.frequency.toFixed(1) + " Hz"
            font.pixelSize: 24
            color: "#ffffff"
        }

        // Peak visualization
        PeakView {
            Layout.fillWidth: true
            Layout.preferredHeight: 200
            peaks: tuner.peaks
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

        // Cents deviation
        Label {
            Layout.alignment: Qt.AlignHCenter
            text: (tuner.cents >= 0 ? "+" : "") + tuner.cents.toFixed(1) + " cents"
            font.pixelSize: 18
            color: "#9e9e9e"
        }

        // Reference note selector
        ComboBox {
            Layout.alignment: Qt.AlignHCenter
            model: ["C2 (65.41 Hz)", "G2 (98.00 Hz)", "D3 (146.83 Hz)", "A3 (220.00 Hz)"]
            currentIndex: 3  // A3 by default
            Material.foreground: "white"
            Material.background: "#2d2d2d"
        }
    }
}

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCore


Dialog {
    id: settingsDialog
    title: "Settings"
    modal: true

    // Settings storage
    Settings {
        id: settingsStorage
        property int sampleRate: 44100
        property int bufferSize: 8112
        property int maxPeaks: 10
        property double referenceA: 440.0
    }

    // Load settings when dialog is created
    Component.onCompleted: {
        sampleRateCombo.currentIndex = sampleRateCombo.indexOfValue(settingsStorage.sampleRate)
        bufferSizeCombo.currentIndex = bufferSizeCombo.indexOfValue(settingsStorage.bufferSize)
        maxPeaksSpinBox.value = settingsStorage.maxPeaks
        referenceASpinBox.value = settingsStorage.referenceA
    }

    anchors.centerIn: parent

    footer: DialogButtonBox {
        Button {
            text: qsTr("Apply")
            DialogButtonBox.buttonRole: DialogButtonBox.ApplyRole
            highlighted: true
            onClicked: {
                // Apply and save settings
                tuner.sampleRate = parseInt(sampleRateCombo.currentText)
                tuner.bufferSize = parseInt(bufferSizeCombo.currentText)
                tuner.maxPeaks = maxPeaksSpinBox.value
                tuner.referenceA = referenceASpinBox.value

                settingsStorage.sampleRate = tuner.sampleRate
                settingsStorage.bufferSize = tuner.bufferSize
                settingsStorage.maxPeaks = tuner.maxPeaks
                settingsStorage.referenceA = tuner.referenceA
                settingsDialog.close()
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
        spacing: 10

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

        Label {
            text: "Reference A Frequency"
            color: "#ffffff"
        }

        RowLayout {
            Layout.fillWidth: true
            SpinBox {
                id: referenceASpinBox
                Layout.fillWidth: true
                from: 400
                to: 500
                value: 440
                stepSize: 1
                editable: true
                Material.foreground: "white"
                Material.background: "#2d2d2d"
            }
            Label {
                text: "Hz"
                color: "#9e9e9e"
            }
        }

        Label {
            text: "Maximum Peaks"
            color: "#ffffff"
        }

        SpinBox {
            id: maxPeaksSpinBox
            Layout.fillWidth: true
            from: 1
            to: 20
            value: 10
            Material.foreground: "white"
            Material.background: "#2d2d2d"
        }

        // Current values display
        ColumnLayout {
            Layout.topMargin: 15
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
                text: "Buffer Size: " + tuner.bufferSize + " samples"
                color: "#9e9e9e"
            }

            Label {
                text: "Latency: " + (tuner.bufferSize / tuner.sampleRate * 1000).toFixed(1) + " ms"
                color: "#9e9e9e"
            }
        }

        // Donation section
        Rectangle {
            Layout.fillWidth: true
            Layout.topMargin: 20
            height: 1
            color: "#3d3d3d"
        }

        Label {
            Layout.fillWidth: true
            text: "Support the Development"
            color: "#9e9e9e"
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
        }

        Button {
            Layout.alignment: Qt.AlignHCenter
            text: "Donate with PayPal"
            highlighted: true
            onClicked: {
                console.log("donation path :", TunerStyle.donationLink)
                Qt.openUrlExternally(TunerStyle.donationLink)
        }
        }
    }
} 

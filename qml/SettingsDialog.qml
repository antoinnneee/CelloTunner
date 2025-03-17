import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Controls.Material
import QtCore


Dialog {
    id: settingsDialog
    title: "Settings"
    width: 400
    height: Math.min(600, parent.height * 0.8)
    anchors.centerIn: parent
    modal: true
    standardButtons: Dialog.Ok | Dialog.Cancel

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
        sampleRateSlider.value = settingsStorage.sampleRate
        bufferSizeSlider.value = settingsStorage.bufferSize
        maxPeaksSlider.value = settingsStorage.maxPeaks
        referenceASpinBox.value = settingsStorage.referenceA
        methodComboBox.currentText = tuner.detectionMethod
        fftPaddingSlider.value = tuner.fftPadding
        thresholdSlider.value = tuner.dbThreshold
    }

    onAccepted: {
        tuner.dbThreshold = thresholdSlider.value
        tuner.sampleRate = sampleRateSlider.value
        tuner.bufferSize = bufferSizeSlider.value
        tuner.maxPeaks = maxPeaksSlider.value
        tuner.referenceA = referenceASpinBox.value
        tuner.detectionMethod = methodComboBox.currentText
        tuner.fftPadding = fftPaddingSlider.value
    }

    Flickable {
        id: settingsFlickable
        anchors.fill: parent
        contentHeight: settingsColumn.height
        clip: true
        ScrollBar.vertical: ScrollBar {}

        ColumnLayout {
            id: settingsColumn
            width: parent.width
            spacing: 16

            // Detection Method
            Label {
                text: "Detection Method"
                font.bold: true
            }
            ComboBox {
                id: methodComboBox
                Layout.fillWidth: true
                model: ["FFT", "Autocorrelation"]
                currentIndex: model.indexOf(tuner.detectionMethod)
            }

            // FFT Settings Section
            Label {
                text: "FFT Settings"
                font.bold: true
                visible: methodComboBox.currentText === "FFT"
            }

            // FFT Padding
            Label {
                text: "FFT Padding: " + fftPaddingSlider.value + "x"
                visible: methodComboBox.currentText === "FFT"
            }
            Slider {
                id: fftPaddingSlider
                Layout.fillWidth: true
                from: 1
                to: 8
                stepSize: 1
                value: tuner.fftPadding
                visible: methodComboBox.currentText === "FFT"

                ToolTip {
                    parent: fftPaddingSlider.handle
                    visible: fftPaddingSlider.pressed
                    text: "Resolution: " + (tuner.sampleRate / (tuner.bufferSize * fftPaddingSlider.value)).toFixed(2) + " Hz"
                }
            }

            // Audio Settings Section
            Label {
                text: "Audio Settings"
                font.bold: true
            }

            // Signal Threshold
            Label {
                text: "Signal Threshold: " + thresholdSlider.value.toFixed(1) + " dB"
            }
            Slider {
                id: thresholdSlider
                Layout.fillWidth: true
                from: -90
                to: -20
                value: tuner.dbThreshold
            }

            // Sample Rate
            Label {
                text: "Sample Rate: " + sampleRateSlider.value + " Hz"
            }
            Slider {
                id: sampleRateSlider
                Layout.fillWidth: true
                from: 8000
                to: tuner.maximumSampleRate
                stepSize: 100
                value: tuner.sampleRate
            }

            // Buffer Size
            Label {
                text: "Buffer Size: " + bufferSizeSlider.value + " samples"
            }
            Slider {
                id: bufferSizeSlider
                Layout.fillWidth: true
                from: 1024
                to: 16384
                stepSize: 1024
                value: tuner.bufferSize
            }

            // Visualization Settings Section
            Label {
                text: "Visualization Settings"
                font.bold: true
            }

            // Max Peaks
            Label {
                text: "Maximum Peaks: " + maxPeaksSlider.value
            }
            Slider {
                id: maxPeaksSlider
                Layout.fillWidth: true
                from: 1
                to: 20
                stepSize: 1
                value: tuner.maxPeaks
            }

            // Tuning Settings Section
            Label {
                text: "Tuning Settings"
                font.bold: true
            }

            // Reference A4
            Label {
                text: "Reference A4: " + referenceASpinBox.value.toFixed(1) + " Hz"
            }
            SpinBox {
                id: referenceASpinBox
                Layout.fillWidth: true
                from: 430
                to: 450
                stepSize: 1
                value: tuner.referenceA
                editable: true
                validator: DoubleValidator {
                    bottom: referenceASpinBox.from
                    top: referenceASpinBox.to
                    decimals: 1
                }
            }

            // Info Section
            Label {
                text: "Information"
                font.bold: true
            }

            Label {
                text: "Maximum supported sample rate: " + tuner.maximumSampleRate + " Hz"
                font.italic: true
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }

            Label {
                text: "Current frequency resolution: " + 
                      (methodComboBox.currentText === "FFT" ? 
                      (tuner.sampleRate / (tuner.bufferSize * fftPaddingSlider.value)).toFixed(2) :
                      (tuner.sampleRate / tuner.bufferSize).toFixed(2)) + " Hz"
                font.italic: true
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }
        }
    }

    footer: DialogButtonBox {
        Button {
            text: qsTr("Apply")
            DialogButtonBox.buttonRole: DialogButtonBox.ApplyRole
            highlighted: true
            onClicked: {
                // Apply and save settings
                tuner.sampleRate = parseInt(sampleRateSlider.value)
                tuner.bufferSize = parseInt(bufferSizeSlider.value)
                tuner.maxPeaks = maxPeaksSlider.value
                tuner.referenceA = referenceASpinBox.value
                tuner.detectionMethod = methodComboBox.currentText
                tuner.fftPadding = fftPaddingSlider.value

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

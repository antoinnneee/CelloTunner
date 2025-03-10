import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCore

Dialog {
    id: donationDialog
    title: "Support the Development"
    modal: true
    width: 400

    // Settings storage for launch counter
    Settings {
        id: launchSettings
        property int launchCount: 0
    }

    Component.onCompleted: {
        console.log("launchCount :", launchSettings.launchCount)
        launchSettings.launchCount++
        if (launchSettings.launchCount === 2 || launchSettings.launchCount % 50 === 0) {
            open()
        }
    }

    anchors.centerIn: parent

    contentItem: ColumnLayout {
        spacing: 20
        width: parent.width

        Label {
            Layout.fillWidth: true
            text: "Thank you for using Cello Tuner!"
            font.pixelSize: 18
            font.bold: true
            color: "#ffffff"
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }

        Label {
            Layout.fillWidth: true
            text: "This app will always remain free and without ads and openSource. If you find it useful and would like to support its development, consider making a small donation."
            color: "#ffffff"
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }

        Button {
            Layout.alignment: Qt.AlignHCenter
            text: "Donate with PayPal"
            highlighted: true
            onClicked: Qt.openUrlExternally(TunerStyle.donationLink)
        }
    }

    footer: DialogButtonBox {
        Button {
            text: "Maybe Later"
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            onClicked: donationDialog.close()
        }
    }
} 

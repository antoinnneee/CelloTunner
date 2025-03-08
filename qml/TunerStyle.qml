pragma Singleton
import QtQuick

QtObject {
    readonly property color backgroundColor: "#1a1a1a"
    readonly property color accentColor: "#9C27B0"
    readonly property color inTuneColor: "#4CAF50"
    readonly property color warningColor: "#FFC107"
    readonly property color errorColor: "#FF5722"
    readonly property color textColor: "#ffffff"
    readonly property color secondaryTextColor: "#9e9e9e"
    
    readonly property int largeTextSize: 72
    readonly property int mediumTextSize: 24
    readonly property int smallTextSize: 18
    
    readonly property int tuningThreshold: 5  // cents
    readonly property int warningThreshold: 15  // cents
} 
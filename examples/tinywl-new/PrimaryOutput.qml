// Copyright (C) 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import Waylib.Server

OutputItem {
    id: rootOutputItem
    readonly property OutputViewport onscreenViewport: outputViewport
    readonly property alias keepAllOutputRotation: rotationAllOutputsOption.checked

    devicePixelRatio: output?.scale ?? devicePixelRatio

    cursorDelegate: Cursor {
        id: cursorItem

        required property QtObject outputCurosr
        readonly property point position: parent.mapFromGlobal(cursor.position.x, cursor.position.y)

        cursor: outputCurosr.cursor
        output: outputCurosr.output.output
        x: position.x - hotSpot.x
        y: position.y - hotSpot.y
        visible: valid && outputCurosr.visible
        OutputLayer.enabled: true
        OutputLayer.keepLayer: true
        OutputLayer.outputs: [onscreenViewport]
        OutputLayer.flags: OutputLayer.Cursor
        OutputLayer.cursorHotSpot: hotSpot
    }

    OutputViewport {
        id: outputViewport

        output: rootOutputItem.output
        devicePixelRatio: parent.devicePixelRatio
        anchors.centerIn: parent

        RotationAnimation {
            id: rotationAnimator

            target: outputViewport
            duration: 200
            alwaysRunToEnd: true
        }

        Timer {
            id: setTransform

            property var scheduleTransform
            onTriggered: onscreenViewport.rotateOutput(scheduleTransform)
            interval: rotationAnimator.duration / 2
        }

        function rotationOutput(orientation) {
            setTransform.scheduleTransform = orientation
            setTransform.start()

            switch(orientation) {
            case WaylandOutput.R90:
                rotationAnimator.to = 90
                break
            case WaylandOutput.R180:
                rotationAnimator.to = 180
                break
            case WaylandOutput.R270:
                rotationAnimator.to = -90
                break
            default:
                rotationAnimator.to = 0
                break
            }

            rotationAnimator.from = rotation
            rotationAnimator.start()
        }
    }

    Wallpaper {
        id: background
        userId: 1000
        output: rootOutputItem.output
        workspace: Helper.workspace.currentIndex

        fillMode: Image.Tile
        cache: false
        asynchronous: true
        sourceSize: Qt.size(1920, 1080)
        anchors.fill: parent
    }

    Column {
        anchors {
            bottom: parent.bottom
            right: parent.right
            margins: 10
        }

        spacing: 10

        Switch {
            id: rotationAllOutputsOption
            text: "Rotation All Outputs"
        }

        Button {
            text: "1X"
            onClicked: {
                onscreenViewport.setOutputScale(1)
            }
        }

        Button {
            text: "1.5X"
            onClicked: {
                onscreenViewport.setOutputScale(1.5)
            }
        }

        Button {
            text: "Normal"
            onClicked: {
                outputViewport.rotationOutput(WaylandOutput.Normal)
            }
        }

        Button {
            text: "R90"
            onClicked: {
                outputViewport.rotationOutput(WaylandOutput.R90)
            }
        }

        Button {
            text: "R270"
            onClicked: {
                outputViewport.rotationOutput(WaylandOutput.R270)
            }
        }

        Button {
            text: "Quit"
            onClicked: {
                Qt.quit()
            }
        }
    }

    Text {
        anchors.centerIn: parent
        text: "'Ctrl+Q' quit"
        font.pointSize: 40
        color: "white"

        SequentialAnimation on rotation {
            id: ani
            running: true
            PauseAnimation { duration: 1500 }
            NumberAnimation { from: 0; to: 360; duration: 5000; easing.type: Easing.InOutCubic }
            loops: Animation.Infinite
        }
    }

    function setTransform(transform) {
        onscreenViewport.rotationOutput(transform)
    }

    function setScale(scale) {
        onscreenViewport.setOutputScale(scale)
    }

    function invalidate() {
        onscreenViewport.invalidate()
    }
}

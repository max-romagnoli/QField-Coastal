/***************************************************************************
                            PlantIdentifyCapture.qml
                              -------------------
              begin                : January 2025
              copyright            : QField Coastal (C) 2025
              email                : maxxromagnoli (at) gmail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts
import org.qfield
import org.qgis
import com.maxxr.qfieldcoastal
import Theme

Popup {
    id: plantIdentifyCapture
    width: parent.width * 0.85
    height: parent.height * 0.85
    modal: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    anchors.centerIn: parent

    property var identificationData: null

    property var fileResourceSource: null

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        // header
        Label {
            text: qsTr("Plant Recogniser")
            font.pointSize: 18
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
        }

        // status
        Label {
            id: statusLabel
            text: qsTr("No identification yet.")
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
        }

        // results
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: identificationData !== null

            ColumnLayout {
                id: resultsColumn
                anchors.fill: parent
                spacing: 8
                anchors.margins: 8

                Repeater {
                    model: identificationData && identificationData.results
                           ? identificationData.results
                           : []

                    delegate: Frame {
                        Layout.fillWidth: true
                        property bool isFirst: index === 0

                        background: Rectangle {
                            color: isFirst ? Theme.accentLightColor : transparent
                            border.color: Theme.accentLightColor
                            radius: 6
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.preferredWidth: parent.width
                            spacing: 6
                            anchors.margins: 6

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 20

                                Loader {
                                    sourceComponent: pieChartComponent
                                    onLoaded: {
                                        item.score = modelData && modelData.score ? modelData.score : 0.0;
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 4

                                    // scientific name
                                    Label {
                                        wrapMode: Text.WordWrap
                                        font.pointSize: 11
                                        font.bold: true
                                        text: {
                                            let row = modelData
                                            if (!row) return qsTr("No row data")

                                            let sp = row.species
                                            if (!sp) return qsTr("No species info")

                                            let sn = sp.scientificName ? sp.scientificName : qsTr("Unknown")
                                            return sn
                                        }
                                    }

                                    // common names label
                                    Label {
                                        Layout.fillWidth: true
                                        wrapMode: Text.WordWrap
                                        font.pointSize: 9
                                        font.bold: true
                                        color: Theme.darkGray
                                        text: {
                                            if (modelData.species.commonNames 
                                                    && modelData.species.commonNames.length > 0) {
                                                return qsTr("Common Names")
                                            }
                                            return ""
                                        }
                                    }

                                    // common names
                                    Repeater {
                                        model: modelData && modelData.species
                                               && modelData.species.commonNames
                                               ? modelData.species.commonNames
                                               : []
                                        delegate: Label {
                                            font.pointSize: 9
                                            Layout.fillWidth: true
                                            wrapMode: Text.WordWrap
                                            text: "• " + modelData
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // footer buttons
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 20

            QfToolButton {
                id: cameraButton
                iconSource: Theme.getThemeVectorIcon("ic_camera_photo_black_24dp")
                text: qsTr("Identify Photo")
                onClicked: capturePhoto()
            }

            QfToolButton {
                id: fileButton
                iconSource: Theme.getThemeVectorIcon("ic_file_black_24dp")
                text: qsTr("Attach file")
                visible: platformUtilities.capabilities & PlatformUtilities.FilePicker
                onClicked: attachFile()
            }
        }
    }


    Component {
        id: pieChartComponent
        Item {
            id: pieChart
            property real score: 0.0  // Score range: 0 to 1
            width: 60
            height: 60

            Canvas {
                id: chartCanvas
                anchors.fill: parent

                onPaint: {
                    var ctx = getContext("2d");
                    var w = width;
                    var h = height;
                    var centerX = w / 2;
                    var centerY = h / 2;
                    var radius = Math.min(w, h) / 2;
                    var innerRadius = radius * 0.6; 

                    ctx.beginPath();
                    ctx.arc(centerX, centerY, radius, 0, 2 * Math.PI, false);
                    ctx.fillStyle = "#eeeeee";
                    ctx.fill();

                    var portion = Math.max(0, Math.min(1, pieChart.score)); 
                    var startAngle = -Math.PI / 2;
                    var endAngle = startAngle + 2 * Math.PI * portion;

                    ctx.beginPath();
                    ctx.moveTo(centerX, centerY);
                    ctx.arc(centerX, centerY, radius, startAngle, endAngle, false);
                    ctx.fillStyle = Theme.mainColor;
                    ctx.fill();

                    // cut center
                    ctx.beginPath();
                    ctx.arc(centerX, centerY, innerRadius, 0, 2 * Math.PI, false);
                    ctx.fillStyle = "#ffffff";
                    ctx.fill();
                }
            }

            // precision percentage
            Label {
                anchors.centerIn: parent
                text: (pieChart.score * 100).toFixed(0) + "%"
                font.bold: true
                font.pointSize: 10
                color: "#333333"
            }

            onScoreChanged: chartCanvas.requestPaint()
        }
}


    Loader {
        id: cameraLoader
        active: false
        sourceComponent: cameraComponent
    }

    Component {
        id: cameraComponent
        QFieldCamera {
            id: qfieldCamera
            visible: false
            Component.onCompleted: {
                qfieldCamera.state = "PhotoCapture"
                open()
            }
            onFinished: function(path) {
                identifyWithPlantNet(path)
                close()
            }
            onCanceled: close()
            onClosed: cameraLoader.active = false
        }
    }

    Connections {
        target: fileResourceSource
        function onResourceReceived(path) {
            statusLabel.text = qsTr("File attached: ") + path
            identifyWithPlantNet(path)
        }
        function onResourceCanceled(message) {
            statusLabel.text = qsTr("File picking canceled.")
        }
    }

    Connections {
        target: scssConnection
        onPlantIdentificationSuccess: function(results) {
            statusLabel.text = qsTr("Success. See results below.")
            identificationData = results
        }
        onPlantIdentificationFailed: function(reason) {
            statusLabel.text = qsTr("Identification Failed: ") + reason
            identificationData = null
        }
    }

    function identifyWithPlantNet(imagePath) {
        statusLabel.text = qsTr("Identifying from ") + imagePath
        identificationData = null
        scssConnection.identifyPlant(imagePath, "", "all")  // TODO:
    }

    function capturePhoto() {
        cameraLoader.active = true
    }

    function attachFile() {
        platformUtilities.requestStoragePermission()
        fileResourceSource = platformUtilities.getFile("", "temporaryName.ext", 0, this)
    }
}
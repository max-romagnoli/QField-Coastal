/***************************************************************************
                            PlantGuide.qml
                              -------------------
              begin                : January 2025
              copyright            : QField Coastal (C) 2025 by max-romagnoli
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

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Theme

Popup {
    id: plantGuidePopup
    width: parent.width * 0.85
    height: parent.height * 0.85
    modal: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    anchors.centerIn: parent

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        // Header Section
        Label {
            text: qsTr("Coastal Vegetation Guide")
            font.pixelSize: 22
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            Column {
                width: parent.width
                spacing: 20

                Text {
                    text: qsTr("Non-invasive Species")
                    font.pixelSize: 18
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    width: parent.width
                }

                Repeater {
                    model: [
                        { imageSource: "qrc:/images/plant-images/zostera.png", description: "Seagrass (Zostera): Soft grass with roots. Z marina (1) long blades. Z Noltii (2) fine lying on mudflats like thin wet lawn." },
                        { imageSource: "qrc:/images/plant-images/glasswort.png", description: "Glasswort (Salicornia) : Green waxy or red-orange in Autumn." },
                        { imageSource: "qrc:/images/plant-images/cordgrass.png", description: "Cordgrass (Spartina): Forms dense upright clumps or fields." },
                        { imageSource: "qrc:/images/plant-images/green_seaweed.png", description: "Green Seaweed: Mainly Ulvas. Tissue tube or thread-like. Attached by holdfast. No roots." },
                        { imageSource: "qrc:/images/plant-images/brownred_seaweed.png", description: "Brown or Read Seaweed: Many species and many forms on rocks and other seaweeds." }
                    ]

                    delegate: RowLayout {
                        spacing: 10
                        width: parent.width

                        Image {
                            source: modelData.imageSource
                            width: parent.width * 0.45
                            height: 100
                            fillMode: Image.PreserveAspectFit
                        }

                        Label {
                            text: modelData.description
                            font.pixelSize: 14
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }
                }

                Text {
                    text: qsTr("Invasive Species")
                    font.pixelSize: 18
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    width: parent.width
                }

                Repeater {
                    model: [
                        { imageSource: "qrc:/images/plant-images/jap_seaweed.png", description: "Japanese Seaweed: Brown/yellowish long thin main stem seaweed with even side branches. Pin shape & size vesicles on stems. Growth up to 10 cm/day. Branches die back in autumn. Dried > black." },
                        { imageSource: "qrc:/images/plant-images/hogweed.png", description: "Giant Hogweed (Check downstream!): Huge hogweed with thick hairy reddish stem, carrying umbrella-shaped seed heads. Can grow up to 5m. DON’T TOUCH gives very nasty burn." },
                        { imageSource: "qrc:/images/plant-images/him_balsam.png", description: "Himalayan Balsam (Check downstream!): Many white or pink trumpet-shaped flowers on bushy 1-1.6m high annual. Can form dense stands or bands marking top of flood water. Seed pods explode when ripe." },
                        { imageSource: "qrc:/images/plant-images/jap_knotweed.png", description: "Japanese Knotweed: Dense stands, hollow stems with distinct raised nodes (like bamboo), with leaves shaped like those of garden beans. Long clusters of small beige flowers from late summer." },
                        { imageSource: "qrc:/images/plant-images/rubharb.png", description: "Giant Rubharb: Up to 2m. Thick stems with hooked bristles. Massive leathery umbrella-shaped toothed leaves. Produces tiny red or orange seeds." },
                        { imageSource: "qrc:/images/plant-images/nz_flax.png", description: "New Zealand Flax: Leathery, dark grey-green, strap-shaped 1-3m leaves. Evergreen perennial. Up to 5m long flowering stems, with dull red flowers. Large seedpods with black shiny seeds." },
                        { imageSource: "qrc:/images/plant-images/sea_buckthorn.png", description: "Sea Buckthorn: Deciduous shrubs, up to 6m high, dense with stiff branches and very thorny. Leaves pale silvery-green, 3–8 cm long. Bright orange edible berries in autumn." }
                    ]

                    delegate: RowLayout {
                        spacing: 10
                        width: parent.width

                        Image {
                            source: modelData.imageSource
                            width: parent.width * 0.45
                            height: 100
                            fillMode: Image.PreserveAspectFit
                        }

                        Label {
                            text: modelData.description
                            font.pixelSize: 14
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }
                }
            }
        }

        QfButton {
            text: qsTr("Close")
            Layout.alignment: Qt.AlignHCenter
            onClicked: plantGuidePopup.close()
        }
    }
}
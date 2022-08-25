import QtQuick 2.14
import QtQuick.Controls 2.14
import QtQuick.Layouts 1.14

import org.qgis 1.0
import org.qfield 1.0
import Theme 1.0

import '.'

Item {
  id: tracking

  property var track: model

  Component.onCompleted: {
    featureModel.resetAttributes()
    embeddedFeatureForm.state = 'Add'
    trackInformationDialog.active = true
  }

  RubberbandModel {
    id: rubberbandModel
    frozen: false
    vectorLayer: track.vectorLayer
    currentCoordinate: positionSource.projectedPosition
    measureValue: ( positionSource.positionInformation.utcDateTime - track.startPositionTimestamp ) / 1000
    currentPositionTimestamp: positionSource.positionInformation.utcDateTime
    crs: mapCanvas.mapSettings.destinationCrs

    onVertexCountChanged: {
      if (vertexCount == 0) {
        return;
      }

      if (geometryType === QgsWkbTypes.PointGeometry) {
        featureModel.applyGeometry()
        featureModel.create();
        featureModel.resetFeatureId();
      } else {
        if ((geometryType === QgsWkbTypes.LineGeometry && vertexCount > 2) ||
            (geometryType === QgsWkbTypes.PolygonGeometry &&vertexCount > 3))
        {
          featureModel.applyGeometry()

          if ((geometryType === QgsWkbTypes.LineGeometry && vertexCount == 3) ||
              (geometryType === QgsWkbTypes.PolygonGeometry && vertexCount == 4))
          {
            // indirect action, no need to check for success and display a toast, the log is enough
            featureModel.create()
            track.feature = featureModel.feature
          }
          else
          {
            // indirect action, no need to check for success and display a toast, the log is enough
            featureModel.save()
          }
        }
      }
    }
  }

  Rubberband {
    id: rubberband
    anchors.fill: parent
    visible: track.visible

    lineWidth: 4
    color: Qt.rgba(Math.random(),Math.random(),Math.random(),0.6);

    mapSettings: mapCanvas.mapSettings
    model: rubberbandModel
  }

  FeatureModel {
    id: featureModel
    project: qgisProject
    currentLayer: track.vectorLayer

    geometry: Geometry {
      id: featureModelGeometry
      rubberbandModel: rubberbandModel
      vectorLayer: track.vectorLayer
    }

    positionInformation: coordinateLocator.positionInformation
    positionLocked: coordinateLocator.overrideLocation !== undefined
    cloudUserInformation: cloudConnection.userInformation
  }


  // Feature form to set attributes
  AttributeFormModel {
    id: embeddedAttributeFormModel
    featureModel: featureModel
  }

  Loader {
    id: embeddedFeatureForm

    sourceComponent: embeddedFeatureFormComponent
    active: false
    onLoaded: {
      item.open()
    }
  }

  Component {
    id: embeddedFeatureFormComponent

    Popup {
      id: embeddedFeatureFormPopup
      parent: ApplicationWindow.overlay

      x: Theme.popupScreenEdgeMargin
      y: Theme.popupScreenEdgeMargin
      padding: 0
      width: parent.width - Theme.popupScreenEdgeMargin * 2
      height: parent.height - Theme.popupScreenEdgeMargin * 2
      modal: true
      closePolicy: Popup.CloseOnEscape

      FeatureForm {
        id: form
        model: embeddedAttributeFormModel

        focus: true
        setupOnly: true
        embedded: true
        toolbarVisible: true

        anchors.fill: parent

        state: 'Add'

        onTemporaryStored: {
          embeddedFeatureForm.active = false
          trackingModel.startTracker(track.vectorLayer)
          displayToast(qsTr('Track on layer %1 started').arg(track.vectorLayer.name))
        }

        onCancelled: {
          embeddedFeatureForm.active = false
          embeddedFeatureForm.focus = false
          trackingModel.stopTracker(track.vectorLayer)
        }
      }

      onClosed: {
        form.confirm()
      }
    }
  }


  // Dialog to set tracker properties
  Loader {
    id: trackInformationDialog

    sourceComponent: trackInformationDialogComponent
    active: false
    onLoaded: {
      item.open()
    }
  }

  Component {
    id: trackInformationDialogComponent

    Popup {
      id: trackInformationPopup
      parent: ApplicationWindow.overlay

      x: Theme.popupScreenEdgeMargin
      y: Theme.popupScreenEdgeMargin
      padding: 0
      width: parent.width - Theme.popupScreenEdgeMargin * 2
      height: parent.height - Theme.popupScreenEdgeMargin * 2
      modal: true
      closePolicy: Popup.CloseOnEscape

      Page {
        focus: true
        anchors.fill: parent

        header: PageHeader {
          title: qsTr("Tracker Settings")

          showApplyButton: false
          showCancelButton: false
          showBackButton: true

          onBack: {
            trackInformationDialog.active = false
            trackingModel.stopTracker(track.vectorLayer)
          }
        }

        ScrollView {
          padding: 20
          ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
          ScrollBar.vertical.policy: ScrollBar.AsNeeded
          contentWidth: trackerSettingsGrid.width
          contentHeight: trackerSettingsGrid.height
          anchors.fill: parent
          clip: true

          GridLayout {
            id: trackerSettingsGrid
            width: parent.parent.width
            Layout.fillWidth: true

            columns: 2
            columnSpacing: 0
            rowSpacing: 5


            Label {
              text: qsTr("Activate time constraint")
              font: Theme.defaultFont
              wrapMode: Text.WordWrap
              Layout.fillWidth: true

              MouseArea {
                anchors.fill: parent
                onClicked: timeInterval.toggle()
              }
            }

            QfSwitch {
              id: timeInterval
              Layout.preferredWidth: implicitContentWidth
              Layout.alignment: Qt.AlignTop
              checked: positioningSettings.trackerTimeIntervalConstraint
              onCheckedChanged: {
                positioningSettings.trackerTimeIntervalConstraint = checked
              }
            }

            Label {
              text: qsTr("Minimum time [sec]")
              font: Theme.defaultFont
              wrapMode: Text.WordWrap
              enabled: timeInterval.checked
              visible: timeInterval.checked
              Layout.leftMargin: 8
              Layout.fillWidth: true
            }

            TextField {
              id: timeIntervalValue
              width: timeInterval.width
              font: Theme.defaultFont
              enabled: timeInterval.checked
              visible: timeInterval.checked
              horizontalAlignment: TextInput.AlignHCenter
              Layout.preferredWidth: 60
              Layout.preferredHeight: font.height + 20

              inputMethodHints: Qt.ImhFormattedNumbersOnly
              validator: DoubleValidator { locale: 'C' }

              background: Rectangle {
                y: parent.height - height - parent.bottomPadding / 2
                implicitWidth: 120
                height: parent.activeFocus ? 2: 1
                color: parent.activeFocus ? Theme.accentColor : Theme.accentLightColor
              }

              Component.onCompleted: {
                text = isNaN(positioningSettings.trackerTimeInterval) ? '' : positioningSettings.trackerTimeInterval
              }

              onTextChanged: {
                if( text.length === 0 || isNaN(text) ) {
                  positioningSettings.trackerTimeInterval = NaN
                } else {
                  positioningSettings.trackerTimeInterval = parseFloat( text )
                }
              }
            }

            Label {
              text: qsTr("Activate distance constraint")
              font: Theme.defaultFont
              wrapMode: Text.WordWrap
              Layout.fillWidth: true

              MouseArea {
                anchors.fill: parent
                onClicked: minimumDistance.toggle()
              }
            }

            QfSwitch {
              id: minimumDistance
              Layout.preferredWidth: implicitContentWidth
              Layout.alignment: Qt.AlignTop
              checked: positioningSettings.trackerMinimumDistanceConstraint
              onCheckedChanged: {
                positioningSettings.trackerMinimumDistanceConstraint = checked
              }
            }

            DistanceArea {
              id: infoDistanceArea
              property VectorLayer currentLayer: track.vectorLayer
              project: qgisProject
              crs: qgisProject.crs
            }

            Label {
              text: qsTr("Minimum distance [%1]").arg( UnitTypes.toAbbreviatedString( infoDistanceArea.lengthUnits ) )
              font: Theme.defaultFont
              wrapMode: Text.WordWrap
              enabled: minimumDistance.checked
              visible: minimumDistance.checked
              Layout.leftMargin: 8
              Layout.fillWidth: true
            }

            TextField {
              id: minimumDistanceValue
              width: minimumDistance.width
              font: Theme.defaultFont
              enabled: minimumDistance.checked
              visible: minimumDistance.checked
              horizontalAlignment: TextInput.AlignHCenter
              Layout.preferredWidth: 60
              Layout.preferredHeight: font.height + 20

              inputMethodHints: Qt.ImhFormattedNumbersOnly
              validator: DoubleValidator { locale: 'C' }

              background: Rectangle {
                y: parent.height - height - parent.bottomPadding / 2
                implicitWidth: 120
                height: parent.activeFocus ? 2: 1
                color: parent.activeFocus ? Theme.accentColor : Theme.accentLightColor
              }

              Component.onCompleted: {
                text = isNaN(positioningSettings.trackerMinimumDistance) ? '' : positioningSettings.trackerMinimumDistance
              }

              onTextChanged: {
                if( text.length === 0 || isNaN(text) ) {
                  positioningSettings.trackerMinimumDistance = NaN
                } else {
                  positioningSettings.trackerMinimumDistance = parseFloat( text )
                }
              }
            }

            Label {
              text: qsTr("Record when both active constraints are met")
              font: Theme.defaultFont
              wrapMode: Text.WordWrap
              Layout.fillWidth: true
              enabled: timeInterval.checked && minimumDistance.checked
              visible: timeInterval.checked && minimumDistance.checked

              MouseArea {
                anchors.fill: parent
                onClicked: allConstraints.toggle()
              }
            }

            QfSwitch {
              id: allConstraints
              Layout.preferredWidth: implicitContentWidth
              Layout.alignment: Qt.AlignTop
              enabled: timeInterval.checked && minimumDistance.checked
              visible: timeInterval.checked && minimumDistance.checked
              checked: positioningSettings.trackerMeetAllConstraints
              onCheckedChanged: {
                positioningSettings.trackerMeetAllConstraints = checked
              }
            }


            Label {
              text: qsTr( "When enabled, vertices with only be recorded when both active constraints are met. If the setting is disabled, individual constraints met will trigger a vertex addition." )
              font: Theme.tipFont
              color: Theme.gray
              textFormat: Qt.RichText
              wrapMode: Text.WordWrap
              Layout.fillWidth: true
              enabled: timeInterval.checked && minimumDistance.checked
              visible: timeInterval.checked && minimumDistance.checked
            }

            Item {
              Layout.preferredWidth: allConstraints.width
            }

            QfButton {
              id: trackingButton
              Layout.topMargin: 8
              Layout.fillWidth: true
              Layout.columnSpan: 2
              font: Theme.defaultFont
              text: qsTr( "Start tracking")
              icon.source: Theme.getThemeVectorIcon( 'directions_walk_24dp' )

              onClicked: {
                if (Number(timeIntervalValue.text) + Number(minimumDistanceValue.text) === 0 ||
                    (timeInterval.checked && minimumDistance.checked && allConstraints.checked &&
                     (Number(timeIntervalValue.text) === 0 || Number(minimumDistanceValue.text) === 0)) ||
                    (!timeInterval.checked && !minimumDistance.checked))
                {
                  displayToast(qsTr( 'Cannot start track with both constaints switched off'), 'warning')
                }
                else
                {
                  track.timeInterval = timeIntervalValue.text.length == 0 || !timeInterval.checked ? 0 : timeIntervalValue.text
                  track.minimumDistance = minimumDistanceValue.text.length == 0 || !minimumDistance.checked ? 0 : minimumDistanceValue.text
                  track.conjunction = timeInterval.checked && minimumDistance.checked && allConstraints.checked
                  track.rubberModel = rubberbandModel

                  trackInformationDialog.active = false
                  embeddedFeatureForm.active = true
                }
              }
            }

            Item {
              // spacer item
              Layout.fillWidth: true
              Layout.fillHeight: true
            }
          }
        }
      }
    }
  }
}

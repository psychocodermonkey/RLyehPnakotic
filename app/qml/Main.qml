// SPDX-FileCopyrightText: 2026 A.D. (PsychoCoderMonkey) <andrew.dixon@rlyeh.dev>
// SPDX-License-Identifier: GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
  id: window

  required property var controller

  width: 1100
  height: 880
  minimumWidth: 900
  minimumHeight: 720
  visible: true
  title: qsTr("R'Lyeh Pnakotic")

  function showDecodeResult(result) {
    decodeValid = result.valid
    decodeStatus = result.status
    decodedDate.text = result.valid ? result.commitDate : ""
    decodedPrefix.text = result.valid ? result.commitPrefix : ""
  }

  function showEncodeResult(result) {
    encodeValid = result.valid
    encodeStatus = result.status
    encodedVersion.text = result.valid ? result.version : ""
  }

  property bool decodeValid: false
  property bool encodeValid: false
  property string decodeStatus: ""
  property string encodeStatus: ""

  header: Pane {
    padding: 18

    ColumnLayout {
      anchors.fill: parent
      spacing: 2

      Label {
        Layout.alignment: Qt.AlignHCenter
        text: qsTr("R'Lyeh Pnakotic")
        font.pixelSize: 30
      }

      Label {
        Layout.alignment: Qt.AlignHCenter
        text: qsTr("Version encoded. Truth retrievable.")
        font.pixelSize: 14
      }
    }
  }

  RowLayout {
    anchors.fill: parent
    anchors.margins: 22
    spacing: 24

    Frame {
      Layout.fillHeight: true
      Layout.preferredWidth: Math.max(420, window.width * 0.48)
      padding: 20

      ColumnLayout {
        anchors.fill: parent
        spacing: 14

        TabBar {
          id: modeTabs
          Layout.fillWidth: true

          TabButton {
            text: qsTr("Decode")
          }

          TabButton {
            text: qsTr("Encode")
          }

          onCurrentIndexChanged: {
            if (currentIndex === 0)
              Qt.callLater(decodeVersion.forceActiveFocus)
            else
              Qt.callLater(encodeDate.forceActiveFocus)
          }
        }

        Label {
          text: qsTr("Project")
        }

        ComboBox {
          id: projectBox
          Layout.fillWidth: true
          model: window.controller.projects
          KeyNavigation.tab: modeTabs.currentIndex === 0 ? decodeVersion : encodeDate
        }

        StackLayout {
          Layout.fillWidth: true
          Layout.fillHeight: true
          currentIndex: modeTabs.currentIndex

          Item {
            ColumnLayout {
              anchors.fill: parent
              spacing: 14

              Label {
                text: qsTr("Version (X.Y.Z.W)")
              }

              RowLayout {
                Layout.fillWidth: true

                TextField {
                  id: decodeVersion
                  Layout.fillWidth: true
                  placeholderText: qsTr("141.-422.-5.-149")
                  selectByMouse: true
                  KeyNavigation.backtab: projectBox
                  KeyNavigation.tab: decodeButton
                  onAccepted: decodeButton.clicked()
                }

                Button {
                  id: decodeButton
                  text: qsTr("Decode")
                  KeyNavigation.backtab: decodeVersion
                  KeyNavigation.tab: decodedDate
                  onClicked: window.showDecodeResult(
                    window.controller.decode(projectBox.currentIndex, decodeVersion.text))
                }
              }

              Label {
                text: qsTr("Decoded information")
                font.pixelSize: 18
                topPadding: 10
              }

              Label {
                text: qsTr("Commit date")
              }

              TextField {
                id: decodedDate
                Layout.fillWidth: true
                readOnly: true
                selectByMouse: true
                KeyNavigation.backtab: decodeButton
                KeyNavigation.tab: decodedPrefix
              }

              Label {
                text: qsTr("Commit hash prefix (first 6 hex)")
              }

              TextField {
                id: decodedPrefix
                Layout.fillWidth: true
                readOnly: true
                selectByMouse: true
                KeyNavigation.backtab: decodedDate
                KeyNavigation.tab: modeTabs
              }

              Label {
                Layout.fillWidth: true
                text: window.decodeStatus
                color: window.decodeStatus === "" ? palette.text
                                                   : window.decodeValid ? "#2e7d32" : "#b3261e"
                wrapMode: Text.WordWrap
              }

              Item {
                Layout.fillHeight: true
              }
            }
          }

          Item {
            ColumnLayout {
              anchors.fill: parent
              spacing: 14

              Label {
                text: qsTr("Commit date (YYYY-MM-DD)")
              }

              TextField {
                id: encodeDate
                Layout.fillWidth: true
                placeholderText: qsTr("2025-08-13")
                selectByMouse: true
                KeyNavigation.backtab: projectBox
                KeyNavigation.tab: encodeHash
                onAccepted: encodeHash.forceActiveFocus()
              }

              Label {
                text: qsTr("Full commit hash")
              }

              TextField {
                id: encodeHash
                Layout.fillWidth: true
                placeholderText: qsTr("Enter at least six hexadecimal characters")
                selectByMouse: true
                KeyNavigation.backtab: encodeDate
                KeyNavigation.tab: encodeButton
                onAccepted: encodeButton.clicked()
              }

              Button {
                id: encodeButton
                Layout.alignment: Qt.AlignRight
                text: qsTr("Encode")
                KeyNavigation.backtab: encodeHash
                KeyNavigation.tab: encodedVersion
                onClicked: window.showEncodeResult(
                  window.controller.encode(
                    projectBox.currentIndex, encodeDate.text, encodeHash.text))
              }

              Label {
                text: qsTr("Public version")
                font.pixelSize: 18
                topPadding: 10
              }

              TextField {
                id: encodedVersion
                Layout.fillWidth: true
                readOnly: true
                selectByMouse: true
                KeyNavigation.backtab: encodeButton
                KeyNavigation.tab: modeTabs
              }

              Label {
                Layout.fillWidth: true
                text: window.encodeStatus
                color: window.encodeStatus === "" ? palette.text
                                                   : window.encodeValid ? "#2e7d32" : "#b3261e"
                wrapMode: Text.WordWrap
              }

              Item {
                Layout.fillHeight: true
              }
            }
          }
        }
      }
    }

    Item {
      Layout.fillWidth: true
      Layout.fillHeight: true
      Layout.minimumWidth: 340

      Image {
        anchors.fill: parent
        source: "qrc:/images/vaulketh-cryptex.png"
        fillMode: Image.PreserveAspectFit
        horizontalAlignment: Image.AlignHCenter
        verticalAlignment: Image.AlignBottom
        Accessible.name: qsTr("Vaul'Keth holding the Pnakotic cryptex")
      }
    }
  }

  footer: Pane {
    padding: 12

    RowLayout {
      anchors.fill: parent

      Button {
        text: qsTr("About")
        onClicked: aboutDialog.open()
      }

      Item {
        Layout.fillWidth: true
      }

      ColumnLayout {
        spacing: 0

        Label {
          Layout.alignment: Qt.AlignHCenter
          text: qsTr("R'Lyeh Pnakotic")
        }

        Label {
          Layout.alignment: Qt.AlignHCenter
          text: qsTr("The locator, not the time. · 0.0.0")
          font.pixelSize: 12
        }
      }

      Item {
        Layout.fillWidth: true
      }

      Button {
        text: qsTr("View Specification")
        onClicked: specificationDialog.open()
      }
    }
  }

  Dialog {
    id: aboutDialog
    anchors.centerIn: parent
    width: Math.min(460, window.width - 48)
    modal: true
    title: qsTr("About R'Lyeh Pnakotic")
    standardButtons: Dialog.Ok

    Label {
      width: parent.width
      text: qsTr("A deterministic, reversible public-version codec.\n\n"
                 + "Application version 0.0.0\n"
                 + "Software licensed under GPL-3.0-only. Artwork and fonts retain their respective licenses.")
      wrapMode: Text.WordWrap
    }
  }

  Dialog {
    id: specificationDialog
    anchors.centerIn: parent
    width: Math.min(460, window.width - 48)
    modal: true
    title: qsTr("Version Format Specification")
    standardButtons: Dialog.Ok

    Label {
      width: parent.width
      text: qsTr("Specification viewing will be added in a later documentation pass.")
      wrapMode: Text.WordWrap
    }
  }

  Component.onCompleted: decodeVersion.forceActiveFocus()
}

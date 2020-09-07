/*
 *  Kaidan - A user-friendly XMPP client for every device!
 *
 *  Copyright (C) 2016-2020 Kaidan developers and contributors
 *  (see the LICENSE file for a full list of copyright authors)
 *
 *  Kaidan is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  In addition, as a special exception, the author of Kaidan gives
 *  permission to link the code of its release with the OpenSSL
 *  project's "OpenSSL" library (or with modified versions of it that
 *  use the same license as the "OpenSSL" library), and distribute the
 *  linked executables. You must obey the GNU General Public License in
 *  all respects for all of the code used other than "OpenSSL". If you
 *  modify this file, you may extend this exception to your version of
 *  the file, but you are not obligated to do so.  If you do not wish to
 *  do so, delete this exception statement from your version.
 *
 *  Kaidan is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kaidan.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.12 as Controls
import org.kde.kirigami 2.12 as Kirigami

import im.kaidan.kaidan 1.0
import EmojiModel 0.1
import MediaUtils 0.1

/**
 * This is a pane for writing and sending chat messages.
 */
Controls.Pane {
	id: root
	padding: 6

	background: Kirigami.ShadowedRectangle {
		shadow.color: Qt.darker(color, 1.2)
		shadow.size: 4
		color: Kirigami.Theme.backgroundColor
	}

	property QtObject chatPage
	property alias messageArea: messageArea

	ColumnLayout {
		anchors.fill: parent
		spacing: 0

		RowLayout {
			visible: chatPage.isWritingSpoiler

			Controls.TextArea {
				id: spoilerHintField
				Layout.fillWidth: true
				placeholderText: qsTr("Spoiler hint")
				wrapMode: Controls.TextArea.Wrap
				selectByMouse: true
				background: Item {}
			}

			Controls.ToolButton {
				Layout.preferredWidth: Kirigami.Units.gridUnit * 1.5
				Layout.preferredHeight: Kirigami.Units.gridUnit * 1.5
				padding: 0

				Kirigami.Icon {
					source: "tab-close"
					smooth: true
					anchors.centerIn: parent
					width: Kirigami.Units.gridUnit * 1.5
					height: width
				}

				onClicked: {
					chatPage.isWritingSpoiler = false
					spoilerHintField.text = ""
				}
			}
		}

		Kirigami.Separator {
			visible: chatPage.isWritingSpoiler
			Layout.fillWidth: true
		}

		RowLayout {
			spacing: 0

			// emoji picker button
			ClickableIcon {
				source: "face-smile"
				enabled: sendButton.enabled
				onClicked: emojiPicker.visible ? emojiPicker.close() : emojiPicker.open()
			}

			EmojiPicker {
				id: emojiPicker
				x: - root.padding
				y: - height - root.padding
				width: Kirigami.Units.gridUnit * 20
				height: Kirigami.Units.gridUnit * 15
				textArea: messageArea

				model: EmojiProxyModel {
					group: hasFavoriteEmojis ? Emoji.Group.Favorites : Emoji.Group.People
					sourceModel: EmojiModel {}
				}
			}

			Controls.TextArea {
				id: messageArea
				placeholderText: qsTr("Compose message")
				background: Item {}
				wrapMode: TextEdit.Wrap
				Layout.fillWidth: true
				verticalAlignment: TextEdit.AlignVCenter
				state: "compose"

				states: [
					State {
						name: "compose"
					},

					State {
						name: "edit"
					}
				]

				onStateChanged: {
					if (state === "edit") {
						// Move the cursor to the end of the text being corrected.
						selectAll()
						cursorPosition = selectionEnd
					}
				}

				Keys.onReturnPressed: {
					if (event.key === Qt.Key_Return) {
						if (event.modifiers & (Qt.ControlModifier | Qt.ShiftModifier)) {
							messageArea.append("")
						} else {
							sendMessage()
							event.accepted = true
						}
					}
				}
			}

			// file sharing button
			ClickableIcon {
				source: "document-send-symbolic"
				visible: Kaidan.serverFeaturesCache.httpUploadSupported

				onClicked: {
					if (Kirigami.Settings.isMobile)
						chatPage.mediaDrawer.open()
					else
						chatPage.openFileDialog(qsTr("All files"), "*", MediaUtilsInstance.label(Enums.MessageType.MessageFile))
				}
			}

			ClickableIcon {
				id: sendButton
				source: {
					if (messageArea.state === "compose")
						return "document-send"
					else if (messageArea.state === "edit")
						return "edit-symbolic"
				}

				onClicked: sendMessage()
			}
		}
	}

	Component.onCompleted: {
		// This makes it possible on desktop devices to directly enter a message after opening the chat page.
		// It is not used on mobile devices because the soft keyboard would otherwise always pop up after opening the chat page.
		if (!Kirigami.Settings.isMobile)
			messageArea.forceActiveFocus()
	}

	/**
	 * Sends the text entered in the messageArea.
	 */
	function sendMessage() {
		// Do not send empty messages.
		if (!messageArea.text.length)
			return

		// Disable the button to prevent sending the same message several times.
		sendButton.enabled = false

		// Send the message.
		if (messageArea.state === "compose") {
			Kaidan.sendMessage(
				Kaidan.messageModel.currentChatJid,
				messageArea.text,
				chatPage.isWritingSpoiler,
				spoilerHintField.text
			)
		} else if (messageArea.state === "edit") {
			Kaidan.correctMessage(chatPage.messageToCorrect, messageArea.text)
		}

		clearMessageArea()

		// Enable the button again.
		sendButton.enabled = true

		// Show the cursor even if another element like the sendButton (after
		// clicking on it) was focused before.
		messageArea.forceActiveFocus()
	}

	function clearMessageArea() {
		messageArea.text = ""
		spoilerHintField.text = ""
		chatPage.isWritingSpoiler = false
		chatPage.messageToCorrect = ''
		messageArea.state = "compose"
	}
}
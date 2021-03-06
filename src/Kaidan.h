/*
 *  Kaidan - A user-friendly XMPP client for every device!
 *
 *  Copyright (C) 2016-2021 Kaidan developers and contributors
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

#pragma once

// Qt
#include <QObject>
#include <QString>
// Kaidan
#include "ClientWorker.h"
#include "RegistrationDataFormModel.h"

class QGuiApplication;
class QSize;
class Database;
class QXmppClient;
class QXmppVersionIq;

/**
 * @class Kaidan Kaidan's Back-End Class
 *
 * @brief This class will initiate the complete back-end, including the @see Database
 * connection, viewing models (@see MessageModel, @see RosterModel), etc.
 *
 * This class will run in the main thread, the XMPP connection and the database managers
 * run in other threads.
 */
class Kaidan : public QObject
{
	Q_OBJECT

	Q_PROPERTY(RosterModel* rosterModel READ rosterModel CONSTANT)
	Q_PROPERTY(MessageModel* messageModel READ messageModel CONSTANT)
	Q_PROPERTY(AvatarFileStorage* avatarStorage READ avatarStorage NOTIFY avatarStorageChanged)
	Q_PROPERTY(PresenceCache* presenceCache READ presenceCache CONSTANT)
	Q_PROPERTY(TransferCache* transferCache READ transferCache CONSTANT)
	Q_PROPERTY(ServerFeaturesCache* serverFeaturesCache READ serverFeaturesCache CONSTANT)
	Q_PROPERTY(QSettings* settings READ settings CONSTANT)
	Q_PROPERTY(quint8 connectionState READ connectionState NOTIFY connectionStateChanged)
	Q_PROPERTY(quint8 connectionError READ connectionError NOTIFY connectionErrorChanged)
	Q_PROPERTY(PasswordVisibility passwordVisibility READ passwordVisibility WRITE setPasswordVisibility NOTIFY passwordVisibilityChanged)

public:
	/**
	 * State which specifies in which way a password is shown on the account transfer page
	 */
	enum PasswordVisibility {
		PasswordVisible, ///< The password is included in the QR code and shown as plain text.
		PasswordVisibleQrOnly, ///< The password is included in the QR code but not shown as plain text.
		PasswordInvisible ///< The password is neither included in the QR code nor shown as plain text.
	};
	Q_ENUM(PasswordVisibility)

	static Kaidan *instance();

	/**
	 * Constructs Kaidan's main object and initializes all components / threads.
	 *
	 * @param app GUI application object for managing Kaidan's GUI
	 * @param enableLogging true to enable logging, otherwise false
	 */
	Kaidan(QGuiApplication *app, bool enableLogging = true, QObject *parent = nullptr);

	~Kaidan();

	/**
	 * Returns the application window size stored in the settings file.
	 *
	 * @return the application window size
	 */
	Q_INVOKABLE QSize applicationWindowSize() const;

	/**
	 * Stores the application window size in the settings file to be restored
	 * on the next application start.
	 *
	 * @param size size of the application window
	 */
	Q_INVOKABLE void storeApplicationWindowSize(const QSize &size) const;

	/**
	 * Connects to the XMPP server and logs in to it.
	 *
	 * The username and password are retrieved from the settings file.
	 */
	Q_INVOKABLE void logIn();

	/**
	 * Connects to the server and requests a data form for account registration.
	 */
	Q_INVOKABLE void requestRegistrationForm();

	/**
	 * Logs out of the XMPP server.
	 *
	 * This disconnects the client from the server.
	 * When disconnected, the connectionStateChanged signal is emitted.
	 */
	Q_INVOKABLE void logOut();

	/**
	 * Returns the current ConnectionState
	 */
	quint8 connectionState() const
	{
		return quint8(m_connectionState);
	}

	/**
	 * Returns the last connection error.
	 */
	quint8 connectionError() const;

	/**
	 * Sets the visibility of the password on the account transfer page.
	 */
	void setPasswordVisibility(PasswordVisibility passwordVisibility);

	/**
	 * Returns the visibility of the password on the account transfer page.
	 */
	PasswordVisibility passwordVisibility() const;

	RosterModel* rosterModel() const
	{
		return m_caches->rosterModel;
	}

	MessageModel* messageModel() const
	{
		return m_caches->msgModel;
	}

	AvatarFileStorage* avatarStorage() const
	{
		return m_caches->avatarStorage;
	}

	PresenceCache* presenceCache() const
	{
		return m_caches->presCache;
	}

	TransferCache* transferCache() const
	{
		return m_caches->transferCache;
	}

	ServerFeaturesCache *serverFeaturesCache() const
	{
		return m_caches->serverFeaturesCache;
	}

	QSettings* settings() const
	{
		return m_caches->settings;
	}

	ClientWorker *client() const;

	RosterDb *rosterDb() const;

	MessageDb *messageDb() const;

	/**
	 * Adds XMPP URI to open as soon as possible
	 */
	void addOpenUri(const QString &uri);

	/**
	 * Connects to the server by the parsed credentials (bare JID and password) from a given XMPP URI (e.g. from scanning a QR code) like "xmpp:user@example.org?login;password=abc"
	 *
	 * The URI is used in the following cases.
	 *
	 * Login attempt (LoginByUriState::Connecting is returned):
	 *	xmpp:user@example.org?login;password=abc
	 *
	 * Pre-fill of JID for opening login page (LoginByUriState::PasswordNeeded is returned):
	 *	xmpp:user@example.org?login;password=
	 *	xmpp:user@example.org?login;password
	 *	xmpp:user@example.org?login;
	 *	xmpp:user@example.org?login
	 *	xmpp:user@example.org?
	 *	xmpp:user@example.org
	 *
	 * In all other cases, LoginByUriState::InvalidLoginUri is returned.
	 *
	 * @param uri string which can be an XMPP login URI
	 *
	 * @return the state which specifies how the XMPP login URI was used
	 */
	Q_INVOKABLE quint8 logInByUri(const QString &uri);

signals:
	/**
	 * Emitted when the application window becomes active or inactive.
	 *
	 * The application window is active when it is in the foreground and focused.
	 *
	 * @param active true if the application window becomes active, false otherwise
	 */
	void applicationWindowActiveChanged(bool active);

	/**
	 * Emitted to request a registration form from the server which is set as the
	 * currently used JID.
	 */
	void registrationFormRequested();

	/**
	 * Emitted when a data form for registration is received from the server.
	 *
	 * @param dataFormModel received model for the registration data form
	 */
	void registrationFormReceived(DataFormModel *dataFormModel);

	/**
	 * Emitted to send a completed data form for registration.
	 */
	void sendRegistrationForm();

	/**
	 * Emitted when the account registration failed.
	 *
	 * @param error received error
	 * @param errorMessage message describing the error
	 */
	void registrationFailed(quint8 error, const QString &errorMessage);

	/**
	 * Emitted to log in to the server with the set credentials.
	 */
	void logInRequested();

	/**
	 * Emitted to log out of the server.
	 *
	 * @param isApplicationBeingClosed true if the application will be terminated directly after logging out, false otherwise
	 */
	void logOutRequested(bool isApplicationBeingClosed = false);

	void avatarStorageChanged();

	/**
	 * Emitted, when the client's connection state has changed (e.g. when
	 * successfully connected or when disconnected)
	 */
	void connectionStateChanged();

	/**
	 * Emitted when the connection error changed.
	 *
	 * That is the case when the client failed to connect or it succeeded to connect after an error.
	 */
	void connectionErrorChanged();

	/**
	 * Emitted when there are no (correct) credentials and new ones are needed.
	 *
	 * The client will be in disconnected state when this is emitted.
	 */
	void credentialsNeeded();

	/**
	 * Emitted when an authenticated connection to the server is established with new credentials for the first time.
	 *
	 * The client will be in connected state when this is emitted.
	 */
	void loggedInWithNewCredentials();

	/**
	 * Raises the window to the foreground so that it is on top of all other windows.
	 */
	void raiseWindowRequested();

	/**
	 * Opens the chat page for a given JID.
	 *
	 * @param chatJid JID of the chat for which the chat page is opened
	 */
	void openChatPageRequested(const QString chatJid);

	/**
	 * Emitted when the removal state of the password on the account transfer page changed.
	 */
	void passwordVisibilityChanged();

	/**
	 * Show passive notification
	 */
	void passiveNotificationRequested(QString text);

	/**
	 * Emitted, whan a subscription request was received
	 */
	void subscriptionRequestReceived(QString from, QString msg);

	/**
	 * Incoming subscription request was accepted or declined by the user
	 */
	void subscriptionRequestAnswered(QString jid, bool accepted);

	/**
	 * Request vCard of any JID
	 *
	 * Is required when the avatar (or other information) of a JID are
	 * requested and the JID is not in the roster.
	 */
	void vCardRequested(const QString &jid);

	/**
	 * XMPP URI received
	 *
	 * Is called when Kaidan was used to open an XMPP URI (i.e. 'xmpp:kaidan@muc.kaidan.im?join')
	 */
	void xmppUriReceived(QString uri);

	/**
	 * The upload progress of a file upload has changed
	 */
	void uploadProgressMade(QString msgId, quint64 sent, quint64 total);

	/**
	 * Send a text message to any JID
	 *
	 * Currently only contacts are displayed on the RosterPage (there is no
	 * way to view a list of all chats -> for contacts and non-contacts), so
	 * you should only send messages to JIDs from your roster, otherwise you
	 * won't be able to see the message history.
	 */
	void sendMessage(QString jid, QString message, bool isSpoiler, QString spoilerHint);

	/**
	 * Correct the last message
	 *
	 */
	void correctMessage(const QString &msgId, const QString &message);

	/**
	 * Upload and send file
	 */
	void sendFile(const QString &jid, const QUrl &fileUrl, const QString &body);

	/**
	 * Add a contact to your roster
	 *
	 * @param nick A simple nick name for the new contact, which should be
	 *             used to display in the roster.
	 */
	void addContact(QString jid, QString nick, QString msg);

	/**
	 * Remove a contact from your roster
	 *
	 * Only the JID is needed.
	 */
	void removeContact(QString jid);

	/**
	 * Change a contact's name
	 */
	void renameContact(const QString &jid, const QString &newContactName);

	/**
	 * Downloads an attached media file of a message
	 *
	 * @param msgId The message
	 * @param url the media url from the message
	 */
	void downloadMedia(QString msgId, QString url);

	/**
	 * Changes the user's display name.
	 *
	 * @param displayName new name that is shown to contacts
	 */
	void changeDisplayName(const QString &displayName);

	/**
	 * Changes the user's password on the server
	 *
	 * @param newPassword The new password
	 */
	void changePassword(const QString &newPassword);

	/**
	 * Emitted when changing the user's password failed.
	 *
	 * @param errorMessage message describing the error
	 */
	void passwordChangeFailed(const QString &errorMessage);

	/**
	 * Emitted, when a contact was muted/unmuted.
	 */
	void notificationsMutedChanged(const QString& jid);

	/**
	 * Deletes the account data from the client and server.
	 */
	void deleteAccountFromClientAndServer();

	/**
	 * Deletes the account data from the configuration file and database.
	 */
	void deleteAccountFromClient();

	/**
	  * Requests fetching the version information of all resources of the given
	  * bare JID
	  */
	void requestClientVersions(const QString &bareJid, const QString &resource = {});

	/**
	  * Emitted when a client version information was received
	  */
	void clientVersionReceived(const QXmppVersionIq &versionIq);

public slots:
	/**
	 * Handles a changed application state and emits whether the application window is active.
	 *
	 * @param applicationState state of the GUI application
	 */
	void handleApplicationStateChanged(Qt::ApplicationState applicationState);

	/**
	 * Set current connection state
	 */
	void setConnectionState(Enums::ConnectionState connectionState);

	/**
	 * Sets a new connection error.
	 */
	void setConnectionError(ClientWorker::ConnectionError error);

	/**
	 * Receives messages from another instance of the application
	 */
	void receiveMessage(quint32, const QByteArray &msg)
	{
		// currently we only send XMPP URIs
		addOpenUri(msg);
	}

	/**
	 * Returns whether notifications are enabled for the given contact.
	 */
	bool notificationsMuted(const QString& jid);

	/**
	 * Sets the notifications to muted/unmuted.
	 *
	 * @param muted true if notifications should be muted
	 * @param jid JID of the entity for that notifications should be muted or unmuted
	 */
	void setNotificationsMuted(const QString& jid, bool muted);

private:
	/**
	 * Initializes the database and the corresponding thread.
	 */
	void initializeDatabase();

	/**
	 * Initializes the caches.
	 */
	void initializeCaches();

	/**
	 * Initializes the client worker and the corresponding thread.
	 *
	 * @param enableLogging true to enable logging, otherwise false
	 */
	void initializeClientWorker(bool enableLogging = true);

	Database *m_database;
	QThread *m_dbThrd;
	MessageDb *m_msgDb;
	RosterDb *m_rosterDb;
	QThread *m_cltThrd;
	ClientWorker::Caches *m_caches;
	ClientWorker *m_client;

	QString m_openUriCache;
	ConnectionState m_connectionState = ConnectionState::StateDisconnected;
	ClientWorker::ConnectionError m_connectionError = ClientWorker::NoError;

	static Kaidan *s_instance;
};

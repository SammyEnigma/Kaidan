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
#include <QList>
#include <QObject>
#include <QSettings>
// QXmpp
#include <QXmppClient.h>
// Kaidan
#include "AccountManager.h"
#include "AvatarFileStorage.h"
#include "Enums.h"
#include "MessageModel.h"
#include "PresenceCache.h"
#include "RosterModel.h"
#include "ServerFeaturesCache.h"
#include "TransferCache.h"
class AccountManager;
class LogHandler;
class ClientWorker;
class RegistrationManager;
class RosterManager;
class MessageHandler;
class DiscoveryManager;
class VCardManager;
class UploadManager;
class DownloadManager;
class VersionManager;

/**
 * The ClientWorker is used as a QObject-based worker on the ClientThread.
 */
class ClientWorker : public QObject
{
	Q_OBJECT

public:
	/**
	 * enumeration of possible connection errors
	 */
	enum ConnectionError {
		NoError,
		AuthenticationFailed,
		NotConnected,
		TlsFailed,
		TlsNotAvailable,
		DnsError,
		ConnectionRefused,
		NoSupportedAuth,
		KeepAliveError,
		NoNetworkPermission,
		RegistrationUnsupported
	};
	Q_ENUM(ConnectionError)

	struct Caches {
		Caches(RosterDb *rosterDb, MessageDb *msgDb, QObject *parent = nullptr)
			: settings(new QSettings(APPLICATION_NAME, APPLICATION_NAME)),
			  accountManager(new AccountManager(settings, parent)),
			  msgModel(new MessageModel(msgDb, parent)),
			  rosterModel(new RosterModel(rosterDb, parent)),
		          avatarStorage(new AvatarFileStorage(parent)),
		          serverFeaturesCache(new ServerFeaturesCache(parent)),
		          presCache(new PresenceCache(parent)),
			  transferCache(new TransferCache(parent))
		{
			rosterModel->setMessageModel(msgModel);
		}

		~Caches()
		{
			delete settings;
		}

		QSettings *settings;
		AccountManager *accountManager;
		MessageModel *msgModel;
		RosterModel *rosterModel;
		AvatarFileStorage *avatarStorage;
		ServerFeaturesCache *serverFeaturesCache;
		PresenceCache *presCache;
		TransferCache* transferCache;
	};

	/**
	 * @param caches All caches running in the main thread for communication with the UI.
	 * @param enableLogging If logging of the XMPP stream should be done.
	 * @param parent Optional QObject-based parent.
	 */
	ClientWorker(Caches *caches, bool enableLogging, QObject *parent = nullptr);

	/**
	 * Initializes this client worker as soon as it runs in a separate thread.
	 */
	void initialize();

	VCardManager *vCardManager() const;

	/**
	 * Returns all models and caches.
	 */
	Caches *caches() const;

	/**
	 * Returns whether the application window is active.
	 *
	 * The application window is active when it is in the foreground and focused.
	 */
	bool isApplicationWindowActive() const;

	/**
	 * Starts or enqueues a task which will be executed after successful login (e.g. a
	 * nickname change).
	 *
	 * This method is called by managers which must call "finishTask()" as soon as the
	 * task is completed.
	 *
	 * If the user is logged out when this method is called, a login is triggered, the
	 * task is started and a logout is triggered afterwards. However, if this method is
	 * called before a login with new credentials (e.g. during account registration), the
	 * task is started after the subsequent login.
	 *
	 * @param task task which is run directly if the user is logged in or enqueued to be
	 * run after an automatic login
	 */
	void startTask(const std::function<void ()> task);

	/**
	 * Finishes a task started by "startTask()".
	 *
	 * This must be called after a possible completion of a pending task.
	 *
	 * A logout is triggered when this method is called after the second login with the
	 * same credentials or later. That means, a logout is not triggered after a login with
	 * new credentials (e.g. after a registration).
	 */
	void finishTask();

public slots:
	/**
	 * Connects to the server and logs in with all needed configuration variables.
	 */
	void logIn();

	/**
	 * Connects to the server and requests a data form for account registration.
	 */
	void connectToRegister();

	/**
	 * Connects to the server with a minimal configuration.
	 *
	 * Some additional configuration variables can be set by passing a configuration.
	 *
	 * @param config configuration with additional variables for connecting to the server
	 * or nothing if only the minimal configuration should be used
	 */
	void connectToServer(QXmppConfiguration config = QXmppConfiguration());

	/**
	 * Logs out of the server if the client is not already logged out.
	 *
	 * @param isApplicationBeingClosed true if the application will be terminated directly after logging out, false otherwise
	 */
	void logOut(bool isApplicationBeingClosed = false);

	/**
	 * Deletes the account data from the client and server.
	 */
	void deleteAccountFromClientAndServer();

	/**
	 * Deletes the account data from the configuration file and database.
	 */
	void deleteAccountFromClient();

	/**
	 * Called when the account is deleted from the server.
	 */
	void handleAccountDeletedFromServer();

	/**
	 * Called when the account could not be deleted from the server.
	 *
	 * @param error error of the failed account deletion
	 */
	void handleAccountDeletionFromServerFailed(const QXmppStanza::Error &error);

signals:
	/**
	 * Emitted when an authenticated connection to the server is established with new
	 * credentials for the first time.
	 *
	 * The client will be in connected state when this is emitted.
	 */
	void loggedInWithNewCredentials();

	/**
	 * Requests to show a notification for a chat message via the system's notification channel.
	 *
	 * @param senderJid JID of the message's sender
	 * @param senderName name of the message's sender
	 * @param message message to show
	 */
	void showMessageNotificationRequested(const QString &senderJid, const QString &senderName, const QString &message);

	/**
	 * Emitted when the client's connection state changed.
	 *
	 * @param connectionState new connection state
	 */
	void connectionStateChanged(Enums::ConnectionState connectionState);

	/**
	 * Emitted when the client failed to connect to the server.
	 *
	 * @param error new connection error
	 */
	void connectionErrorChanged(ClientWorker::ConnectionError error);

	/**
	 * Deletes data related to the current account (messages, contacts etc.) from the database.
	 */
	void deleteAccountFromDatabase();

private slots:
	/**
	 * Sets the value to know whether the application window is active.
	 *
	 * @param active true if the application window is active, false otherwise
	 */
	void setIsApplicationWindowActive(bool active);

	/**
	 * Called when an authenticated connection to the server is established.
	 */
	void onConnected();

	/**
	 * Called when the connection to the server is closed.
	 */
	void onDisconnected();

	/**
	 * Handles the change of the connection state.
	 *
	 * @param connectionState new connection state
	 */
	void onConnectionStateChanged(QXmppClient::State connectionState);

	/**
	 * Handles a connection error.
	 *
	 * @param error new connection error
	 */
	void onConnectionError(QXmppClient::Error error);


private:
	/**
	 * Starts a pending (enqueued) task (e.g. a password change) if the variable (e.g. a
	 * password) could not be changed on the server before because the client was not
	 * logged in.
	 *
	 * @return true if at least one pending task is started on the second login with the
	 * same credentials or later, otherwise false
	 */
	bool startPendingTasks();

	Caches *m_caches;
	QXmppClient *m_client;
	LogHandler *m_logger;
	bool m_enableLogging;

	RegistrationManager *m_registrationManager;
	RosterManager *m_rosterManager;
	MessageHandler *m_messageHandler;
	DiscoveryManager *m_discoveryManager;
	VCardManager *m_vCardManager;
	UploadManager *m_uploadManager;
	DownloadManager *m_downloadManager;
	VersionManager *m_versionManager;

	QList<std::function<void ()>> m_pendingTasks;
	uint m_activeTasks = 0;

	bool m_isFirstLoginAfterStart = true;
	bool m_isApplicationWindowActive;
	bool m_isReconnecting = false;
	bool m_isDisconnecting = false;
	QXmppConfiguration m_configToBeUsedOnNextConnect;

	// These variables are used for checking the state of an ongoing account deletion.
	bool m_isAccountToBeDeletedFromClient = false;
	bool m_isAccountToBeDeletedFromClientAndServer = false;
	bool m_isAccountDeletedFromServer = false;
	bool m_isClientConnectedBeforeAccountDeletionFromServer = true;
};

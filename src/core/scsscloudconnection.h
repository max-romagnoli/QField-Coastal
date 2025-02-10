/******************************************************************************
    scsscloudconnection.h
    ---------------------
    begin                : January 2025
    copyright            : (C) 2025 QField Coastal by max-romagnoli
    email                : maxxromagnoli (at) gmail.com
 ******************************************************************************
 *                                                                            *
 *   This program is free software; you can redistribute it and/or modify     *
 *   it under the terms of the GNU General Public License as published by     *
 *   the Free Software Foundation; either version 2 of the License, or        *
 *   (at your option) any later version.                                      *
 *                                                                            *
 ******************************************************************************/

#ifndef SCSSCLOUDCONNECTION_H
#define SCSSCLOUDCONNECTION_H

#include <QObject>
#include <QNetworkReply>
#include <QVariantMap>
#include <QJsonDocument>
#include <QJsonObject>

/**
 * \ingroup core
 * \brief A minimal reference class to communicate with the QField Coastal Field Manager on SCSS,
 *        inspired by QField's QFieldCloudConnection but simplified.
 */
class ScssCloudConnection : public QObject
{
    Q_OBJECT

  public:
    //! Connection status
    enum class ConnectionStatus
    {
      Disconnected,
      Connecting,
      LoggedIn
    };
    Q_ENUM( ConnectionStatus )

    /**
     * \brief Constructor.
     */
    explicit ScssCloudConnection( QObject *parent = nullptr );

    /**
     * \brief The base URL of the Field Manager, TODO: for now http://127.0.0.1:8000, will probably be rooted to Proxy
     */
    Q_PROPERTY( QString baseUrl READ baseUrl WRITE setBaseUrl NOTIFY baseUrlChanged )

    /**
     * \brief The username for authentication.
     */
    Q_PROPERTY( QString username READ username WRITE setUsername NOTIFY usernameChanged )

    /**
     * \brief The password for authentication.
     */
    Q_PROPERTY( QString password READ password WRITE setPassword NOTIFY passwordChanged )

    /**
     * \brief The token obtained after successful login.
     */
    Q_PROPERTY( QString token READ token NOTIFY tokenChanged )

    /**
     * \brief The connection status enum property.
     */
    Q_PROPERTY( ConnectionStatus status READ status NOTIFY statusChanged )

  public:
    //! Returns the current base URL.
    QString baseUrl() const;
    //! Sets the base URL of the Field Manager.
    void setBaseUrl( const QString &url );

    //! Returns the current username.
    QString username() const;
    //! Sets the username.
    void setUsername( const QString &user );

    //! Returns the current password.
    QString password() const;
    //! Sets the password.
    void setPassword( const QString &pass );

    //! Returns the token used for authenticated requests.
    QString token() const;

    //! Returns the current status of the connection.
    ConnectionStatus status() const;

    /**
     * \brief Attempt to log in to Field Manager. 
     *        TODO: Could be: /api/token-auth/ or /api/v1/auth/.
     */
    Q_INVOKABLE void login();

    /**
     * \brief Log out from Field Manager, clearing any stored token.
     */
    Q_INVOKABLE void logout();

    /**
     * \brief A minimal example of uploading a project folder. 
     *
     * \param projectPath The local path to the project folder (containing .qgz + subfolders).
     */
    Q_INVOKABLE void uploadProject( const QString &projectPath );

    /**
     * \brief Join a project as a guest.
     *
     * \param projectSlug The project slug to join.
     */
    Q_INVOKABLE void joinProjectAsGuest(const QString &projectSlug);

    /**
     * \brief Download a project instance.
     * 
     * \param instanceId The instance ID to download.
     */
    Q_INVOKABLE void downloadProjectInstance( int instanceId );
    Q_INVOKABLE void downloadProjectInstanceZipped( int instanceId );

  signals:
    void baseUrlChanged();
    void usernameChanged();
    void passwordChanged();
    void tokenChanged();
    void statusChanged();
    void loginFailed( const QString &reason );
    void joinProjectAsGuestSuccess(const QJsonObject &jsonInfo);
    void joinProjectAsGuestFailed(const QString &reason);
    void downloadInstanceFailed( const QString &reason );
    void downloadInstanceSucceeded( const QString &destinationFolder, const QString &fileName );

  private slots:
    //! Slot called when the login network reply is finished.
    void onLoginReplyFinished();

  private:
    //! Helper method to set the authentication header on \a request if token is present.
    void setAuthHeader( QNetworkRequest &request ) const;

    //! Minimal helper to do a POST request with JSON data
    QNetworkReply *postJson( const QString &endpoint, const QVariantMap &payload );

    //! Minimal helper to do a GET request
    QNetworkReply *getJson( const QString &endpoint, const QVariantMap &params = QVariantMap() );

    //! Internal method for packaging and uploading local files
    void uploadFiles( const QString &projectPath );

    //! Helper to save network reply to a local file
    bool saveReplyToFile( QNetworkReply *reply, const QString &targetFile );

    // Zipped helpers
    bool unzipFile( const QString &zipFilePath, const QString &destinationPath );

    // Manifest helpers
    void onManifestReplyFinished();
    void startFileDownloads();
    void downloadNextFile();
    void onFileReplyFinished();

    int mCurrentInstanceId = -1;
    QString mDestinationFolder;
    QList<QVariantMap> mFilesToDownload;  // each entry might have { "path": "..", "checksum": "..", ... }
    int mCurrentFileIndex = 0;

    QString mBaseUrl;
    QString mUsername;
    QString mPassword;
    QString mToken;
    ConnectionStatus mStatus = ConnectionStatus::LoggedIn;  // TODO: for dev default to LoggedIn
};

#endif // SCSSCLOUDCONNECTION_H

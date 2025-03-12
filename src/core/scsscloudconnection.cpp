/******************************************************************************
    scsscloudconnection.cpp
    -----------------------
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

#include "scsscloudconnection.h"
#include "fileutils.h"
#include "JlCompress.h"

#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QHttpMultiPart>
#include <QTimer>
#include <QStandardPaths>
#include <QSettings>

ScssCloudConnection::ScssCloudConnection( QObject *parent )
  : QObject( parent )
{
}

QString ScssCloudConnection::baseUrl() const
{
  return mBaseUrl;
}

void ScssCloudConnection::setBaseUrl( const QString &url )
{
  if ( url == mBaseUrl )
    return;

  mBaseUrl = url;
  emit baseUrlChanged();
}

QString ScssCloudConnection::username() const
{
  return mUsername;
}

void ScssCloudConnection::setUsername( const QString &user )
{
  if ( user == mUsername )
    return;

  mUsername = user;
  emit usernameChanged();
}

QString ScssCloudConnection::password() const
{
  return mPassword;
}

void ScssCloudConnection::setPassword( const QString &pass )
{
  if ( pass == mPassword )
    return;

  mPassword = pass;
  emit passwordChanged();
}

QString ScssCloudConnection::token() const
{
  return mToken;
}

ScssCloudConnection::ConnectionStatus ScssCloudConnection::status() const
{
  return mStatus;
}

void ScssCloudConnection::login()
{
  // TODO: Might want to integrate a Django login endpoint at /api/token-auth/
  setBaseUrl( mBaseUrl.trimmed() );

  if ( mBaseUrl.isEmpty() )
  {
    emit loginFailed( tr( "Base URL not set" ) );
    return;
  }

  mStatus = ConnectionStatus::Connecting;
  emit statusChanged();

  QVariantMap payload;
  payload.insert( "username", mUsername );
  payload.insert( "password", mPassword );

  QNetworkReply *reply = postJson( "/api/token-auth/", payload );
  if ( !reply )
  {
    mStatus = ConnectionStatus::Disconnected;
    emit statusChanged();
    return;
  }

  // Handle reply with finished signal
  connect( reply, &QNetworkReply::finished, this, &ScssCloudConnection::onLoginReplyFinished );
}

void ScssCloudConnection::onLoginReplyFinished()
{
  QNetworkReply *reply = qobject_cast<QNetworkReply *>( sender() );
  if ( !reply )
    return;

  reply->deleteLater();

  if ( reply->error() != QNetworkReply::NoError )
  {
    QString reason = tr( "Network error: %1" ).arg( reply->errorString() );
    emit loginFailed( reason );
    mStatus = ConnectionStatus::Disconnected;
    emit statusChanged();
    return;
  }

  // Parse JSON response
  const QByteArray bytes = reply->readAll();
  const QJsonDocument doc = QJsonDocument::fromJson( bytes );
  const QJsonObject obj = doc.object();

  if ( !obj.contains( "token" ) )
  {
    emit loginFailed( tr( "Server response missing token" ) );
    mStatus = ConnectionStatus::Disconnected;
    emit statusChanged();
    return;
  }

  mToken = obj.value( "token" ).toString();
  if ( mToken.isEmpty() )
  {
    emit loginFailed( tr( "Empty token" ) );
    mStatus = ConnectionStatus::Disconnected;
    emit statusChanged();
    return;
  }

  // If we reach here, consider 'LoggedIn'
  mStatus = ConnectionStatus::LoggedIn;
  emit statusChanged();
  emit tokenChanged();
}

void ScssCloudConnection::logout()
{
  mToken.clear();
  mStatus = ConnectionStatus::Disconnected;
  emit statusChanged();
  emit tokenChanged();
}

void ScssCloudConnection::joinProjectAsGuest(const QString &projectSlug)
{
  if ( mBaseUrl.isEmpty() )
  {
    emit joinProjectAsGuestFailed( "Base URL not set" );
    qDebug() << "Base URL not set.";
    return;
  }

  QVariantMap payload;
  payload.insert( "project_id", projectSlug );
  payload.insert( "guest", true );

  QNetworkReply *reply = postJson( "/api/field_manager/projects/join/", payload );
  if ( !reply )
  {
    emit joinProjectAsGuestFailed( "Network error" );
    qDebug() << "Network or base URL error.";
    return;
  }

  connect( reply, &QNetworkReply::finished, this, [this, reply]()
  {
    reply->deleteLater();

    if ( reply->error() != QNetworkReply::NoError )
    {
      emit joinProjectAsGuestFailed( "Network error" );
      qDebug() << "Join project request failed:" << reply->errorString();
      return;
    }

    QByteArray bytes = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson( bytes );
    if ( doc.isNull() || !doc.isObject() )
    {
      emit joinProjectAsGuestFailed( "Invalid JSON response" );
      qDebug() << "Server returned invalid JSON response.";
      return;
    }

    QJsonObject obj = doc.object();
    if ( obj.contains("error") )
    {
      emit joinProjectAsGuestFailed( obj.value("error").toString() );
      qDebug() << "Error from server:" << obj.value("error").toString();
      return;
    }

    // Otherwise success
    // e.g. { "instance_id": 5, "instance_slug": "coastal-001-guest_abc123", ... }
    emit joinProjectAsGuestSuccess(obj);
    qDebug() << "Successfully joined project. Server response:" << obj;
  } );
}

void ScssCloudConnection::uploadProject( const QString &projectPath )
{
  // TODO: Skip authentication check for now
  // if ( mStatus != ConnectionStatus::LoggedIn || mToken.isEmpty() )
  // {
  //   emit loginFailed( tr( "Not logged in" ) );
  //   return;
  // }

  uploadFiles( projectPath );
}

void ScssCloudConnection::uploadFiles( const QString &projectPath )
{

  // 1. Collect all the files under 'projectPath':
  QDir dir( projectPath );
  if ( !dir.exists() )
  {
    qDebug() << "Project folder doesn't exist";
    return;
  }

  QString compressedPath = dir.path() + QStringLiteral( "/%1.zip" ).arg( dir.dirName() );
  if ( !JlCompress::compressDir( compressedPath, projectPath, true) )
  {
    qDebug() << "Project files were not zipped successfully.";
    return;
  }

  qDebug() << "Successfully compressed project to:" << compressedPath;
  QFile *compressedDirFile = new QFile( compressedPath );

  // TODO: now that use zipped, we might want to add a filter in size
  // TODO: Might want to implement different approach (recursive or zip entire folder)
 /*  QStringList filters;
  filters << "*.qgz" << "*.gpkg" << "*.png" << "*.jpg" << "*.jpeg";

  QFileInfoList fileInfoList = dir.entryInfoList( filters, QDir::Files | QDir::NoDotAndDotDot, QDir::Name );

  if ( fileInfoList.isEmpty() )
  {
    qDebug() << "No recognised files found.";
    return;
  } */

  // 2. Prepare the multipart form data:
  QHttpMultiPart *multiPart = new QHttpMultiPart( QHttpMultiPart::FormDataType );

  // Text part
  QHttpPart textPart;
  textPart.setHeader( QNetworkRequest::ContentDispositionHeader,
                      QVariant( "form-data; name=\"json_data\"" ) );

  QVariantMap metaData;
  metaData.insert( "instance_slug", dir.dirName() );
  metaData.insert( "user", mUsername );

  QJsonDocument jsonDoc( QJsonObject::fromVariantMap( metaData ) );
  textPart.setBody( jsonDoc.toJson() );
  multiPart->append( textPart );

  // File part
  if ( !compressedDirFile->open( QIODevice::ReadOnly ) )
  {
    compressedDirFile->deleteLater();
    qDebug() << "Could not read zipped directory";
    return;
  }

  QHttpPart filePart;
  filePart.setHeader( QNetworkRequest::ContentDispositionHeader,
                      QVariant( QString( "form-data; name=\"file\"; filename=\"%1.zip\"" )
                                .arg( dir.dirName() ) ) );

  filePart.setHeader( QNetworkRequest::ContentTypeHeader, QVariant( "application/octet-stream" ) );
  filePart.setBodyDevice( compressedDirFile ); 

  compressedDirFile->setParent( multiPart );
  multiPart->append( filePart );

  /* for ( const QFileInfo &fi : fileInfoList )
  {
    if ( !fi.exists() )
      continue;

    QFile *file = new QFile( fi.absoluteFilePath() );
    if ( !file->open( QIODevice::ReadOnly ) )
    {
      file->deleteLater();
      continue;
    }

    QHttpPart filePart;
    filePart.setHeader( QNetworkRequest::ContentDispositionHeader,
                        QVariant( QString( "form-data; name=\"file\"; filename=\"%1\"" )
                                  .arg( fi.fileName() ) ) );

    filePart.setHeader( QNetworkRequest::ContentTypeHeader, QVariant( "application/octet-stream" ) );
    filePart.setBodyDevice( file );

    file->setParent( multiPart );
    multiPart->append( filePart );
  } */

  // POST
  QString endpoint = mBaseUrl + "/api/field_manager/projects/upload/";

  QNetworkRequest request( endpoint );
  // request.setHeader( QNetworkRequest::ContentTypeHeader, QVariant( "multipart/form-data" ) );
  // setAuthHeader( request );  // TODO: Uncomment when authentication is implemented

  QNetworkAccessManager *nam = new QNetworkAccessManager( this );
  QNetworkReply *reply = nam->post( request, multiPart );

  multiPart->setParent( reply );

  connect( reply, &QNetworkReply::finished, this, [=]() {
    reply->deleteLater();
    QNetworkReply::NetworkError err = reply->error();
    if ( err != QNetworkReply::NoError )
    {
      qDebug() << "Upload failed:" << reply->errorString();
      return;
    }

    // Response
    QByteArray respBytes = reply->readAll();
    QJsonDocument respDoc = QJsonDocument::fromJson( respBytes );
    if ( !respDoc.isNull() && respDoc.isObject() )
    {
      qDebug() << "Upload success:" << respDoc.object();
    }
  } );

  /* if (QFile::remove(compressedPath)) {
    qDebug() << "Temporary file removed successfully.";
  } else {
    qDebug() << "Failed to remove temporary file.";
  } */
}

void ScssCloudConnection::setAuthHeader( QNetworkRequest &request ) const
{
  if ( !mToken.isEmpty() )
  {
    // TODO: if Django expects `Authorization: Token <token>`
    // or `Authorization: Bearer <token>`
    QString headerValue = QString( "Token %1" ).arg( mToken );
    request.setRawHeader( "Authorization", headerValue.toUtf8() );
  }
}


/**
 * Download project instance using manifest 
 * */ 

void ScssCloudConnection::downloadProjectInstance( int instanceSlug ) 
{
  if ( mBaseUrl.isEmpty() )
  {
    qDebug() << "Base URL not set, cannot download instance" << instanceSlug;
    emit downloadInstanceFailed("Base URL not set");
    return;
  }

  mCurrentInstanceId = instanceSlug;
  mFilesToDownload.clear();
  mCurrentFileIndex = 0;

  const QString endpoint = QStringLiteral( "/api/field_manager/project-instances/%1/manifest" ).arg( instanceSlug );
  const QString urlString = mBaseUrl + endpoint;
  QUrl requestUrl( urlString );

  QNetworkRequest request( requestUrl );
  setAuthHeader( request );

  QNetworkAccessManager *nam = new QNetworkAccessManager( this );
  QNetworkReply *reply = nam->get( request );

  connect( reply, &QNetworkReply::finished, this, &ScssCloudConnection::onManifestReplyFinished );
}

void ScssCloudConnection::downloadProjectInstanceZipped( QString instanceSlug )
{
  if ( mBaseUrl.isEmpty() )
  {
    qDebug() << "Base URL not set, cannot download zipped instance" << instanceSlug;
    emit downloadInstanceFailed("Base URL not set");
    return;
  }

  // For example: your server returns JSON with { "qgs_filename": "coastal.qgz", "zip_data": <BASE64 STRING> }
  QString endpoint = QStringLiteral( "/api/field_manager/project-instances/%1/download/" ).arg( instanceSlug );
  QUrl requestUrl( mBaseUrl + endpoint );

  QNetworkRequest request( requestUrl );
  setAuthHeader( request );

  QNetworkAccessManager *nam = new QNetworkAccessManager( this );
  QNetworkReply *reply = nam->get( request );

  connect( reply, &QNetworkReply::finished, this, [this, reply, instanceSlug]()
  {
    reply->deleteLater();

    if ( reply->error() != QNetworkReply::NoError )
    {
      qDebug() << "Zipped download request failed:" << reply->errorString();
      emit downloadInstanceFailed( reply->errorString() );
      return;
    }

    // 1) Parse JSON to retrieve "qgs_filename" and base64 "zip_data"
    QByteArray rawJson = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(rawJson);
    if ( doc.isNull() || !doc.isObject() )
    {
      qDebug() << "Server response is not valid JSON";
      emit downloadInstanceFailed("Invalid JSON response");
      return;
    }

    QJsonObject obj = doc.object();
    if ( !obj.contains("zip_data") || !obj.contains("qgs_filename") || !obj.contains("instance_slug") )
    {
      qDebug() << "JSON missing zip_data or qgs_filename";
      emit downloadInstanceFailed("Missing fields in JSON (zip_data / qgs_filename)");
      return;
    }

    QString qgsFilename = obj.value("qgs_filename").toString();
    if ( qgsFilename.isEmpty() )
    {
      qgsFilename = "coastal.qgz"; // fallback
    }

    QString instanceSlug = obj.value("instance_slug").toString();
    if ( instanceSlug.isEmpty() )
    {
      qDebug() << "Project instance slug is empty";
      emit downloadInstanceFailed("Project instance slug is empty");
      return;
    }

    // The ZIP data in base64 form
    QString zipDataBase64 = obj.value("zip_data").toString();
    QByteArray zipData = QByteArray::fromBase64( zipDataBase64.toUtf8() );
    if ( zipData.isEmpty() )
    {
      qDebug() << "Base64 decode returned empty ZIP data";
      emit downloadInstanceFailed("Empty or invalid ZIP data");
      return;
    }

    // 2) Create local folder structure
    QString baseDir = QStandardPaths::writableLocation( QStandardPaths::AppDataLocation );
    if ( baseDir.isEmpty() )
    {
      baseDir = QDir::homePath() + "/.local/share/QFieldCoastal";
    }

    QDir dir( baseDir + "/coastal_projects" );
    if ( !dir.exists() )
      dir.mkpath( dir.path() );

    QString zipFilePath = dir.path() + QStringLiteral( "/%1.zip" ).arg( instanceSlug );

    // 3) Write the decoded ZIP bytes to disk
    QFile outFile( zipFilePath );
    if ( !outFile.open( QFile::WriteOnly | QFile::Truncate ) )
    {
      qDebug() << "Could not open zip file for writing:" << zipFilePath;
      emit downloadInstanceFailed("Failed to open .zip for writing");
      return;
    }
    outFile.write( zipData );
    outFile.close();

    qDebug() << "Saved zipped project to:" << zipFilePath;

    // 4) Unzip to final location
    QString destinationFolder = dir.path() + QStringLiteral( "/%1" ).arg( instanceSlug );

    // Remove old if exists
    QDir destDir( destinationFolder );
    if ( destDir.exists() )
    {
      destDir.removeRecursively();
    }

    // Use JlCompress
    QStringList extracted = JlCompress::extractDir(zipFilePath, destinationFolder);
    if ( extracted.isEmpty() )
    {
      qDebug() << "Failed to unzip the project instance to:" << destinationFolder;
      emit downloadInstanceFailed("Failed to unzip");
      return;
    }

    qDebug() << "Project instance unzipped at:" << destinationFolder;

    // 5) Emit success, pass qgsFilename so QML can open it
    emit downloadInstanceSucceeded( destinationFolder, qgsFilename );
  } );
}

bool ScssCloudConnection::unzipFile( const QString &zipFilePath, const QString &destinationPath )
{
  qDebug() << "Unzipping file:" << zipFilePath << "->" << destinationPath;
  QStringList extractedFiles = JlCompress::extractDir( zipFilePath, destinationPath );

  if ( extractedFiles.isEmpty() )
  {
    qDebug() << "JlCompress::extractDir returned an empty list, indicating failure for:" << zipFilePath;
    return false;
  }

  return true;
}

void ScssCloudConnection::onManifestReplyFinished()
{
  QNetworkReply *reply = qobject_cast<QNetworkReply *>( sender() );
  if ( !reply )
    return;

  reply->deleteLater();

  if ( reply->error() != QNetworkReply::NoError )
  {
    qDebug() << "Manifest request failed:" << reply->errorString();
    emit downloadInstanceFailed( reply->errorString() );
    return;
  }

  // Parse the manifest JSON
  const QByteArray bytes = reply->readAll();
  QJsonDocument doc = QJsonDocument::fromJson( bytes );
  if ( doc.isNull() || !doc.isObject() )
  {
    qDebug() << "Server returned invalid manifest JSON.";
    emit downloadInstanceFailed( "Invalid JSON manifest" );
    return;
  }

  QJsonObject rootObj = doc.object();
  if ( !rootObj.contains("files") || !rootObj.value("files").isArray() )
  {
    qDebug() << "Manifest JSON missing 'files' array.";
    emit downloadInstanceFailed( "Manifest missing files array" );
    return;
  }

  QJsonArray filesArr = rootObj.value("files").toArray();
  if ( filesArr.isEmpty() )
  {
    qDebug() << "Manifest indicates no files to download. Possibly an empty project?";
    emit downloadInstanceSucceeded( QString(), "TODO:" );
    return;
  }

  // Convert array items to a simpler structure in mFilesToDownload
  for ( const QJsonValue &val : filesArr )
  {
    if ( val.isObject() )
    {
      QJsonObject fObj = val.toObject();
      // We assume something like: { "path": "collection/invasive.gpkg", "checksum": "abc123", ... }
      QVariantMap map = fObj.toVariantMap();
      mFilesToDownload.append( map );
    }
  }

  // Prepare local folder for the instance
  QString baseDir = QStandardPaths::writableLocation( QStandardPaths::AppDataLocation );
  if ( baseDir.isEmpty() )
  {
    baseDir = QDir::homePath() + "/.local/share/QFieldCoastal"; // fallback
  }

  QDir rootDir( baseDir + "/coastal_projects" );
  if ( !rootDir.exists() )
  {
    rootDir.mkpath( rootDir.path() );
  }

  mDestinationFolder = rootDir.path() + QStringLiteral( "/instance_%1" ).arg( mCurrentInstanceId );

  // Optionally remove old data or do partial checks for old files
  QDir destDir( mDestinationFolder );
  if ( destDir.exists() )
  {
    destDir.removeRecursively();
  }
  destDir.mkpath( mDestinationFolder );

  // Start downloading each file in turn
  startFileDownloads();
}

void ScssCloudConnection::startFileDownloads()
{
  if ( mFilesToDownload.isEmpty() )
  {
    // no files => done
    emit downloadInstanceSucceeded( mDestinationFolder , ""); // TODO:
    return;
  }

  mCurrentFileIndex = 0;
  downloadNextFile();  // triggers the first file download
}

void ScssCloudConnection::downloadNextFile()
{
  if ( mCurrentFileIndex >= mFilesToDownload.size() )
  {
    // All files done
    qDebug() << "All manifest files downloaded for instance" << mCurrentInstanceId;
    emit downloadInstanceSucceeded( mDestinationFolder, "" );   // TODO:
    return;
  }

  const QVariantMap fileInfo = mFilesToDownload.at( mCurrentFileIndex );
  const QString relPath = fileInfo.value("path").toString();
  if ( relPath.isEmpty() )
  {
    // skip or handle
    mCurrentFileIndex++;
    downloadNextFile();
    return;
  }

  // Build a GET request for the file
  // e.g. /api/field_manager/project-instances/<id>/file?path=relPath
  QString endpoint = QStringLiteral("/api/field_manager/project-instances/%1/file").arg( mCurrentInstanceId );
  QUrl requestUrl( mBaseUrl + endpoint );

  // Add query param for the file path
  QUrlQuery query;
  query.addQueryItem("path", relPath);
  requestUrl.setQuery(query);

  QNetworkRequest request( requestUrl );
  setAuthHeader( request );

  QNetworkAccessManager *nam = new QNetworkAccessManager( this );
  QNetworkReply *reply = nam->get( request );

  connect( reply, &QNetworkReply::finished, this, &ScssCloudConnection::onFileReplyFinished );
}

void ScssCloudConnection::onFileReplyFinished()
{
  QNetworkReply *reply = qobject_cast<QNetworkReply *>( sender() );
  if ( !reply )
    return;

  reply->deleteLater();

  if ( reply->error() != QNetworkReply::NoError )
  {
    qDebug() << "File download failed:" << reply->errorString();
    emit downloadInstanceFailed( reply->errorString() );
    return;
  }

  // Identify which file we were downloading
  const QVariantMap fileInfo = mFilesToDownload.at( mCurrentFileIndex );
  const QString relPath = fileInfo.value("path").toString();

  // Build local path in mDestinationFolder
  QString localFilePath = mDestinationFolder + "/" + relPath;
  QFileInfo fi( localFilePath );
  QDir localDir( fi.absoluteDir() );
  if ( !localDir.exists() )
  {
    localDir.mkpath( localDir.path() );
  }

  // Save to disk
  if ( !saveReplyToFile( reply, localFilePath ) )
  {
    qDebug() << "Failed to save file to disk:" << localFilePath;
    emit downloadInstanceFailed("Failed to save " + relPath);
    return;
  }

  // TODO: If you want to verify checksums, do it here
  // e.g. compare with fileInfo["checksum"]

  qDebug() << "Downloaded file:" << relPath;

  // Move on to the next file
  mCurrentFileIndex++;
  downloadNextFile();
}

QNetworkReply *ScssCloudConnection::postJson( const QString &endpoint, const QVariantMap &payload )
{
  if ( mBaseUrl.isEmpty() )
    return nullptr;

  QString url = mBaseUrl + endpoint;
  QUrl requestUrl( url );
  QNetworkRequest request( requestUrl );
  request.setHeader( QNetworkRequest::ContentTypeHeader, "application/json" );
  setAuthHeader( request );

  QJsonDocument doc( QJsonObject::fromVariantMap( payload ) );
  QByteArray data = doc.toJson();

  QNetworkAccessManager *nam = new QNetworkAccessManager( this );
  QNetworkReply *reply = nam->post( request, data );
  return reply;
}

QNetworkReply *ScssCloudConnection::getJson( const QString &endpoint, const QVariantMap &params )
{
  if ( mBaseUrl.isEmpty() )
    return nullptr;

  QString url = mBaseUrl + endpoint;
  QUrl requestUrl( url );

  if ( !params.isEmpty() )
  {
    QUrlQuery query;
    for ( auto it = params.constBegin(); it != params.constEnd(); ++it )
    {
      query.addQueryItem( it.key(), it.value().toString() );
    }
    requestUrl.setQuery( query );
  }

  QNetworkRequest request( requestUrl );
  request.setHeader( QNetworkRequest::ContentTypeHeader, "application/json" );
  setAuthHeader( request );

  QNetworkAccessManager *nam = new QNetworkAccessManager( this );
  QNetworkReply *reply = nam->get( request );
  return reply;
}

bool ScssCloudConnection::saveReplyToFile( QNetworkReply *reply, const QString &targetFile )
{
  QFile outFile( targetFile );
  if ( !outFile.open( QFile::WriteOnly | QFile::Truncate ) )
  {
    qDebug() << "Could not open file for writing:" << targetFile;
    return false;
  }

  const QByteArray data = reply->readAll();
  qint64 written = outFile.write( data );
  outFile.close();

  if ( written < data.size() )
  {
    qDebug() << "Not all data was written to:" << targetFile;
    return false;
  }
  return true;
}

void ScssCloudConnection::identifyPlant(const QString &imageFilePath, 
  const QString &plantNetApiKey,
  const QString &project)
{
  // TODO: get api key from ini (will need to implement QKeychain)
  QSettings settings("config.ini", QSettings::IniFormat);
  QString todoApiKey = settings.value("PLANT_API_KEY").toString();

  // 1) Validate inputs
  if (imageFilePath.isEmpty() || todoApiKey.isEmpty())  // TODO:
  {
  emit plantIdentificationFailed("Missing image file or API key");
  return;
  }

  QFileInfo fi(imageFilePath);
  if (!fi.exists())
  {
  emit plantIdentificationFailed("Image file does not exist");
  return;
  }

  // 2) Build the endpoint
  // e.g. https://api.plantnet.org/v2/identify/all?api-key=API_KEY
  QString endpoint = QString("https://my-api.plantnet.org/v2/identify/%1").arg(project);
  QUrl requestUrl(endpoint);

  QUrlQuery query;
  query.addQueryItem("api-key", todoApiKey);
  // add additional parameters if needed, e.g. "include-related-images"
  // query.addQueryItem("include-related-images", "false");
  requestUrl.setQuery(query);

  QNetworkRequest request(requestUrl);
  // If you had an Authorization header, do it here
  // e.g.: request.setRawHeader("Authorization", "Bearer xxx");

  // 3) Prepare multi-part data
  QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

  // We can also add 'organs' if we know them. We'll skip it for simplicity
  // So the param name is "images"
  QHttpPart filePart;
  filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
  QVariant(QString("form-data; name=\"images\"; filename=\"%1\"").arg(fi.fileName())));
  // Common image mime type, or "image/png" if you want
  filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/jpeg"));

  QFile *file = new QFile(imageFilePath);
  if (!file->open(QIODevice::ReadOnly))
  {
  file->deleteLater();
  multiPart->deleteLater();
  emit plantIdentificationFailed("Failed to open image file");
  return;
  }
  file->setParent(multiPart); // so it gets deleted with multiPart
  filePart.setBodyDevice(file);
  multiPart->append(filePart);

  // 4) Post
  QNetworkAccessManager *nam = new QNetworkAccessManager(this);
  QNetworkReply *reply = nam->post(request, multiPart);
  multiPart->setParent(reply);

  connect(reply, &QNetworkReply::finished, this, [this, reply]()
  {
  reply->deleteLater();
  if (reply->error() != QNetworkReply::NoError)
  {
  emit plantIdentificationFailed(reply->errorString());
  return;
  }

  QByteArray data = reply->readAll();
  QJsonDocument doc = QJsonDocument::fromJson(data);
  if (doc.isNull() || !doc.isObject())
  {
  emit plantIdentificationFailed("Invalid JSON response");
  return;
  }
  QJsonObject obj = doc.object();
  if (obj.contains("error"))
  {
  emit plantIdentificationFailed(obj.value("error").toString());
  return;
  }

  // Otherwise success
  emit plantIdentificationSuccess(obj);
  });
}
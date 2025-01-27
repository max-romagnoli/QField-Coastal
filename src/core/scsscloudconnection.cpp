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

#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QHttpMultiPart>
#include <QTimer>

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
    qDebug() << "Base URL not set.";
    return;
  }

  QVariantMap payload;
  payload.insert( "project_id", projectSlug );
  payload.insert( "guest", true );

  QNetworkReply *reply = postJson( "/api/field_manager/projects/join/", payload );
  if ( !reply )
  {
    qDebug() << "Network or base URL error.";
    return;
  }

  connect( reply, &QNetworkReply::finished, this, [this, reply]()
  {
    reply->deleteLater();

    if ( reply->error() != QNetworkReply::NoError )
    {
      qDebug() << "Join project request failed:" << reply->errorString();
      return;
    }

    QByteArray bytes = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson( bytes );
    if ( doc.isNull() || !doc.isObject() )
    {
      qDebug() << "Server returned invalid JSON response.";
      return;
    }

    QJsonObject obj = doc.object();
    if ( obj.contains("error") )
    {
      qDebug() << "Error from server:" << obj.value("error").toString();
      return;
    }

    // Otherwise success
    // e.g. { "instance_id": 5, "instance_slug": "coastal-001-guest_abc123", ... }
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

  // TODO: Might want to implement different approach (recursive or zip entire folder)
  QStringList filters;
  filters << "*.qgz" << "*.gpkg" << "*.png" << "*.jpg" << "*.jpeg";

  QFileInfoList fileInfoList = dir.entryInfoList( filters, QDir::Files | QDir::NoDotAndDotDot, QDir::Name );

  if ( fileInfoList.isEmpty() )
  {
    qDebug() << "No recognised files found.";
    return;
  }

  // 2. Prepare the multipart form data:
  QHttpMultiPart *multiPart = new QHttpMultiPart( QHttpMultiPart::FormDataType );

  // Text part
  QHttpPart textPart;
  textPart.setHeader( QNetworkRequest::ContentDispositionHeader,
                      QVariant( "form-data; name=\"json_data\"" ) );

  QVariantMap metaData;
  metaData.insert( "project_name", dir.dirName() );
  metaData.insert( "user", mUsername );

  QJsonDocument jsonDoc( QJsonObject::fromVariantMap( metaData ) );
  textPart.setBody( jsonDoc.toJson() );
  multiPart->append( textPart );

  // File part
  for ( const QFileInfo &fi : fileInfoList )
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
  }

  // POST
  QString endpoint = mBaseUrl + "/api/projects/upload/";

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

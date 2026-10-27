#include "../RequestDeadline.hpp"
// Adapted from ZhihuDaily-Cascades by liuwanlin. See THIRD_PARTY_NOTICES.md.
/*
 * Requester.cpp
 *
 *  Created on: 2018718
 *      Author: liuwanlin
 */

#include "Requester.hpp"

#include <bb/cascades/ImageView>
#include <QObject>
#include <QCoreApplication>
#include <QTimer>
#include <QtCore/QDir>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkDiskCache>
#include <QtNetwork/QSslConfiguration>
#include <QtNetwork/QSsl>
#include <bb/data/JsonDataAccess>

using namespace bb::data;

QNetworkAccessManager *Requester::qNetworkAccessManager = 0;
QNetworkDiskCache *Requester::qNetworkDiskCache = 0;

QString Requester::CACHE_DIR = QDir::homePath() + "/cache/data";
qint64 Requester::CACHE_SIZE = 100 * 1024 * 1024; // 100M

Requester::Requester() : QObject() {
    if (!qNetworkAccessManager) {
        qNetworkAccessManager = new QNetworkAccessManager(QCoreApplication::instance());
        qNetworkDiskCache = new QNetworkDiskCache(qNetworkAccessManager);
    }
    QDir().mkpath(CACHE_DIR);
    this->mMethod = Get;

    QFileInfo cacheDir(Requester::CACHE_DIR);
    if(!cacheDir.exists()) {
        QDir().mkdir(cacheDir.path());
    }

    qNetworkDiskCache->setCacheDirectory(Requester::CACHE_DIR);
    qNetworkDiskCache->setMaximumCacheSize(Requester::CACHE_SIZE);
    qNetworkAccessManager->setCache(qNetworkDiskCache);
}

QUrl Requester::url() const {
    return this->mUrl;
}

Requester::Method Requester::method() const {
    return this->mMethod;
}

QVariant Requester::params() const {
    return this->mParams;
}

QVariant Requester::headers() const {
    return this->mHeaders;
}

void Requester::setUrl(const QUrl url) {
    this->mUrl = url;
    emit urlChanged();
}

void Requester::setMethod(const Method method) {
    this->mMethod = method;
    emit methodChanged();
}

void Requester::setParams(const QVariant params) {
    this->mParams = params;
    emit paramsChanged();
}

void Requester::setHeaders(const QVariant headers) {
    this->mHeaders = headers;
    emit headersChanged();
}

void Requester::setCacheLimit(qint64 bytes) {
    CACHE_SIZE = bytes;
    if (qNetworkDiskCache) {
        qNetworkDiskCache->setMaximumCacheSize(bytes);
        if (qNetworkDiskCache->cacheSize() > bytes) qNetworkDiskCache->clear();
    } else {
        QNetworkDiskCache cache;
        cache.setCacheDirectory(CACHE_DIR);
        cache.setMaximumCacheSize(bytes);
        if (cache.cacheSize() > bytes) cache.clear();
    }
}
void Requester::clearCache() {
    if (qNetworkDiskCache) qNetworkDiskCache->clear();
    else {
        QNetworkDiskCache cache;
        cache.setCacheDirectory(CACHE_DIR); cache.clear();
    }
}

/**
 * 
 */
void Requester::send() {
    this->send(this->mUrl);
}

void Requester::send(QUrl url) {
    emit beforeSend();

    QNetworkRequest request;
    // 
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferNetwork);
    // Direct public HTTPS was verified on BB10; never suppress certificate errors.
    QUrl secureUrl(url);
    if (secureUrl.scheme() == "http") secureUrl.setScheme("https");
    request.setUrl(secureUrl);
    request.setRawHeader("User-Agent", "Mozilla/5.0");
    request.setRawHeader("Accept-Encoding", "identity");
    //  header
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("Content-Type", "application/json");
    //  header
    QMap<QString, QVariant> headers = this->mHeaders.toMap();
    QMapIterator<QString, QVariant> headersIt(headers);
    while(headersIt.hasNext()) {
        headersIt.next();
        request.setRawHeader(headersIt.key().toUtf8(), headersIt.value().toByteArray());
    }
    /**
     * @TODO:  github https  SSL handshake failed 
     * QSsl::SslV3
     * QSsl::SslV2
     * QSsl::TlsV1
     * QSsl::UnknownProtocol
     * QSsl::AnyProtocol
     * QSsl::TlsV1SslV3
     * QSsl::SecureProtocols
     */
//    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
//    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
//    sslConfig.setProtocol(QSsl::AnyProtocol);
//    request.setSslConfiguration(sslConfig);

    QNetworkReply *reply = NULL;

    if(this->mMethod == Get) {
        reply = qNetworkAccessManager->get(request);
    }else if(this->mMethod == Post) {
        QMap<QString, QVariant> params = this->mParams.toMap();
        JsonDataAccess jda;
        QString paramsStr;

        // QT Object convert to JSON string
        jda.saveToBuffer(params, &paramsStr);

        reply = qNetworkAccessManager->post(request, paramsStr.toUtf8());
    }

    connect(reply, SIGNAL(finished()), this, SLOT(replyFinished()));
    connect(reply, SIGNAL(finished()), reply, SLOT(deleteLater()));
    new RequestDeadline(reply);
}

void Requester::replyFinished() {
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    QVariant fromCache = reply->attribute(QNetworkRequest::SourceIsFromCacheAttribute);

    qDebug() << "Http:" << reply->url() << "fromCache:" << fromCache.toBool();

    if(reply->error() == QNetworkReply::NoError) {
        QString data = QString::fromUtf8(reply->readAll());

//        qDebug() << "Http Data:" << data;

        emit finished(data);
    }else {
//        qDebug() << "Http Error:" << reply->errorString();

        emit error(reply->errorString());
    }


}

#include "../RequestDeadline.hpp"
// Adapted from ZhihuDaily-Cascades by liuwanlin. See THIRD_PARTY_NOTICES.md.
/*
 * WebImageView.cpp
 *
 *  Created on: 2018718
 *      Author: liuwanlin
 */

#include "WebImageView.hpp"
#include "../Misc/Misc.hpp"
#include <QCoreApplication>
#include <QTimer>

#include <bb/cascades/ImageView>

#include <QtCore/QUrl>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkDiskCache>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

QNetworkAccessManager *WebImageView::qNetworkAccessManager = 0;
QNetworkDiskCache *WebImageView::qNetworkDiskCache = 0;

QString WebImageView::CACHE_DIR = QDir::homePath() + "/cache/images";
qint64 WebImageView::CACHE_SIZE = 100 * 1024 * 1024; // 100M

WebImageView::WebImageView() : ImageView() {
    if (!qNetworkAccessManager) {
        qNetworkAccessManager = new QNetworkAccessManager(QCoreApplication::instance());
        qNetworkDiskCache = new QNetworkDiskCache(qNetworkAccessManager);
    }
    QDir().mkpath(CACHE_DIR);
    QFileInfo cacheDir(WebImageView::CACHE_DIR);
    if(!cacheDir.exists()) {
        QDir().mkdir(cacheDir.path());
    }

    qNetworkDiskCache->setCacheDirectory(WebImageView::CACHE_DIR);
    qNetworkDiskCache->setMaximumCacheSize(WebImageView::CACHE_SIZE);
    qNetworkAccessManager->setCache(qNetworkDiskCache);
}

void WebImageView::setUrl(const QUrl url) {
    this->mUrl = url;
    this->resetImage();
    this->imageData.clear();

    emit urlChanged();

    //  LoadingImage
    if(!this->mLoadingImageSource.isEmpty()) {
        this->setImageSource(this->mLoadingImageSource);
    }

    QString scheme = url.scheme().toLower();

    if(scheme == "") {
        this->setImageSource(this->mFailImageSource);
        emit loaded();
        return;
    }

    if(scheme != "http" && scheme != "https" && scheme != "ftp") {
        this->setImageSource(url);
        emit loaded();
        return;
    }

    QNetworkRequest request;
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
    // Direct public HTTPS was verified on BB10; never suppress certificate errors.
    QUrl secureUrl(url);
    if (secureUrl.scheme() == "http") secureUrl.setScheme("https");
    request.setUrl(secureUrl);
    request.setRawHeader("User-Agent", "Mozilla/5.0");
    request.setRawHeader("Accept-Encoding", "identity");

    //  GET 
    QNetworkReply *reply = qNetworkAccessManager->get(request);

    connect(reply, SIGNAL(finished()), this, SLOT(replyFinished()));
    connect(reply, SIGNAL(finished()), reply, SLOT(deleteLater()));
    new RequestDeadline(reply);
    connect(reply, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(downloadProgress(qint64, qint64)));
}

QUrl WebImageView::url() const {
    return this->mUrl;
}

void WebImageView::replyFinished() {
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (reply->request().url().path() != mUrl.path()) return;

    if(reply->error() == QNetworkReply::NoError) {
        imageData = reply->readAll();
        setImage(Image(imageData));
    }else {
        if(!this->mFailImageSource.isEmpty()) {
            this->setImageSource(this->mFailImageSource);
        }
        qDebug() << "WebImageView reply on error:" << reply->errorString();
    }

    emit loaded();

}

void WebImageView::downloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    emit progressed(bytesReceived, bytesTotal);
}

void WebImageView::setCacheLimit(qint64 bytes) {
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
void WebImageView::clearCache() {
    if (qNetworkDiskCache) qNetworkDiskCache->clear();
    else {
        QNetworkDiskCache cache;
        cache.setCacheDirectory(CACHE_DIR); cache.clear();
    }
    // 
    QDir cacheDir(WebImageView::CACHE_DIR);
    cacheDir.setFilter(QDir::NoDotAndDotDot | QDir::Files);

    foreach(const QString& file, cacheDir.entryList()) {
        cacheDir.remove(file);
    }
}

void WebImageView::invokeViewImage() {
    // 1. 
    // 2. invoke 
    if(this->imageData.isEmpty()) {
        qDebug() << "WebImageView invokeViewImage imageData isEmpty";
        return;
    }

    QString filePath = WebImageView::CACHE_DIR + "/invoke_pre_image.gif";
    QFile file(filePath);

    if(file.open(QIODevice::WriteOnly)) {
        file.write(this->imageData);
        file.close();
        filePath = "file://" + filePath;

        // invoke
        Misc().invokeViewIamge(filePath);
    }else {
        qDebug() << "WebImageView invokeViewImage open file fail";
    }

    emit invokeViewImaged();
}

void WebImageView::setFailImageSource(const QUrl url) {
    this->mFailImageSource = url;
    emit failImageSourceChanged();
}

QUrl WebImageView::failImageSource() const {
    return this->mFailImageSource;
}

void WebImageView::setLoadingImageSource(const QUrl url) {
    this->mLoadingImageSource = url;
    emit loadingImageSourceChanged();
}

QUrl WebImageView::loadingImageSource() const {
    return this->mLoadingImageSource;
}

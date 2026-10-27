/*
 * Copyright (c) 2011-2015 BlackBerry Limited.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "applicationui.hpp"
#include "daily/WebImageView/WebImageView.hpp"
#include "daily/Requester/Requester.hpp"

#include <bb/cascades/Application>

#include <QLocale>
#include <QTranslator>
#include <QDir>
#include <QFile>
#include <QTimer>
#include <QtDeclarative>
#include <cstdio>

#include <Qt/qdeclarativedebug.h>

using namespace bb::cascades;

static void logMessage(QtMsgType type, const char *message)
{
    QFile file(QDir::homePath() + "/data/zhihu.log");
    if (file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        file.write(message); file.write("\n");
    }
    std::fprintf(stderr, "%s\n", message);
    if (type == QtFatalMsg) std::abort();
}

Q_DECL_EXPORT int main(int argc, char **argv)
{
    QDir().mkpath(QDir::homePath() + "/data");
    QFile log(QDir::homePath() + "/data/zhihu.log");
    if (log.open(QIODevice::WriteOnly | QIODevice::Truncate)) log.close();
    qInstallMsgHandler(logMessage);
    qmlRegisterType<QTimer>("tech.lwl", 1, 0, "QTimer");
    qmlRegisterType<WebImageView>("tech.lwl", 1, 0, "WebImageView");
    qmlRegisterType<Requester>("tech.lwl", 1, 0, "Requester");
    Application app(argc, argv);

    // Create the Application UI object, this is where the main.qml file
    // is loaded and the application scene is set.
    ApplicationUI appui;

    // Enter the application main event loop.
    return Application::exec();
}

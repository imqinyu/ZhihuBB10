#include "check.hpp"
#include "applicationui.hpp"
#include "zhihu.hpp"
#include "phone_login.hpp"
#include "daily/WebImageView/WebImageView.hpp"
#include "daily/Requester/Requester.hpp"
#include <bb/cascades/Application>
#include <bb/cascades/TabbedPane>
#include <bb/cascades/Tab>
#include <bb/cascades/NavigationPane>
#include <bb/cascades/Page>
#include <bb/cascades/ListView>
#include <bb/cascades/AbstractActionItem>
#include <bb/cascades/DropDown>
#include <bb/cascades/ToggleButton>
#include <bb/cascades/SegmentedControl>
#include <bb/cascades/TitleBar>
#include <bb/cascades/TitleBarKind>
#include <bb/cascades/Window>
#include <bb/cascades/Label>
#include <bb/cascades/LabelAutoSizeProperties>
#include <bb/cascades/LayoutUpdateHandler>
#include <QtGui/QImage>
#include <cerrno>
#include <bb/system/Clipboard>
#include <bb/system/InvokeManager>
#include <bb/data/JsonDataAccess>
#include <QTimer>
#include <QFile>
#include <QDir>
#include <QNetworkDiskCache>
#include <QNetworkCacheMetaData>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QDateTime>
#include <QCryptographicHash>
#include <QCoreApplication>
#include <QtDeclarative>
#include <cstdio>
using namespace bb::cascades;
static void message(QtMsgType, const char *text) { std::fprintf(stderr, "%s\n", text); std::fflush(stderr); }
static QString sha1(const QString &value) {
    return QCryptographicHash::hash(value.toUtf8(), QCryptographicHash::Sha1).toHex();
}
static bool snapshot() {
    const screen_window_t window=Application::instance()->mainWindow()->handle();
    screen_context_t context=0; screen_pixmap_t pixmap=0; screen_buffer_t buffer=0;
    int size[2]={0,0},format=SCREEN_FORMAT_RGBA8888,usage=SCREEN_USAGE_READ|SCREEN_USAGE_NATIVE;
    bool ok=screen_get_window_property_pv(window,SCREEN_PROPERTY_CONTEXT,reinterpret_cast<void **>(&context))==0
        && screen_get_window_property_iv(window,SCREEN_PROPERTY_SIZE,size)==0 && screen_create_pixmap(&pixmap,context)==0;
    if(ok) ok=screen_set_pixmap_property_iv(pixmap,SCREEN_PROPERTY_FORMAT,&format)==0
        && screen_set_pixmap_property_iv(pixmap,SCREEN_PROPERTY_USAGE,&usage)==0
        && screen_set_pixmap_property_iv(pixmap,SCREEN_PROPERTY_BUFFER_SIZE,size)==0 && screen_create_pixmap_buffer(pixmap)==0
        && screen_get_pixmap_property_pv(pixmap,SCREEN_PROPERTY_RENDER_BUFFERS,reinterpret_cast<void **>(&buffer))==0
        && screen_read_window(window,buffer,0,0,0)==0;
    unsigned char *pixels=0; int stride=0;
    if(ok) ok=screen_get_buffer_property_pv(buffer,SCREEN_PROPERTY_POINTER,reinterpret_cast<void **>(&pixels))==0
        && screen_get_buffer_property_iv(buffer,SCREEN_PROPERTY_STRIDE,&stride)==0;
    if(ok) {
        QImage image(pixels,size[0],size[1],stride,QImage::Format_ARGB32);
        ok=image.rgbSwapped().save(QDir::homePath()+"/data/qa-shot.png");
    }
    qDebug()<<"[snapshot]"<<size[0]<<size[1]<<"ok="<<ok<<"errno="<<errno;
    if(pixmap) screen_destroy_pixmap(pixmap);
    return ok;
}
static QVariantList fixtureRows() {
    bb::data::JsonDataAccess json;
    return json.load(QCoreApplication::applicationDirPath()+"/qa/fixture.json").toList();
}
Fixture::Fixture(QObject *parent, bool question, bool comments) : QObject(parent), m_model(new ArrayDataModel(this)) {
    bb::data::JsonDataAccess json;
    if (comments) {
        QVariantList rows;
        foreach (const QVariant &raw, json.load(QCoreApplication::applicationDirPath()+"/qa/comments.json").toList())
            rows << Zhihu::normalizeComment(raw.toMap());
        m_model->append(rows); return;
    }
    m_model->append(json.load(QCoreApplication::applicationDirPath()+"/qa/fixture.json").toList());
    if (question) {
        m_model->append(fixtureRows());
        m_model->append(fixtureRows());
        QVariantMap heading; heading["kind"] = "questionHeader"; heading["title"] = fixtureRows().first().toMap().value("questionTitle");
        m_model->insert(0, heading);
    }
}
Check::Check(QObject *scene, QObject *parent) : QObject(parent), m_scene(scene), m_settingsToasts(0) {
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(poll())); timer->start(250);
    QObject *client = QDeclarativeEngine::contextForObject(scene)->contextProperty("zhihu").value<QObject *>();
    connect(client, SIGNAL(toast(QString)), this, SLOT(toast(QString)));
}
void Check::toast(const QString &code) { if (code == "settings_saved") ++m_settingsToasts; }
void Check::commentProbeFinished() {
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;
    reply->deleteLater();
    bb::data::JsonDataAccess json;
    const QVariantMap data = json.loadFromBuffer(reply->readAll()).toMap();
    qDebug() << "[comment-probe]" << reply->property("probe") << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()
        << "count=" << data.value("data").toList().size() << "error=" << data.value("error").toMap().value("code")
        << "fields=" << (data.value("data").toList().isEmpty() ? QStringList() : data.value("data").toList().first().toMap().keys());
}
void Check::poll() {
    QFile command(QDir::cleanPath(QCoreApplication::applicationDirPath()+"/../../data/ui-command"));
    if (!command.open(QIODevice::ReadOnly)) return;
    const QString value = QString::fromUtf8(command.readAll()).trimmed(); command.close();
    // BarDeploy creates the file before writing the short command payload.
    if (value.isEmpty()) return;
    command.remove();
    TabbedPane *root = qobject_cast<TabbedPane *>(m_scene);
    if (!root) return;
    bool ok = true;
    if (value == "quit") { QCoreApplication::quit(); return; }
    Zhihu *client = qobject_cast<Zhihu *>(QDeclarativeEngine::contextForObject(root)->contextProperty("zhihu").value<QObject *>());
    if (value == "guest" && client) client->logout();
    if (value == "clearqaaccounts" && client) {
        const QVariantList accounts=client->accounts();
        foreach(const QVariant &account,accounts) client->removeAccount(account.toMap().value("id").toString());
    }
    if (value == "snapshot") ok=snapshot();
    if (value == "emptylibraries" && client) {
        client->write("history_"+client->scope(),QVariantList());
        client->write("saved_"+client->scope(),QVariantList());
        QMetaObject::invokeMethod(client,"libraryChanged");
    }
    if (value == "darkempty" && client) client->saveSettings(true,client->largeText());
    if (value == "lightempty" && client) client->saveSettings(false,client->largeText());
    if (value == "follow" || value == "notifications")
        root->setActiveTab(root->findChild<Tab *>(value == "follow" ? "followTab" : "notificationsTab"));
    if (value == "daily" || value == "home" || value == "fixture")
        root->setActiveTab(root->findChild<Tab *>(value == "daily" ? "dailyTab" : "homeTab"));
    NavigationPane *pane = qobject_cast<NavigationPane *>(root->activePane());
    if (value == "fixture" && pane && pane->top())
        ok = pane->top()->setProperty("feed", QVariant::fromValue(static_cast<QObject *>(new Fixture(pane->top()))));
    if (value == "hot" || value == "sections" || value == "before") {
        QString name = value == "hot" ? "dailyHotAction" : value == "sections" ? "dailySectionsAction" : "dailyBeforeAction";
        QObject *action = root->findChild<QObject *>(name);
        ok = action && QMetaObject::invokeMethod(action, "triggered");
    }
    if (value == "back" && pane && pane->count() > 1) pane->pop();
    if (value == "search") ok = QMetaObject::invokeMethod(root, "openSearch", Q_ARG(QVariant, QVariant()));
    if (value == "login") ok = QMetaObject::invokeMethod(root, "openLogin", Q_ARG(QVariant, QVariant()));
    if (value == "phonemode" && pane && pane->top()) {
        TitleBar *tabs=pane->top()->titleBar();
        ok=tabs; if(tabs) tabs->setSelectedIndex(1);
    }
    if (value == "loginlayout" && pane && pane->top()) {
        Page *page=pane->top(); QObject *field=page->findChild<QObject *>("loginSessionField");
        QObject *paste=page->findChild<QObject *>("loginPasteSession");
        ok=page->titleBar() && page->titleBar()->kind()==TitleBarKind::Segmented && !page->findChild<SegmentedControl *>()
            && field && paste && field->parent()==paste->parent();
        QObject *device=page->findChild<QObject *>("loginDeviceField"), *devicePaste=page->findChild<QObject *>("loginPasteDevice");
        ok=ok && device && devicePaste && device->parent()==devicePaste->parent();
        qDebug()<<"[login-layout] titlebar/inline paste="<<ok;
    }
    if (value == "profileempty") {
        QVariantMap user; user["name"]="QA Profile";
        ok=QMetaObject::invokeMethod(root,"openUser",Q_ARG(QVariant,QVariant(user)),Q_ARG(QVariant,QVariant()));
    }
    if (value == "profilefixture" && pane && pane->top()) {
        Fixture *fixture=new Fixture(pane->top()); fixture->model()->clear();
        ok=pane->top()->setProperty("currentFeed",QVariant::fromValue(static_cast<QObject *>(fixture)));
    }
    if (value == "profilewidth" && pane && pane->top()) {
        QObject *body=pane->top()->findChild<QObject *>("profileFeedBody");
        QObject *empty=body?body->findChild<QObject *>("emptyState"):0;
        LayoutUpdateHandler *layout=empty?empty->findChild<LayoutUpdateHandler *>():0;
        int size[2]={0,0}; screen_get_window_property_iv(Application::instance()->mainWindow()->handle(),SCREEN_PROPERTY_SIZE,size);
        ok=layout && empty->property("visible").toBool() && qAbs(layout->layoutFrame().width()-size[0])<2;
        qDebug()<<"[profile-width] viewport="<<(layout?layout->layoutFrame().width():-1)<<"window="<<size[0]<<"fill="<<ok;
    }
    if (value == "quoteexpand" && pane && pane->top()) {
        bool found=false; ok=true;
        foreach(Label *label,pane->top()->findChildren<Label *>("commentQuote")) {
            if(label->text().isEmpty()) continue;
            found=true; ok=ok && QMetaObject::invokeMethod(label,"expandQuote") && label->autoSize()->maxLineCount()==-1;
        }
        ok=ok && found;
        qDebug()<<"[comment-quote] expanded="<<ok;
    }
    if (value == "imagecard" && client && pane && pane->top()) {
        Feed *feed=qobject_cast<Feed *>(pane->top()->property("feed").value<QObject *>());
        QRegExp link("href=\"(zhihu-image:[^\"]+)\"");
        ok=feed && link.indexIn(feed->html())>=0;
        if(ok) ok=QMetaObject::invokeMethod(pane->top(),"openLink",Q_ARG(QVariant,QVariant(link.cap(1).replace("&amp;","&"))));
        qDebug()<<"[image-card] reader link routed="<<ok;
    }
    if (value == "imagecleanup" && client) ok=QMetaObject::invokeMethod(client,"cleanupImagePreview");
    if (value == "imageclose" && client) {
        bb::system::InvokeManager *manager=client->findChild<bb::system::InvokeManager *>();
        ok=manager && manager->closeChildCard();
    }
    if (value == "imagefile" && client) {
        const QString path="/accounts/1000/shared/misc/zhihu-preview/preview.png";
        QFile file(path); ok=file.open(QIODevice::ReadOnly) && file.read(8)==QByteArray::fromHex("89504e470d0a1a0a");
        qDebug()<<"[image-card] shared PNG readable="<<ok;
    }
    if (value == "imageremoved") {
        ok=!QFile::exists("/accounts/1000/shared/misc/zhihu-preview/preview.png");
        qDebug()<<"[image-card] shared copy cleaned="<<ok;
    }
    if (value == "imageinvalid" && client) {
        ok=!client->openReaderImage("https://www.zhihu.com/") && !client->openImage("file:///etc/passwd");
        qDebug()<<"[image-card] private file rejected="<<ok;
    }
    if (value == "phonecheck" && pane && pane->top()) {
        PhoneLogin *login=qobject_cast<PhoneLogin *>(pane->top()->property("phoneLogin").value<QObject *>());
        ok=login && PhoneLogin::normalizePhone(" +86 138-0013-8000 ")=="+8613800138000"
            && PhoneLogin::normalizePhone("8613800138000")=="+8613800138000" && PhoneLogin::normalizePhone("123").isEmpty();
        if(login) {
            login->requestDigits("123"); ok=ok && !login->busy() && login->error()=="invalid_phone";
            login->signIn("13800138000","bad"); ok=ok && !login->busy() && login->error()=="invalid_digits";
        }
        ok=ok && PhoneLogin::encrypt("")=="AgYDtAmQ9DcWAdIHqMAysA=="
            && PhoneLogin::encrypt("input_text=a7Bc")=="8y+nruHkWUF5iUSDK74xjA=="
            && PhoneLogin::encrypt("0123456789abcdef")=="8L02W2HSadvSGUC0LMKJMLRS8YUcplXc63hWozhpk0w="
            && PhoneLogin::encrypt("username=%2B8613800138000&client_id=8d5227e0aaaa4797a763ac64e0c3b8")=="yvCz/bLScUp6jOxNdKgjBs1iL43eot1Z0iob9p1a2HNkvUD0sMooTDezg6+MRx9rfrpipZLCnQzQsZBFmF3GGg1Cfh2x9dJM2l5W47Xm1kE="
            && PhoneLogin::hmacSha1(QByteArray(20,'\x0b'),"Hi There")=="b617318655057264e28bc0b6fb378c8ef146be00";
        qDebug()<<"[phone-local] validation/encryption/HMAC="<<ok;
    }
    if (value == "emptyhistory" || value == "emptysaved") {
        ok=QMetaObject::invokeMethod(root,"openLibrary",Q_ARG(QVariant,QVariant(value=="emptyhistory"?"history":"saved")),Q_ARG(QVariant,QVariant()));
    }
    if (value == "emptycheck" && pane && pane->top()) {
        QObject *state=0;
        foreach(QObject *candidate,pane->top()->findChildren<QObject *>("emptyState")) if(candidate->property("visible").toBool()) { state=candidate; break; }
        ok=state && !state->property("heading").toString().isEmpty() && !state->property("actionText").toString().isEmpty();
        qDebug()<<"[empty-state] visible/explanation/action="<<ok;
    }
    if (value == "emptyaction" && pane && pane->top()) {
        QObject *state=pane->top()->findChild<QObject *>("emptyState");
        ok=state && QMetaObject::invokeMethod(state,"actionTriggered");
    }
    if (value == "accounts") ok = QMetaObject::invokeMethod(root, "openAccounts", Q_ARG(QVariant, QVariant()));
    if (value == "comments") {
        QObject *action = pane && pane->top() ? pane->top()->findChild<QObject *>("detailCommentsAction") : 0;
        ok = action && QMetaObject::invokeMethod(action, "triggered");
    }
    if (value == "realdetail") {
        NavigationPane *home=root->findChild<NavigationPane *>("homeNav");
        Feed *feed=home && home->at(0) ? qobject_cast<Feed *>(home->at(0)->property("feed").value<QObject *>()) : 0;
        QVariantMap item;int best=-1;
        if(feed)for(int i=0;i<feed->count();++i){
            const QVariantMap row=feed->model()->data(QVariantList()<<i).toMap();
            if((row.value("kind")=="answer" || row.value("kind")=="article") && row.value("comments").toInt()>best){item=row;best=row.value("comments").toInt();}
        }
        ok=!item.isEmpty() && QMetaObject::invokeMethod(root,"openItem",Q_ARG(QVariant,QVariant(item)),Q_ARG(QVariant,QVariant()));
        qDebug()<<"[comments-target] expected="<<best;
    }
    if (value == "livecommentscheck" && pane && pane->top()) {
        Feed *feed=qobject_cast<Feed *>(pane->top()->property("feed").value<QObject *>());
        ok=feed && feed->loaded() && !feed->busy() && feed->error().isEmpty();
        qDebug()<<"[live-comments] count="<<(feed?feed->count():-1)<<"more="<<(feed?feed->hasMore():false)<<"error="<<(feed?feed->error():"missing");
    }
    if (value == "livereply" && pane && pane->top()) {
        ListView *list=pane->top()->findChild<ListView *>("commentsList");
        ok=false;
        if(list && list->dataModel())for(int i=0;i<list->dataModel()->childCount(QVariantList());++i){
            if(list->dataModel()->data(QVariantList()<<i).toMap().value("children").toInt()>0){
                ok=QMetaObject::invokeMethod(list,"triggered",Q_ARG(QVariantList,QVariantList()<<i));break;
            }
        }
    }
    if (value == "commentmore" && pane && pane->top()) ok=QMetaObject::invokeMethod(pane->top(),"loadMore");
    if (value == "commentfixture" && pane && pane->top())
        ok = pane->top()->setProperty("feed", QVariant::fromValue(static_cast<QObject *>(new Fixture(pane->top(), false, true))));
    if (value == "commentscheck" && pane && pane->top()) {
        ListView *list = pane->top()->findChild<ListView *>("commentsList");
        ok = list && list->dataModel() && list->dataModel()->childCount(QVariantList()) == 4
            && !pane->top()->findChild<SegmentedControl *>() && pane->top()->titleBar()->kind() != TitleBarKind::Segmented;
        if (list) {
            const QVariantMap first = list->dataModel()->data(QVariantList()<<0).toMap();
            ok = ok && first.value("likes").toInt() == 12 && first.value("children").toInt() == 2
                && !first.value("content").toString().contains("<p>");
        }
        qDebug() << "[comments-check] normalized rows and no segmented control=" << ok;
    }
    if (value == "commentreplies" && pane && pane->top()) {
        ListView *list = pane->top()->findChild<ListView *>("commentsList");
        ok = list && QMetaObject::invokeMethod(list,"triggered", Q_ARG(QVariantList,QVariantList()<<0));
        ok = ok && pane->top()->property("target").toMap().value("kind") == "commentReplies";
    }
    if (value == "commentprobe" && client) {
        const QString id = "2094699456440673419";
        const QStringList paths = QStringList() << "comment_v5/answers/"+id+"/root_comment?order_by=score&limit=20&offset="
            << "answers/"+id+"/root_comments?order=normal&limit=20&offset=0" << "answers/"+id+"/comments?limit=20&offset=0";
        for(int i=0;i<paths.size();++i) {
            QNetworkReply *reply=client->get("http://api.zhihu.com/"+paths[i],"comment-probe",this,SLOT(commentProbeFinished()));
            if(reply) reply->setProperty("probe",i);
        }
    }
    if (value == "dailycomments") {
        QVariantMap item;item["kind"]="daily";item["id"]="9793487";
        ok=QMetaObject::invokeMethod(root,"openComments",Q_ARG(QVariant,QVariant(item)),Q_ARG(QVariant,QVariant()));
    }
    if (value == "originaldailycomments" && pane && pane->top())
        ok=QMetaObject::invokeMethod(pane->top(),"goCommentsPage");
    if (value == "grayauthor" && client) {
        QVariantMap item; item["author"]="QA Author";
        QVariantMap data; data["content"]="<p>Body text</p>";
        const QString html=client->document(data,item);
        ok=html.contains(".reader-author{color:#888}") && html.contains("<p class=\"reader-author\">QA Author</p>");
        qDebug()<<"[author-gray]"<<ok;
    }
    if (value == "clipboard" && client && pane && pane->top()) {
        const QString synthetic = "_xsrf=qa; d_c0=qa-device|123; z_c0=synthetic-credential";
        bb::system::Clipboard clipboard;
        const QByteArray original = clipboard.value("text/plain");
        clipboard.insert("text/plain", synthetic.toUtf8());
        QObject *paste = pane->top()->findChild<QObject *>("loginPasteSession");
        QObject *field = pane->top()->findChild<QObject *>("loginSessionField");
        QObject *device = pane->top()->findChild<QObject *>("loginDeviceField");
        ok = paste && field && device && QMetaObject::invokeMethod(paste, "clicked") && field->property("text").toString() == "synthetic-credential"
            && device->property("text").toString()=="qa-device|123";
        if (field) field->setProperty("text", "");
        if (device) device->setProperty("text", "");
        const int originalCount=client->accounts().size();
        client->verifyCookieValues("z_c0=bad; injected=value","device");
        ok=ok && !client->verifying() && client->accountError()=="invalid_cookie_values" && client->accounts().size()==originalCount;
        ok=ok && client->splitCookieValues("z_c0=one=two|three; d_c0=device=|123").value("z_c0")=="one=two|three";
        QSettings settings(QDir::homePath()+"/data/settings.ini", QSettings::IniFormat);
        const QVariant oldAccounts = settings.value("accounts"), oldActive = settings.value("active");
        QVariantMap a; a["id"] = "qa-one"; a["name"] = "QA"; a["cookie"] = synthetic; a["mobile"]=QVariantMap();
        QVariantMap b = a; b["id"] = "qa-two";
        settings.setValue("accounts", QVariantList()<<a<<b); settings.setValue("active", "qa-one"); settings.sync();
        {
            Zhihu probe;
            ok = ok && probe.copyAccountCredential("qa-one") && probe.clipboardText() == synthetic
                && !probe.account().contains("cookie") && !probe.accounts().first().toMap().contains("cookie")
                && !probe.account().contains("mobile") && !probe.accounts().first().toMap().contains("mobile");
            probe.removeAccount("qa-one");
            ok = ok && !probe.loggedIn() && probe.accounts().size() == 1 && probe.accounts().first().toMap().value("id") == "qa-two";
        }
        settings.setValue("accounts", oldAccounts); settings.setValue("active", oldActive); settings.sync();
        if (original.isEmpty()) clipboard.remove("text/plain"); else clipboard.insert("text/plain", original);
        ok = ok && Zhihu::normalizedCookie("\"_xsrf=qa; d_c0=qa|123\"") == "_xsrf=qa; d_c0=qa|123"
            && Zhihu::normalizedCookie("z_c0=qa\r\nX-Test: injected").isEmpty();
        qDebug() << "[credentials] paste/copy/delete/masking/console format=" << ok;
    }
    if (value == "question") {
        QVariantMap question; question["kind"]="question"; question["id"]="1234";
        bb::data::JsonDataAccess json;
        question["title"] = fixtureRows().first().toMap().value("questionTitle");
        ok = QMetaObject::invokeMethod(root, "openItem", Q_ARG(QVariant,QVariant(question)), Q_ARG(QVariant,QVariant()));
        pane = qobject_cast<NavigationPane *>(root->activePane());
        if (pane && pane->top()) pane->top()->setProperty("feed", QVariant::fromValue(static_cast<QObject *>(new Fixture(pane->top(), true))));
    }
    if (value == "questioncheck" || value == "questionscroll") {
        ListView *list = pane && pane->top() ? pane->top()->findChild<ListView *>("feedList") : 0;
        ok = list && !list->property("showTitles").toBool() && list->dataModel()->data(QVariantList()<<0).toMap().value("kind") == "questionHeader"
            && pane->top()->property("kind").toString() == "question";
        if (value == "questionscroll" && list) list->scrollToItem(QVariantList()<<(list->dataModel()->childCount(QVariantList())-1), ScrollAnimation::None);
        qDebug() << "[question] row titles hidden and scrolling leading visual=" << ok;
    }
    if (value == "profile") {
        NavigationPane *home = root->findChild<NavigationPane *>("homeNav");
        Feed *feed = home && home->at(0) ? qobject_cast<Feed *>(home->at(0)->property("feed").value<QObject *>()) : 0;
        QVariantMap author;
        if (feed) for (int i=0;i<feed->count();++i) {
            QVariantMap row = feed->model()->data(QVariantList()<<i).toMap();
            if (row.value("author_token").toString().isEmpty()) continue;
            author["id"]=row.value("author_id"); author["url_token"]=row.value("author_token");
            author["name"]=row.value("author"); author["avatar_url"]=row.value("author_avatar"); break;
        }
        ok = !author.isEmpty() && QMetaObject::invokeMethod(root, "openUser", Q_ARG(QVariant,QVariant(author)), Q_ARG(QVariant,QVariant()));
    }
    if (value == "authenticate" && client) {
        QFile file(QDir::homePath()+"/qa-cookie");
        ok = file.open(QIODevice::ReadOnly);
        const QString cookie = QString::fromUtf8(file.readAll()); file.close(); file.remove();
        if (ok) client->verifyAccount(cookie);
    }
    if (value == "authcheck" && client) {
        ok = client->loggedIn() && !client->verifying() && client->accountError().isEmpty();
        qDebug() << "[auth-check] authenticated=" << client->loggedIn() << "error=" << client->accountError();
    }
    if (value == "profileanswers" || value == "profilearticles") {
        SegmentedControl *tabs = pane && pane->top() ? pane->top()->findChild<SegmentedControl *>("profileTabs") : 0;
        if (tabs) tabs->setSelectedIndex(value == "profileanswers" ? 1 : 2);
        ok = tabs;
    }
    if (value == "profilecheck") {
        Feed *profile = pane && pane->top() ? qobject_cast<Feed *>(pane->top()->property("profileFeed").value<QObject *>()) : 0;
        Feed *feed = pane && pane->top() ? qobject_cast<Feed *>(pane->top()->property("currentFeed").value<QObject *>()) : 0;
        ok = profile && profile->loaded() && profile->error().isEmpty() && !profile->profile().value("id").toString().isEmpty()
            && feed && feed->loaded() && feed->error().isEmpty();
        qDebug() << "[profile]" << "profile error=" << (profile ? profile->error() : "missing") << "list error=" << (feed ? feed->error() : "missing") << "count=" << (feed ? feed->count() : -1);
    }
    if (value == "silentsettings" && client) {
        client->saveSettings(!client->dark(), client->largeText());
        ok = m_settingsToasts == 0;
        qDebug() << "[settings] save toasts=" << m_settingsToasts;
    }
    if (value == "settings") ok = QMetaObject::invokeMethod(root, "openSettings", Q_ARG(QVariant, QVariant()));
    if (value == "history" || value == "saved")
        ok = QMetaObject::invokeMethod(root, "openLibrary", Q_ARG(QVariant, QVariant(value)), Q_ARG(QVariant, QVariant()));
    if (value == "libraryseed" && client) {
        QVariantList rows = fixtureRows();
        for (int i = 0; i < rows.size(); ++i) {
            QVariantMap row = rows[i].toMap();
            if (i < 2) {
                const qint64 time = QDateTime::currentDateTime().addDays(-i).toMSecsSinceEpoch();
                row["visitedAt"] = time; row["savedAt"] = time;
            }
            rows[i] = row;
        }
        client->write("history_" + client->scope(), rows);
        client->write("saved_" + client->scope(), rows);
        QMetaObject::invokeMethod(client, "libraryChanged");
    }
    if (value == "groups" && pane && pane->top()) {
        Feed *feed = qobject_cast<Feed *>(pane->top()->property("feed").value<QObject *>());
        DataModel *model = feed ? feed->model() : 0;
        ok = model && feed->count() == 3 && model->childCount(QVariantList()) == 3;
        const QStringList dates = QStringList() << QDate::currentDate().toString(Qt::ISODate)
            << QDate::currentDate().addDays(-1).toString(Qt::ISODate) << "0000-00-00";
        if (ok) for (int i = 0; i < 3; ++i) {
            ok = ok && model->itemType(QVariantList()<<i) == "header"
                && model->data(QVariantList()<<i).toString() == dates[i]
                && model->childCount(QVariantList()<<i) == 1
                && model->data(QVariantList()<<i<<0).toMap().value("id").toString() == QString("fixture-%1").arg(i+1);
        }
        ListView *list = pane->top()->findChild<ListView *>("feedList");
        const int before = pane->count();
        ok = ok && list && QMetaObject::invokeMethod(list, "triggered", Q_ARG(QVariantList, QVariantList()<<0));
        ok = ok && pane->count() == before;
        qDebug() << "[library-groups]" << (feed ? feed->count() : -1) << "dated/undated, header click ignored=" << ok;
    }
    if (value == "content" && client) {
        const QVariantMap item = fixtureRows().first().toMap();
        const QString source = "http://picx.zhimg.com/qa-native-cache.png";
        const QString imagePath = QDir::homePath() + "/data/images/" + sha1(source) + ".jpg";
        QFile::remove(imagePath);
        ok = QFile::copy(QCoreApplication::applicationDirPath()+"/assets/daily/images/app/logo_144x144.png", imagePath);
        QVariantMap detail; detail["title"] = item.value("title");
        detail["title"] = "Cached article fixture";
        detail["content"] = "<img src=\"placeholder\" data-original=\"" + source + "\"><p>" + item.value("summary").toString() + "</p>";
        QVariantMap cache; cache["detail"] = detail;
        client->write("feed_" + client->scope() + "_" + sha1("detail:" + Zhihu::identity(item)), cache);
        ok = ok && QMetaObject::invokeMethod(root, "openItem", Q_ARG(QVariant, QVariant(item)), Q_ARG(QVariant, QVariant()));
    }
    if (value == "contentcheck" && pane && pane->top()) {
        Feed *feed = qobject_cast<Feed *>(pane->top()->property("feed").value<QObject *>());
        ok = feed && feed->cached() && feed->html().contains("file:///")
            && feed->html().contains(fixtureRows().first().toMap().value("summary").toString())
            && !feed->html().contains("src=\"placeholder\"");
        qDebug() << "[library-content] cached text and local inline image=" << ok;
    }
    if ((value == "darknews" || value == "lightnews") && client && pane && pane->top()) {
        const bool dark = value == "darknews";
        client->saveSettings(dark, client->largeText());
        ok = pane->top()->property("isNewsEyeProtectionMode").isValid()
            && pane->top()->property("isNewsEyeProtectionMode").toBool() == dark;
        qDebug() << "[daily-theme] live eye protection=" << pane->top()->property("isNewsEyeProtectionMode");
    }
    if (value == "theme" && pane && pane->top()) {
        DropDown *theme = pane->top()->findChild<DropDown *>("settingsTheme");
        const bool wasDark = client && client->dark();
        if (theme) theme->setSelectedIndex(wasDark ? 0 : 1);
        ok = theme && client && client->dark() != wasDark;
        if (theme) theme->setSelectedIndex(1);
        ok = ok && client->dark();
    }
    if (value == "bright" && client) {
        client->saveSettings(false, client->largeText());
        ok = !client->dark();
    }
    if (value == "preferences" && pane && pane->top()) {
        DropDown *limit = pane->top()->findChild<DropDown *>("settingsCacheLimit");
        ToggleButton *large = pane->top()->findChild<ToggleButton *>("settingsLargeText");
        if (limit) limit->setSelectedIndex(1);
        if (large) large->setChecked(true);
        QSettings daily;
        ok = limit && large && client && client->cacheLimitMb() == 1024 && client->largeText()
            && daily.value("newsLargeFont").toString() == "1";
    }
    if (value == "restored" && pane && pane->top()) {
        DropDown *theme = pane->top()->findChild<DropDown *>("settingsTheme");
        DropDown *limit = pane->top()->findChild<DropDown *>("settingsCacheLimit");
        ToggleButton *large = pane->top()->findChild<ToggleButton *>("settingsLargeText");
        ToggleButton *eye = pane->top()->findChild<ToggleButton *>("settingsDailyEye");
        ok = theme && limit && large && !eye && theme->selectedIndex() == 1 && limit->selectedIndex() == 1
            && large->isChecked() && client->dark() && client->cacheLimitMb() == 1024;
    }
    if (value == "cache" && client && pane && pane->top()) {
        // Daily image caches are normally created only when their tab is rendered.
        WebImageView ensureImageCache;
        const QString dir = QDir::homePath() + "/data/images/";
        QFile oversized(dir + "qa_old.jpg");
        bool seeded = oversized.open(QIODevice::WriteOnly) && oversized.seek(129 * 1024 * 1024)
            && oversized.write("x", 1) == 1;
        oversized.close();
        client->setCacheLimitMb(512);
        const bool bounded = seeded && !QFile::exists(oversized.fileName());
        client->setCacheLimitMb(1024);
        QVariantMap sentinel; sentinel["qa"] = "keep";
        const QVariant history = client->read("history_" + client->scope());
        const QVariant saved = client->read("saved_" + client->scope());
        client->write("history_qa", sentinel); client->write("saved_qa", sentinel);
        client->write("feed_qa_notifications", sentinel);
        const QUrl dailyUrl("https://news-at.zhihu.com/qa-cache-check");
        int dailySeeds = 0;
        foreach (QNetworkDiskCache *cache, QCoreApplication::instance()->findChildren<QNetworkDiskCache *>()) {
            QNetworkCacheMetaData metadata;
            metadata.setUrl(dailyUrl); metadata.setSaveToDisk(true);
            metadata.setExpirationDate(QDateTime::currentDateTime().addDays(1));
            QIODevice *device = cache->prepare(metadata);
            if (device) { device->write(QByteArray(1024 * 1024, 'x')); cache->insert(device); ++dailySeeds; }
        }
        QFile avatar(dir + "qa_avatar.jpg");
        seeded = avatar.open(QIODevice::WriteOnly) && avatar.write("avatar", 6) == 6; avatar.close();
        const QString inlinePath = dir + sha1("http://picx.zhimg.com/qa-native-cache.png") + ".jpg";
        if (!QFile::exists(inlinePath)) QFile::copy(QCoreApplication::applicationDirPath()+"/assets/icons/ic_favorite.png", inlinePath);
        QFile settings(QDir::homePath() + "/data/settings.ini");
        settings.open(QIODevice::ReadOnly); const QByteArray original = settings.readAll(); settings.close();
        client->refreshCacheUsage(); qDebug() << "[cache-before]" << client->cacheUsageText();
        QObject *clear = pane->top()->findChild<QObject *>("settingsClearCache");
        ok = clear && QMetaObject::invokeMethod(clear, "clicked");
        settings.open(QIODevice::ReadOnly); const bool settingsKept = settings.readAll() == original;
        const bool kept = client->read("history_qa").toMap() == sentinel && client->read("saved_qa").toMap() == sentinel && settingsKept
            && client->read("history_" + client->scope()) == history && client->read("saved_" + client->scope()) == saved;
        const bool cleared = !QFile::exists(avatar.fileName()) && !QFile::exists(client->storage("feed_qa_notifications"));
        const QVariantMap contentItem = fixtureRows().first().toMap();
        const bool contentCleared = !QFile::exists(client->storage("feed_" + client->scope() + "_" + sha1("detail:" + Zhihu::identity(contentItem))))
            && !QFile::exists(dir + sha1("http://picx.zhimg.com/qa-native-cache.png") + ".jpg");
        bool dailyCleared = dailySeeds == 2;
        foreach (QNetworkDiskCache *cache, QCoreApplication::instance()->findChildren<QNetworkDiskCache *>())
            dailyCleared = dailyCleared && !cache->metaData(dailyUrl).isValid();
        client->refreshCacheUsage(); qDebug() << "[cache-after]" << client->cacheUsageText();
        qDebug() << "[cache-check] bounded=" << bounded << "cleared=" << cleared << "daily stores=" << dailySeeds << "daily=" << dailyCleared << "library/settings kept=" << kept << "library content cleared=" << contentCleared;
        ok = ok && seeded && bounded && cleared && dailyCleared && kept && contentCleared;
    }
    if (value == "query" && pane && pane->top()) {
        QObject *field = pane->top()->findChild<QObject *>("searchQuery");
        if (field) field->setProperty("text", "BlackBerry");
        ok = field && QMetaObject::invokeMethod(pane->top(), "search");
    }
    if (value == "first" && pane && pane->top()) {
        ListView *list = pane->top()->findChild<ListView *>("dailyStoryList");
        if (!list) list = pane->top()->findChild<ListView *>("dailySectionGrid");
        ok = list && list->dataModel() && list->dataModel()->childCount(QVariantList()) > 0;
        if (ok) {
            int index = 0;
            while (index < list->dataModel()->childCount(QVariantList()) &&
                   list->dataModel()->data(QVariantList()<<index).toMap().contains("__type")) ++index;
            ok = QMetaObject::invokeMethod(list, "triggered", Q_ARG(QVariantList, QVariantList()<<index));
        }
    }
    if ((value == "previousday" || value == "nextday") && pane && pane->top())
        ok = QMetaObject::invokeMethod(pane->top(), "changeDate", Q_ARG(QVariant, QVariant(value == "previousday" ? -1 : 1)));
    pane = qobject_cast<NavigationPane *>(root->activePane());
    if (pane && pane->top()) {
        QStringList names;
        for (int i = 0; i < pane->top()->actionCount(); ++i) names << pane->top()->actionAt(i)->objectName();
        qDebug() << "[actions]" << pane->top()->property("kind") << pane->top()->actionCount() << names;
    }
    qDebug() << "[ui-check]" << value << "ok=" << ok;
}
Q_DECL_EXPORT int main(int argc, char **argv) {
    qInstallMsgHandler(message);
    qmlRegisterType<QTimer>("tech.lwl", 1, 0, "QTimer");
    qmlRegisterType<WebImageView>("tech.lwl", 1, 0, "WebImageView");
    qmlRegisterType<Requester>("tech.lwl", 1, 0, "Requester");
    Application app(argc, argv);
    ApplicationUI ui;
    Check check(app.scene());
    return Application::exec();
}

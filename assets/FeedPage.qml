import bb.cascades 1.4
Page {
    id: page
    property string kind: "home"
    property string parameter: ""
    property string pageTitle: ""
    property variant appRoot: null
    property variant navigator: null
    property variant feed: null
    property bool autoLoad: true
    property bool actionsReady: false
    property bool libraryActionsAdded: false
    function syncActions() {
        var show = kind === "home" || kind === "follow";
        if (show === libraryActionsAdded) return;
        if (show) {
            addAction(searchAction, ActionBarPlacement.Signature);
            addAction(historyAction, ActionBarPlacement.OnBar);
            addAction(savedAction, ActionBarPlacement.OnBar);
        } else {
            removeAction(searchAction);
            removeAction(historyAction);
            removeAction(savedAction);
        }
        libraryActionsAdded = show;
    }
    onKindChanged: { if (actionsReady) syncActions(); }
    titleBar: TitleBar { title: page.pageTitle; visibility: page.kind !== "question" && page.pageTitle.length > 0 ? ChromeVisibility.Visible : ChromeVisibility.Hidden }
    function ensureLoaded() { if (feed && !feed.loaded && !feed.busy) feed.load(false); }
    function setup(newKind, newParameter, title) {
        var old = feed;
        kind = newKind;
        parameter = newParameter;
        pageTitle = title;
        feed = zhihu.createFeed(kind, parameter, page);
        if (kind === "question") feed.heading = pageTitle;
        if (old) zhihu.releaseFeed(old);
        ensureLoaded();
    }
    function reload() { contentBody.refresh(); }
    onCreationCompleted: {
        actionsReady = true;
        syncActions();
        feed = zhihu.createFeed(kind, parameter, page);
        if (kind === "question") feed.heading = pageTitle;
        if (autoLoad) ensureLoaded();
    }
    actions: [
        ActionItem {
            title: "重新加载"
            imageSource: "asset:///icons/ic_resume.png"
            ActionBar.placement: ActionBarPlacement.InOverflow
            enabled: !page.feed || !page.feed.busy
            onTriggered: page.reload()
        }
    ]
    attachedObjects: [
        ActionItem {
            id: searchAction
            objectName: "feedSearchAction"
            title: "搜索"
            imageSource: "asset:///icons/action_search.png"
            ActionBar.placement: ActionBarPlacement.Signature
            onTriggered: page.appRoot.openSearch(page.navigator)
        },
        ActionItem {
            id: historyAction
            objectName: "feedHistoryAction"
            title: "最近浏览"
            imageSource: "asset:///icons/ic_history.png"
            ActionBar.placement: ActionBarPlacement.OnBar
            onTriggered: page.appRoot.openLibrary("history", page.navigator)
        },
        ActionItem {
            id: savedAction
            objectName: "feedSavedAction"
            title: "收藏夹"
            imageSource: "asset:///icons/ic_favorite.png"
            ActionBar.placement: ActionBarPlacement.OnBar
            onTriggered: page.appRoot.openLibrary("saved", page.navigator)
        }
    ]
    FeedBody {
        id: contentBody
        feed: page.feed
        questionTitle: page.kind === "question" ? page.pageTitle : ""
        showTitles: page.kind !== "question"
        emptyTitle: page.kind === "history" ? "还没有浏览记录" : page.kind === "saved" ? "还没有收藏" : page.kind === "notifications" || page.kind === "notificationItems" ? "暂时没有通知" : page.kind === "question" ? "还没有回答" : "暂时没有内容"
        emptyIcon: page.kind === "history" ? "asset:///icons/ic_history.png" : page.kind === "saved" ? "asset:///icons/ic_favorite.png" : page.kind === "notifications" || page.kind === "notificationItems" ? "asset:///icons/ca_notification_focus.png" : "asset:///icons/ic_sb_notes.png"
        emptyBrowse: page.kind === "history" || page.kind === "saved"
        emptyAction: emptyBrowse ? "浏览首页" : "重新加载"
        emptyMessage: page.kind === "history" ? "还没有浏览记录。打开一篇内容后会保存在这里。" :
                   (page.kind === "saved" ? "还没有收藏。阅读时可从底部菜单收藏内容。" :
                   (page.kind === "search" ? "没有找到相关内容。" : "这里还没有内容。"))
        onOpenItem: page.appRoot.openItem(item, page.navigator)
        onOpenUser: page.appRoot.openUser(user, page.navigator)
        onConfigure: page.appRoot.openSettings(page.navigator)
        onLogin: page.appRoot.openAccounts(page.navigator)
        onBrowse: page.appRoot.browseHome()
    }
}

import bb.cascades 1.4
import bb.system 1.2
import "Messages.js" as Messages
import "daily/common" as DailyCommon
import "daily/pages" as DailyPages

// Preserve the five destinations and action placements supplied by the user.
// Descriptions show verified counts only; account management follows BBTiebaUI.
TabbedPane {
    id: root
    property bool ready: false
    property bool homeReturnInProgress: false
    // The imported Daily pages use their original common/nav context contract.
    property variant nav: dailyNav
    showTabsOnActionBar: false
    function currentNavigator() {
        if (activeTab === followTab) return followNav;
        if (activeTab === dailyTab) return dailyNav;
        if (activeTab === notificationsTab) return notificationsNav;
        if (activeTab === accountTab) return accountNav;
        return homeNav;
    }
    function push(definition, nav) {
        var target = nav || currentNavigator();
        var page = definition.createObject(target);
        page.appRoot = root;
        page.navigator = target;
        console.log("[navigation bindings]", page.appRoot === root, page.navigator === target);
        target.push(page);
        return page;
    }
    function openSettings(nav) { return push(settingsDef, nav); }
    function openAccounts(nav) { return push(accountsDef, nav); }
    function openLogin(nav) { return push(loginDef, nav); }
    function openSearch(nav) { return push(searchDef, nav); }
    function browseHome() {
        activeTab = homeTab;
        homeReturnInProgress = homeNav.count() > 1;
        if (homeReturnInProgress) homeNav.pop();
        homePage.ensureLoaded();
    }
    function openComments(item, nav) {
        var page = push(commentsDef, nav);
        page.initialize(item);
        return page;
    }
    function openUser(user, nav) {
        var page = push(userDef, nav);
        page.userData = user;
        return page;
    }
    function openLibrary(kind, nav) {
        var page = push(feedDef, nav);
        page.setup(kind, "", kind === "history" ? "最近浏览" : "收藏夹（本地）");
    }
    function openItem(item, nav) {
        if (item.kind === "question" || item.kind === "notificationGroup") {
            var page = push(feedDef, nav);
            page.setup(item.kind === "question" ? "question" : "notificationItems", item.id.toString(),
                       item.title || (item.kind === "question" ? "问题回答" : "通知"));
        } else if (item.kind === "notification") {
            if (item.url) zhihu.openBrowser(item.url);
        } else {
            var target = nav || currentNavigator();
            var detail = detailDef.createObject(target);
            detail.appRoot = root;
            detail.navigator = target;
            console.log("[detail bindings]", detail.appRoot === root, detail.navigator === target);
            detail.initialize(item);
            target.push(detail);
        }
    }
    function refreshVisible() {
        if (!ready) return;
        if (activeTab === homeTab) homePage.ensureLoaded();
        if (activeTab === followTab) followPage.ensureLoaded();
        if (activeTab === notificationsTab) notificationsPage.ensureLoaded();
        if (activeTab === accountTab) accountPage.ensureLoaded();
    }
    onActiveTabChanged: { if (ready) { refreshVisible(); zhihu.refreshCounters(); } }
    Menu.definition: MenuDefinition {
        settingsAction: SettingsActionItem { title: "设置"; onTriggered: root.openSettings() }
        helpAction: HelpActionItem { title: "关于"; onTriggered: root.push(aboutDef) }
        actions: [ ActionItem { title: "账号管理"; imageSource: "asset:///icons/ic_accounts.png"; onTriggered: root.openAccounts() } ]
    }
    tabs: [
        Tab {
            id: homeTab
            objectName: "homeTab"
            title: "首页"
            imageSource: "asset:///icons/ic_home.png"
            NavigationPane {
                id: homeNav
                objectName: "homeNav"
                onPopTransitionEnded: {
                    page.destroy();
                    if (root.homeReturnInProgress && homeNav.count() > 1) homeNav.pop();
                    else root.homeReturnInProgress = false;
                }
                FeedPage { id: homePage; appRoot: root; navigator: homeNav; kind: "home" }
            }
        },
        Tab {
            id: followTab
            objectName: "followTab"
            title: "关注"
            description: zhihu.followUnread > 0 ? zhihu.followUnread + "条更新" : ""
            imageSource: "asset:///icons/ic_link_contacts.png"
            NavigationPane {
                id: followNav
                onPopTransitionEnded: { page.destroy(); }
                FeedPage { id: followPage; appRoot: root; navigator: followNav; kind: "follow"; autoLoad: false }
            }
        },
        Tab {
            id: dailyTab
            objectName: "dailyTab"
            title: "日报"
            imageSource: "asset:///icons/ic_sb_notes.png"
            NavigationPane {
                id: dailyNav
                objectName: "dailyNav"
                onPopTransitionEnded: { page.destroy(); }
                DailyPages.index { id: dailyPage }
            }
        },
        Tab {
            id: notificationsTab
            objectName: "notificationsTab"
            title: "通知"
            description: zhihu.notificationUnread > 0 ? zhihu.notificationUnread + "条未读" : ""
            imageSource: "asset:///icons/ca_notification_focus.png"
            NavigationPane {
                id: notificationsNav
                onPopTransitionEnded: { page.destroy(); }
                FeedPage { id: notificationsPage; appRoot: root; navigator: notificationsNav; kind: "notifications"; autoLoad: false }
            }
        },
        Tab {
            id: accountTab
            objectName: "accountTab"
            title: "账号"
            description: zhihu.loggedIn ? zhihu.account.name : ""
            imageSource: "asset:///icons/ic_contact.png"
            NavigationPane {
                id: accountNav
                objectName: "accountNav"
                onPopTransitionEnded: { page.destroy(); }
                UserDetailPage { id: accountPage; appRoot: root; navigator: accountNav; ownProfile: true; autoLoad: false }
            }
        }
    ]
    onCreationCompleted: {
        ready = true;
        OrientationSupport.supportedDisplayOrientation = SupportedDisplayOrientation.DisplayPortrait;
        zhihu.toast.connect(function(code) { notice.body = Messages.message(code); notice.show(); });
        zhihu.sessionChanged.connect(refreshVisible);
        zhihu.refreshCounters();
    }
    attachedObjects: [
        DailyCommon.Common { id: common },
        SystemToast { id: notice; body: "" },
        ComponentDefinition { id: settingsDef; source: "SettingsPage.qml" },
        ComponentDefinition { id: accountsDef; source: "AccountsPage.qml" },
        ComponentDefinition { id: userDef; source: "UserDetailPage.qml" },
        ComponentDefinition { id: loginDef; source: "LoginPage.qml" },
        ComponentDefinition { id: searchDef; source: "SearchPage.qml" },
        ComponentDefinition { id: aboutDef; source: "AboutPage.qml" },
        ComponentDefinition { id: feedDef; FeedPage { autoLoad: false } },
        ComponentDefinition { id: detailDef; source: "DetailPage.qml" },
        ComponentDefinition { id: commentsDef; source: "CommentsPage.qml" }
    ]
}

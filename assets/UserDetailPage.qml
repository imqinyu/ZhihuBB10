// Layout adapted from BBTiebaUI UserDetailPage.qml.
import bb.cascades 1.4
import tech.lwl 1.0

Page {
    id: userPage
    property variant appRoot: null
    property variant navigator: null
    property bool ownProfile: false
    property bool autoLoad: true
    property variant userData: ({})
    property variant baseProfile: ownProfile ? zhihu.account : userData
    property string userId: (baseProfile.url_token || baseProfile.id || "").toString()
    property variant profileFeed: null
    property variant feeds: []
    property variant currentFeed: null
    property variant profile: mergedProfile(baseProfile, profileFeed ? profileFeed.profile : ({}))
    property bool ready: false
    property string avatar: readAvatar(zhihu.imageRevision, profile.avatar_url || "")
    function readAvatar(revision, url) { return zhihu.image(url); }
    function mergedProfile(base, remote) {
        var result = {}, key;
        for (key in base) result[key] = base[key];
        for (key in remote) if (remote[key] !== null && remote[key] !== "") result[key] = remote[key];
        return result;
    }
    function statistic(value) { return value === undefined || value === null ? "—" : value.toString(); }
    function scheduleReload() { if (ready && autoLoad) reloadTimer.start(); }
    function ensureLoaded() {
        autoLoad = true;
        if (!profileFeed) scheduleReload();
        else selectFeed();
    }
    function rebuild() {
        currentFeed = null;
        for (var i = 0; i < feeds.length; ++i) zhihu.releaseFeed(feeds[i]);
        feeds = [];
        if (profileFeed) zhihu.releaseFeed(profileFeed);
        profileFeed = null;
        if (!userId) return;
        profileFeed = zhihu.createFeed("profile", userId, userPage);
        feeds = [zhihu.createFeed("memberQuestions", userId, userPage),
                 zhihu.createFeed("memberAnswers", userId, userPage),
                 zhihu.createFeed("memberArticles", userId, userPage)];
        profileFeed.load(false);
        selectFeed();
    }
    function selectFeed() {
        if (!ready || feeds.length !== 3) return;
        currentFeed = feeds[profileTabs.selectedIndex];
        if (!currentFeed.loaded && !currentFeed.busy) currentFeed.load(false);
    }
    onUserIdChanged: scheduleReload()
    onCreationCompleted: {
        ready = true;
        zhihu.sessionChanged.connect(scheduleReload);
        scheduleReload();
    }
    actions: [
        ActionItem {
            title: "账号管理"
            imageSource: "asset:///icons/ic_contact.png"
            ActionBar.placement: ActionBarPlacement.Signature
            onTriggered: userPage.appRoot.openAccounts(userPage.navigator)
        },
        ActionItem {
            title: "重新加载"
            imageSource: "asset:///icons/ic_resume.png"
            ActionBar.placement: ActionBarPlacement.InOverflow
            onTriggered: {
                if (userPage.profileFeed && !userPage.profileFeed.busy) userPage.profileFeed.load(false);
                if (userPage.currentFeed && !userPage.currentFeed.busy) userPage.currentFeed.load(false);
            }
        }
    ]
    Container {
        horizontalAlignment: HorizontalAlignment.Fill
        Container {
            preferredHeight: ui.du(25)
            horizontalAlignment: HorizontalAlignment.Fill
            layout: DockLayout {}
            ImageView {
                imageSource: "asset:///images/profile_background.jpg"
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                scalingMethod: ScalingMethod.AspectFill
            }
            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Bottom
                leftPadding: ui.du(2)
                rightPadding: ui.du(2)
                bottomPadding: ui.du(2)
                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                ImageView {
                    imageSource: userPage.avatar.length > 0 ? userPage.avatar : "asset:///icons/ic_contact.png"
                    preferredWidth: ui.du(12)
                    minWidth: ui.du(12)
                    maxWidth: ui.du(12)
                    preferredHeight: ui.du(12)
                    minHeight: ui.du(12)
                    maxHeight: ui.du(12)
                    scalingMethod: ScalingMethod.AspectFill
                }
                Container {
                    leftPadding: ui.du(1.5)
                    verticalAlignment: VerticalAlignment.Center
                    layoutProperties: StackLayoutProperties { spaceQuota: 1 }
                    Label {
                        text: userPage.profile.name || (userPage.ownProfile ? "未登录" : "用户")
                        textFormat: TextFormat.Plain
                        textStyle.color: Color.White
                        textStyle.fontSize: FontSize.Medium
                        autoSize.maxLineCount: 1
                    }
                    Label {
                        text: "关注 " + userPage.statistic(userPage.profile.following_count) + "  粉丝 " + userPage.statistic(userPage.profile.follower_count) + "  获赞 " + userPage.statistic(userPage.profile.voteup_count)
                        textStyle.color: Color.White
                        textStyle.fontSize: FontSize.XSmall
                    }
                }
            }
        }
        SegmentedControl {
            id: profileTabs
            objectName: "profileTabs"
            selectedIndex: 0
            options: [Option { text: "提问" }, Option { text: "回答" }, Option { text: "文章" }]
            onSelectedIndexChanged: userPage.selectFeed()
        }
        EmptyState {
            visible: userPage.ownProfile && !zhihu.loggedIn
            layoutProperties: StackLayoutProperties { spaceQuota: 1 }
            heading: "登录后查看个人内容"
            message: "登录知乎，查看你的提问、回答和文章。"
            iconSource: "asset:///icons/ic_contact.png"
            actionText: "登录"
            onActionTriggered: userPage.appRoot.openAccounts(userPage.navigator)
        }
        FeedBody {
            objectName: "profileFeedBody"
            visible: !!userPage.currentFeed && (!userPage.ownProfile || zhihu.loggedIn)
            layoutProperties: StackLayoutProperties { spaceQuota: 1 }
            feed: userPage.currentFeed
            emptyTitle: profileTabs.selectedIndex === 0 ? "还没有提问" : profileTabs.selectedIndex === 1 ? "还没有回答" : "还没有文章"
            emptyMessage: "这个分类暂时没有公开内容，稍后可以重新加载。"
            emptyIcon: "asset:///icons/ic_contact.png"
            onOpenItem: userPage.appRoot.openItem(item, userPage.navigator)
            onOpenUser: userPage.appRoot.openUser(user, userPage.navigator)
            onLogin: userPage.appRoot.openAccounts(userPage.navigator)
        }
    }
    attachedObjects: [QTimer { id: reloadTimer; interval: 0; singleShot: true; onTimeout: userPage.rebuild() }]
}

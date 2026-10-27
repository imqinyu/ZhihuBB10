import bb.cascades 1.4
import tech.lwl 1.0
import "Messages.js" as Messages

Page {
    id: page
    property variant appRoot: null
    property variant navigator: null
    property variant target: ({})
    property variant feed: null
    property int requestedCount: -1
    titleBar: TitleBar { title: page.target.kind === "commentReplies" ? "回复" : "评论" }
    actionBarVisibility: ChromeVisibility.Compact
    function initialize(item) {
        var old = feed;
        target = item;
        requestedCount = -1;
        feed = zhihu.createFeed(item.kind === "commentReplies" ? "commentReplies" : "comments", "", page);
        feed.setItem(item);
        if (old) zhihu.releaseFeed(old);
        feed.load(false);
    }
    function refresh() {
        requestedCount = -1;
        if (feed) feed.load(false);
    }
    function sessionChanged() { sessionRefresh.start(); }
    onCreationCompleted: zhihu.sessionChanged.connect(sessionChanged)
    attachedObjects: [QTimer { id: sessionRefresh; interval: 0; singleShot: true; onTimeout: page.refresh() }]
    function loadMore() {
        if (!feed || feed.busy || !feed.hasMore || feed.error.length > 0 || feed.count === requestedCount) return;
        requestedCount = feed.count;
        feed.load(true);
    }
    function openReplies(comment) {
        if (!appRoot || !(comment.children > 0)) return;
        appRoot.openComments({kind: "commentReplies", id: comment.id}, navigator);
    }
    actions: [
        ActionItem {
            title: "重新加载"
            imageSource: "asset:///icons/ic_resume.png"
            ActionBar.placement: ActionBarPlacement.InOverflow
            enabled: !!page.feed && !page.feed.busy
            onTriggered: page.refresh()
        }
    ]
    Container {
        ActivityIndicator {
            visible: !!page.feed && page.feed.busy
            running: page.feed ? page.feed.busy : false
            horizontalAlignment: HorizontalAlignment.Center
            preferredHeight: ui.du(6)
        }
        Container {
            visible: !!page.feed && page.feed.error.length > 0 && page.feed.count > 0
            leftPadding: ui.du(2)
            rightPadding: ui.du(2)
            Label { text: page.feed ? Messages.message(page.feed.error) : ""; multiline: true }
            Button {
                text: page.feed && page.feed.error === "login_required" && page.appRoot ? "账号管理" : "重试"
                onClicked: {
                    if (page.feed.error === "login_required" && page.appRoot) page.appRoot.openAccounts(page.navigator);
                    else page.refresh();
                }
            }
        }
        Label {
            visible: !!page.feed && page.feed.cached && page.feed.count > 0
            text: "正在显示本地缓存"
            leftMargin: ui.du(2)
            textStyle.base: SystemDefaults.TextStyles.SmallText
            textStyle.color: Color.Gray
        }
        Container {
            layout: DockLayout {}
            layoutProperties: StackLayoutProperties { spaceQuota: 1 }
            EmptyState {
                visible: !!page.feed && page.feed.loaded && !page.feed.busy && page.feed.count === 0
                heading: page.feed && page.feed.error ? (page.feed.error === "login_required" ? "登录后查看评论" : "评论暂时无法加载") : page.target.kind === "commentReplies" ? "还没有回复" : "还没有评论"
                message: page.feed && page.feed.error ? Messages.message(page.feed.error) : "暂时没有可显示的内容，稍后可以重新加载。"
                iconSource: "asset:///icons/ic_sb_textmessage.png"
                actionText: page.feed && page.feed.error === "login_required" && page.appRoot ? "登录" : "重新加载"
                onActionTriggered: {
                    if (page.feed.error === "login_required" && page.appRoot) page.appRoot.openAccounts(page.navigator);
                    else page.refresh();
                }
            }
            ListView {
                id: commentsList
                objectName: "commentsList"
                property variant units: ui
                visible: !!page.feed && page.feed.count > 0
                dataModel: page.feed ? page.feed.model : null
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                listItemComponents: [
                    ListItemComponent { type: ""; CommentItem { commentData: ListItemData } }
                ]
                onTriggered: page.openReplies(dataModel.data(indexPath))
                attachedObjects: [ListScrollStateHandler { onAtEndChanged: { if (atEnd) page.loadMore(); } }]
            }
        }
    }
}

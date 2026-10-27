import bb.cascades 1.4
import "Messages.js" as Messages

Container {
    id: body
    horizontalAlignment: HorizontalAlignment.Fill
    property variant feed: null
    property string emptyMessage: "这里还没有内容。"
    property string emptyTitle: "还没有内容"
    property string emptyIcon: "asset:///icons/ic_sb_notes.png"
    property string emptyAction: "重新加载"
    property bool emptyBrowse: false
    property int requestedCount: -1
    property string questionTitle: ""
    property bool showTitles: true
    signal openItem(variant item)
    signal openUser(variant user)
    signal configure()
    signal login()
    signal browse()
    function loadMore() {
        if (!feed || feed.busy || !feed.hasMore || feed.count === requestedCount || feed.error.length > 0) return;
        requestedCount = feed.count;
        feed.load(true);
    }
    function refresh() {
        requestedCount = -1;
        if (feed) feed.load(false);
    }
    layout: StackLayout {}
    Container {
        visible: !!body.feed && body.feed.busy
        horizontalAlignment: HorizontalAlignment.Fill
        leftPadding: ui.du(2)
        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
        ActivityIndicator {
            running: body.feed ? body.feed.busy : false
            preferredWidth: ui.du(6)
            preferredHeight: ui.du(6)
        }
        Label { text: "正在加载…"; verticalAlignment: VerticalAlignment.Center; textStyle.base: SystemDefaults.TextStyles.SmallText }
    }
    Container {
        visible: !!body.feed && body.feed.error.length > 0 && body.feed.count > 0
        leftPadding: ui.du(2)
        rightPadding: ui.du(2)
        topPadding: ui.du(1)
        Label { text: body.feed ? Messages.message(body.feed.error) : ""; multiline: true; textStyle.base: SystemDefaults.TextStyles.SmallText }
        Button {
            text: body.feed && body.feed.error === "login_required" ? "账号管理" : "重试"
            onClicked: {
                if (body.feed.error === "login_required") body.login();
                else body.refresh();
            }
        }
    }
    Label {
        visible: !!body.feed && body.feed.cached && body.feed.count > 0
        text: "正在显示本地缓存"
        leftMargin: ui.du(2)
        textStyle.base: SystemDefaults.TextStyles.SmallText
        textStyle.color: Color.Gray
    }
    EmptyState {
        visible: !!body.feed && !body.feed.busy && body.feed.count === 0
        layoutProperties: StackLayoutProperties { spaceQuota: 1 }
        heading: body.feed && body.feed.error ? (body.feed.error === "login_required" ? "登录后查看" : "暂时无法加载") : body.emptyTitle
        message: body.feed && body.feed.error ? Messages.message(body.feed.error) : body.emptyMessage
        iconSource: body.feed && body.feed.error === "login_required" ? "asset:///icons/ic_contact.png" : body.emptyIcon
        actionText: body.feed && body.feed.error ? (body.feed.error === "login_required" ? "登录" : "重试") : body.emptyAction
        onActionTriggered: {
            if (body.feed.error === "login_required") body.login();
            else if (!body.feed.error && body.emptyBrowse) body.browse();
            else body.refresh();
        }
    }
    ListView {
        id: feedList
        objectName: "feedList"
        visible: !!body.feed && body.feed.count > 0
        property variant api: zhihu
        property variant units: ui
        property bool showTitles: body.showTitles
        property real contentWidth: feedWidth.layoutFrame.width
        property int imageVersion: zhihu.imageRevision
        dataModel: body.feed ? body.feed.model : null
        // Keep the regular row delegate while using native grouped headers.
        function itemType(data, indexPath) {
            if (data && data.kind === "questionHeader") return "questionHeader";
            return dataModel && dataModel.itemType(indexPath) === "header" ? "header" : "";
        }
        layoutProperties: StackLayoutProperties { spaceQuota: 1 }
        listItemComponents: [
            ListItemComponent {
                type: "questionHeader"
                Container {
                    id: headingRow
                    property variant units: headingRow.ListItem.view.units
                    preferredWidth: headingRow.ListItem.view.contentWidth
                    minWidth: headingRow.ListItem.view.contentWidth
                    maxWidth: headingRow.ListItem.view.contentWidth
                    preferredHeight: units.du(40)
                    minHeight: units.du(40)
                    horizontalAlignment: HorizontalAlignment.Fill
                    layout: DockLayout {}
                    ImageView {
                        imageSource: "asset:///images/question_background.png"
                        horizontalAlignment: HorizontalAlignment.Fill
                        verticalAlignment: VerticalAlignment.Fill
                        scalingMethod: ScalingMethod.AspectFill
                    }
                    Container {
                        preferredWidth: headingRow.ListItem.view.contentWidth * 0.67
                        minWidth: preferredWidth
                        maxWidth: preferredWidth
                        leftPadding: headingRow.units.du(2)
                        horizontalAlignment: HorizontalAlignment.Left
                        verticalAlignment: VerticalAlignment.Center
                        Label {
                        objectName: "questionHeading"
                        text: ListItemData.title
                        multiline: true
                        autoSize.maxLineCount: 5
                        textFormat: TextFormat.Plain
                        textStyle.fontSize: FontSize.XXLarge
                        textStyle.color: Color.White
                        horizontalAlignment: HorizontalAlignment.Fill
                        }
                    }
                }
            },
            ListItemComponent {
                type: "header"
                Header { title: ListItemData === "0000-00-00" ? "未记录日期" : ListItemData }
            },
            ListItemComponent {
                type: ""
                Container {
                    id: row
                    property variant units: row.ListItem.view.units
                    property string localImage: readImage(row.ListItem.view.imageVersion)
                    function readImage(version) {
                        return row.ListItem.view.api.image(ListItemData.image || "");
                    }
                    leftPadding: units.du(2)
                    rightPadding: units.du(2)
                    topPadding: units.du(1.5)
                    bottomPadding: units.du(0.5)
                    Label {
                        objectName: "feedRowTitle"
                        visible: row.ListItem.view.showTitles
                        text: ListItemData.title || ListItemData.author || "内容"
                        multiline: true
                        autoSize.maxLineCount: 2
                        horizontalAlignment: HorizontalAlignment.Fill
                        textStyle.base: SystemDefaults.TextStyles.TitleText
                        textFormat: TextFormat.Plain
                    }
                    Container {
                        visible: (ListItemData.summary || "").length > 0 || row.localImage.length > 0
                        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                        Label {
                            layoutProperties: StackLayoutProperties { spaceQuota: 1 }
                            text: ListItemData.summary || ""
                            multiline: true
                            autoSize.maxLineCount: 4
                            textStyle.base: SystemDefaults.TextStyles.BodyText
                            textFormat: TextFormat.Plain
                        }
                        ImageView {
                            visible: row.localImage.length > 0
                            imageSource: row.localImage
                            preferredWidth: row.units.du(12)
                            minWidth: row.units.du(12)
                            maxWidth: row.units.du(12)
                            preferredHeight: row.units.du(12)
                            minHeight: row.units.du(12)
                            maxHeight: row.units.du(12)
                            scalingMethod: ScalingMethod.AspectFill
                            verticalAlignment: VerticalAlignment.Center
                        }
                    }
                    Label {
                        text: {
                            var kind = ListItemData.kind;
                            if (kind === "daily") return "知乎日报";
                            if (kind === "notificationGroup") return (ListItemData.unread || 0) + " 条未读";
                            if (kind === "notification") return "通知";
                            if (kind === "question") return (ListItemData.answers || 0) + " 个回答";
                            return (ListItemData.author || "") + "  " + (ListItemData.votes || 0) + " 赞同  " + (ListItemData.comments || 0) + " 评论";
                        }
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.color: Color.Gray
                        textFormat: TextFormat.Plain
                    }
                    Divider {}
                    contextActions: [
                        ActionSet {
                            title: ListItemData.author || "作者"
                            ActionItem {
                                title: "作者主页"
                                enabled: (ListItemData.author_token || ListItemData.author_id || "").length > 0
                                onTriggered: row.ListItem.view.requestUser(ListItemData)
                            }
                        }
                    ]
                }
            }
        ]
        onTriggered: {
            var item = dataModel.data(indexPath);
            if (dataModel.itemType(indexPath) !== "header" && item.kind !== "questionHeader") body.openItem(item);
        }
        function requestUser(item) {
            body.openUser({id: item.author_id || "", url_token: item.author_token || "", name: item.author || "", avatar_url: item.author_avatar || ""});
        }
        attachedObjects: [
            LayoutUpdateHandler { id: feedWidth },
            ListScrollStateHandler {
                onAtEndChanged: { if (atEnd) body.loadMore(); }
            }
        ]
    }
    Label {
        visible: !!body.feed && body.feed.loaded && !body.feed.busy && body.feed.count > 0 && !body.feed.hasMore && body.feed.error.length === 0
        text: "没有更多了"
        horizontalAlignment: HorizontalAlignment.Center
        textStyle.base: SystemDefaults.TextStyles.SmallText
        textStyle.color: Color.Gray
    }
}

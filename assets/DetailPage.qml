import bb.cascades 1.4
import "Messages.js" as Messages
Page {
    id: page
    property variant item: ({})
    property variant appRoot: null
    property variant navigator: null
    property variant feed: null
    property bool saved: false
    property string renderedHtml: ""
    function openLink(value) {
        if (value.indexOf("zhihu-image:") === 0) { zhihu.openReaderImage(value); return; }
        var target = zhihu.linkTarget(value);
        if (target.id) page.appRoot.openItem(target, page.navigator);
        else zhihu.openBrowser(value);
    }
    function applyHtml() {
        if (!feed || feed.html.length === 0 || feed.html === renderedHtml) return;
        renderedHtml = feed.html;
        reader.loadHtml(renderedHtml, zhihu.readerBaseUrl());
    }
    function initialize(value) {
        item = value;
        feed = zhihu.createFeed("detail", "", page);
        feed.setItem(item);
        feed.changed.connect(applyHtml);
        saved = zhihu.isSaved(item);
        zhihu.remember(item);
        feed.load(false);
    }
    actions: [
        ActionItem {
            title: "作者主页"
            enabled: (page.item.author_token || page.item.author_id || "").toString().length > 0
            ActionBar.placement: ActionBarPlacement.InOverflow
            onTriggered: page.appRoot.openUser({id: page.item.author_id || "", url_token: page.item.author_token || "", name: page.item.author || "", avatar_url: page.item.author_avatar || ""}, page.navigator)
            imageSource: "asset:///icons/ic_open_contacts_white.png"
        },
        ActionItem {
            title: page.saved ? "取消收藏" : "收藏"
            imageSource: "asset:///icons/ic_favorite.png"
            ActionBar.placement: ActionBarPlacement.OnBar
            onTriggered: { zhihu.toggleSaved(page.item); page.saved = zhihu.isSaved(page.item); }
        },
        ActionItem {
            title: "查看问题"
            enabled: (page.item.question_id || "").toString().length > 0
            imageSource: "asset:///icons/ic_search_by_subject.png"
            ActionBar.placement: ActionBarPlacement.Signature
            onTriggered: page.appRoot.openItem({kind: "question", id: page.item.question_id, title: page.item.title}, page.navigator)
        },
        ActionItem {
            title: "评论"
            objectName: "detailCommentsAction"
            imageSource: "asset:///icons/ic_sb_textmessage.png"
            ActionBar.placement: ActionBarPlacement.OnBar
            onTriggered: page.appRoot.openComments(page.item, page.navigator)
        },
        ActionItem {
            title: "在浏览器打开"
            imageSource: "asset:///icons/ic_browser.png"
            ActionBar.placement: ActionBarPlacement.InOverflow
            onTriggered: zhihu.openBrowser(page.item.url || "")
        },
        ActionItem {
            title: "重新加载"
            imageSource: "asset:///icons/ic_resume.png"
            ActionBar.placement: ActionBarPlacement.InOverflow
            enabled: page.feed && !page.feed.busy
            onTriggered: page.feed.load(false)
        }
    ]
    Container {
        ActivityIndicator {
            visible: page.feed && page.feed.busy
            running: page.feed ? page.feed.busy : false
            horizontalAlignment: HorizontalAlignment.Center
            preferredHeight: ui.du(6)
        }
        Container {
            visible: page.feed && page.feed.error.length > 0
            leftPadding: ui.du(2)
            rightPadding: ui.du(2)
            Label { text: page.feed ? Messages.message(page.feed.error) : ""; multiline: true }
//            Button {
//                text: "重试"
//                onClicked: page.feed.load(false)
//            }
        }

        ScrollView {
            layoutProperties: StackLayoutProperties { spaceQuota: 1 }
            scrollViewProperties.scrollMode: ScrollMode.Vertical
            WebView {
                id: reader
                settings.javaScriptEnabled: false
                settings.cookiesEnabled: false
                onNavigationRequested: {
                    if (request.navigationType === WebNavigationType.LinkClicked) {
                        request.action = WebNavigationRequestAction.Ignore;
                        page.openLink(request.url.toString());
                    }
                }
            }
        }
    }
}

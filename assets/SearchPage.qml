import bb.cascades 1.4
Page {
    id: page
    property variant appRoot: null
    property variant navigator: null
    property variant feed: null
    function search() {
        var query = input.text.trim();
        if (query.length === 0) return;
        var old = feed;
        feed = zhihu.createFeed("search", query, page);
        if (old) zhihu.releaseFeed(old);
        results.requestedCount = -1;
        feed.load(false);
    }
    titleBar: TitleBar { title: "搜索" }
    actions: [
        ActionItem {
            title: "搜索"
            imageSource: "asset:///icons/action_search.png"
            ActionBar.placement: ActionBarPlacement.Signature
            onTriggered: page.search()
        },
        ActionItem {
            title: "重新加载"
            imageSource: "asset:///icons/ic_resume.png"
            ActionBar.placement: ActionBarPlacement.InOverflow
            enabled: !!page.feed && !page.feed.busy
            onTriggered: results.refresh()
        }
    ]
    Container {
        Container {
            topPadding: ui.du(1)
            leftPadding: ui.du(1)
            rightPadding: ui.du(1)
            TextField {
                id: input
                objectName: "searchQuery"
                hintText: "搜索问题、回答和文章"
                input.submitKey: SubmitKey.Search
                input.onSubmitted: page.search()
            }
        }
        EmptyState {
            visible: !page.feed
            layoutProperties: StackLayoutProperties { spaceQuota: 1 }
            heading: "搜索知乎"
            message: "输入关键词，查找感兴趣的问题、回答和文章。"
            iconSource: "asset:///icons/action_search.png"
            actionText: "输入关键词"
            onActionTriggered: input.requestFocus()
        }
        FeedBody {
            id: results
            visible: !!page.feed
            feed: page.feed
            layoutProperties: StackLayoutProperties { spaceQuota: 1 }
            emptyMessage: "没有找到相关内容，换个关键词试试。"
            emptyTitle: "没有搜索结果"
            emptyIcon: "asset:///icons/action_search.png"
            emptyAction: "更换关键词"
            emptyBrowse: true
            onBrowse: input.requestFocus()
            onOpenItem: page.appRoot.openItem(item, page.navigator)
            onOpenUser: page.appRoot.openUser(user, page.navigator)
            onConfigure: page.appRoot.openSettings(page.navigator)
            onLogin: page.appRoot.openAccounts(page.navigator)
        }
    }
}

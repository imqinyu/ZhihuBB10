import bb.cascades 1.4
import "Messages.js" as Messages
Page {
    id: page
    property variant appRoot: null
    property variant navigator: null
    property variant current: zhihu.account
    property string avatar: readAvatar(zhihu.imageRevision, current.avatar_url || "")
    function readAvatar(version, url) { return zhihu.image(url); }
    titleBar: TitleBar { title: "账号管理" }
    actions: [
        ActionItem {
            title: "新增账号"
            imageSource: "asset:///icons/ic_add_contact.png"
            ActionBar.placement: ActionBarPlacement.Signature
            onTriggered: page.appRoot.openLogin(page.navigator)
        },
        ActionItem {
            title: "退出登录"
            imageSource: "asset:///icons/ca_contact_blacklist.png"
            ActionBar.placement: ActionBarPlacement.InOverflow
            enabled: zhihu.loggedIn
            onTriggered: zhihu.logout()
        }
    ]
    Container {
        Header { title: "当前账号" }
        Container {
            contextActions: [
                ActionSet {
                    title: page.current.name || "当前账号"
                    ActionItem {
                        title: "复制凭据"
                        enabled: zhihu.loggedIn
                        imageSource: "asset:///icons/ic_copy.png"
                        onTriggered: zhihu.copyAccountCredential(page.current.id)
                    }
                    DeleteActionItem {
                        title: "删除账号"
                        enabled: zhihu.loggedIn
                        onTriggered: zhihu.removeAccount(page.current.id)
                    }
                }
            ]
            leftPadding: ui.du(2)
            rightPadding: ui.du(2)
            topPadding: ui.du(1)
            bottomPadding: ui.du(1)
            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
            ImageView {
                imageSource: page.avatar.length > 0 ? page.avatar : "asset:///icons/ic_contact.png"
                preferredWidth: ui.du(13.75)
                minWidth: ui.du(13.75)
                maxWidth: ui.du(13.75)
                preferredHeight: ui.du(13.75)
                minHeight: ui.du(13.75)
                maxHeight: ui.du(13.75)
                scalingMethod: ScalingMethod.AspectFit
            }
            Container {
                verticalAlignment: VerticalAlignment.Center
                layoutProperties: StackLayoutProperties { spaceQuota: 1 }
                Label { text: zhihu.loggedIn ? page.current.name : "未登录"; textFormat: TextFormat.Plain; textStyle.base: SystemDefaults.TextStyles.PrimaryText }
                Label {
                    text: zhihu.loggedIn ? page.current.masked : "登录后可查看关注和通知"
                    multiline: true
                    textStyle.base: SystemDefaults.TextStyles.SmallText
                    textStyle.color: Color.Gray
                }
            }
        }
        Header { title: "已保存的账号" }
        EmptyState {
            visible: zhihu.accounts.length === 0
            layoutProperties: StackLayoutProperties { spaceQuota: 1 }
            heading: "还没有账号"
            message: "添加知乎账号，查看关注、通知和个人内容。"
            iconSource: "asset:///icons/ic_contact.png"
            actionText: "新增账号"
            onActionTriggered: page.appRoot.openLogin(page.navigator)
        }
        ListView {
            id: accountsList
            visible: zhihu.accounts.length > 0
            property variant api: zhihu
            property variant units: ui
            dataModel: ArrayDataModel { id: accountModel }
            function updateAccounts() { accountModel.clear(); accountModel.append(zhihu.accounts); }
            onCreationCompleted: {
                updateAccounts();
                zhihu.accountChanged.connect(updateAccounts);
            }
            layoutProperties: StackLayoutProperties { spaceQuota: 1 }
            listItemComponents: [
                ListItemComponent {
                    type: ""
                    Container {
                        id: accountRow
                        leftPadding: accountRow.ListItem.view.units.du(2)
                        rightPadding: accountRow.ListItem.view.units.du(2)
                        topPadding: accountRow.ListItem.view.units.du(2)
                        Label { text: ListItemData.name; textFormat: TextFormat.Plain; textStyle.base: SystemDefaults.TextStyles.PrimaryText }
                        Label { text: ListItemData.masked; textStyle.base: SystemDefaults.TextStyles.SmallText; textStyle.color: Color.Gray }
                        Divider {}
                        contextActions: [
                            ActionSet {
                                title: ListItemData.name
                                actions: [
                                    ActionItem {
                                        title: "复制凭据"
                                        imageSource: "asset:///icons/ic_copy.png"
                                        onTriggered: accountRow.ListItem.view.api.copyAccountCredential(ListItemData.id)
                                    },
                                    ActionItem {
                                        title: "切换到此账号"
                                        imageSource: "asset:///icons/ic_groups_white.png"
                                        onTriggered: accountRow.ListItem.view.api.switchAccount(ListItemData.id)
                                    },
                                    DeleteActionItem {
                                        title: "删除账号"
                                        onTriggered: accountRow.ListItem.view.api.removeAccount(ListItemData.id)
                                    }
                                ]
                            }
                        ]
                    }
                }
            ]
            onTriggered: zhihu.switchAccount(dataModel.data(indexPath).id)
        }
    }
}

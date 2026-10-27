// Reused from ZhihuDaily-Cascades CommentList.qml; data is normalized by Feed.
import bb.cascades 1.4
import tech.lwl 1.0

CustomListItem {
    id: commentRow
    objectName: "commentItem"
    property variant commentData: ({})
    property variant units: commentRow.ListItem.view ? commentRow.ListItem.view.units : null
    function du(value) { return units ? units.du(value) : 0; }
    dividerVisible: true
    Container {
        layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
        topPadding: commentRow.du(2)
        bottomPadding: commentRow.du(2)
        leftPadding: commentRow.du(2)
        rightPadding: commentRow.du(2)
        WebImageView {
            preferredWidth: commentRow.du(8)
            minWidth: commentRow.du(8)
            maxWidth: commentRow.du(8)
            preferredHeight: commentRow.du(8)
            url: commentRow.commentData.avatar || "asset:///daily/images/account_avatar.png"
            loadingImageSource: "asset:///daily/images/account_avatar.png"
            failImageSource: "asset:///daily/images/account_avatar.png"
        }
        Container {
            topPadding: commentRow.du(2)
            leftPadding: commentRow.du(2)
            layoutProperties: StackLayoutProperties { spaceQuota: 1 }
            Container {
                rightPadding: commentRow.du(2)
                horizontalAlignment: HorizontalAlignment.Fill
                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                Label {
                    text: commentRow.commentData.author || "匿名用户"
                    textFormat: TextFormat.Plain
                    multiline: true
                    layoutProperties: StackLayoutProperties { spaceQuota: 1 }
                    textStyle.fontWeight: FontWeight.Bold
                }
                Container {
                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                    WebImageView {
                        preferredWidth: commentRow.du(4)
                        preferredHeight: commentRow.du(4)
                        url: "asset:///daily/images/comment_vote.png"
                    }
                    Label {
                        text: (commentRow.commentData.likes || 0).toString()
                        textStyle.base: SystemDefaults.TextStyles.SubtitleText
                        textStyle.color: Color.Gray
                    }
                }
            }
            Label {
                text: commentRow.commentData.deleted ? "该评论已删除" : (commentRow.commentData.content || "")
                textFormat: TextFormat.Plain
                multiline: true
            }
            Container {
                id: replyContainer
                property variant replyInfo: commentRow.commentData.reply_to || ({})
                visible: !!commentRow.commentData.reply_to
                horizontalAlignment: HorizontalAlignment.Fill
                Container {
                    visible: (replyContainer.replyInfo.status || 0) === 0
                    Label {
                        text: "@" + (replyContainer.replyInfo.author || "") + "："
                        textFormat: TextFormat.Plain
                        textStyle.fontWeight: FontWeight.Bold
                    }
                    Label {
                        id: quoteText
                        objectName: "commentQuote"
                        visible: (replyContainer.replyInfo.content || "").length > 0
                        text: "　　" + (replyContainer.replyInfo.content || "")
                        textFormat: TextFormat.Plain
                        multiline: true
                        autoSize.maxLineCount: 2
                        onTextChanged: autoSize.maxLineCount = 2
                        function expandQuote() { autoSize.resetMaxLineCount(); }
                        onTouch: { if (event.isUp()) expandQuote(); }
                    }
                }
                Label {
                    visible: (replyContainer.replyInfo.status || 0) !== 0
                    text: replyContainer.replyInfo.error_msg || "原评论已不可用"
                    multiline: true
                    textFormat: TextFormat.Plain
                    textStyle.color: Color.Gray
                    textStyle.base: SystemDefaults.TextStyles.SubtitleText
                }
            }
            Label {
                text: commentRow.commentData.time_text || ""
                textStyle.base: SystemDefaults.TextStyles.SubtitleText
                textStyle.color: Color.Gray
            }
            Label {
                visible: (commentRow.commentData.children || 0) > 0
                text: "查看 " + (commentRow.commentData.children || 0) + " 条回复"
                textStyle.color: Color.create("#0084ff")
                textStyle.base: SystemDefaults.TextStyles.SmallText
            }
        }
    }
}

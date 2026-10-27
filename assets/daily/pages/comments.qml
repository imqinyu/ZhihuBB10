// Daily compatibility entry; both comment types share one list.
import bb.cascades 1.4
import "asset:///" as App
App.CommentsPage {
    property variant newsId
    onNewsIdChanged: {
        if (newsId) initialize({kind: "daily", id: newsId.toString()});
    }
}

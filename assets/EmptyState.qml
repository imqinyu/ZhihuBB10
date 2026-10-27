import bb.cascades 1.4
// BB10 UI Guidelines p.114: explain the state and offer one next action.
ScrollView {
    id: state
    objectName: "emptyState"
    property string heading: "没有内容"
    property string message: ""
    property string iconSource: "asset:///icons/ic_sb_notes.png"
    property string actionText: "重新加载"
    signal actionTriggered()
    horizontalAlignment: HorizontalAlignment.Fill
    verticalAlignment: VerticalAlignment.Fill
    scrollViewProperties.scrollMode: ScrollMode.Vertical
    attachedObjects: [LayoutUpdateHandler { id: viewport }]
    Container {
        horizontalAlignment: HorizontalAlignment.Fill
        minWidth: viewport.layoutFrame.width
        maxWidth: viewport.layoutFrame.width
        minHeight: viewport.layoutFrame.height
        layout: DockLayout {}
        Container {
        horizontalAlignment: HorizontalAlignment.Fill
        verticalAlignment: VerticalAlignment.Center
        leftPadding: ui.du(4)
        rightPadding: ui.du(4)
        topPadding: ui.du(3)
        bottomPadding: ui.du(3)
        ImageView {
            imageSource: state.iconSource
            preferredWidth: ui.du(10)
            preferredHeight: ui.du(10)
            horizontalAlignment: HorizontalAlignment.Center
            scalingMethod: ScalingMethod.AspectFit
            filterColor: Color.Gray
            opacity: 0.65
        }
        Label {
            text: state.heading
            multiline: true
            horizontalAlignment: HorizontalAlignment.Fill
            textStyle.base: SystemDefaults.TextStyles.PrimaryText
            textStyle.fontWeight: FontWeight.Bold
            textStyle.textAlign: TextAlign.Center
            textFormat: TextFormat.Plain
        }
        Label {
            text: state.message
            visible: text.length > 0
            multiline: true
            horizontalAlignment: HorizontalAlignment.Fill
            textStyle.base: SystemDefaults.TextStyles.SmallText
            textStyle.color: Color.Gray
            textStyle.textAlign: TextAlign.Center
            textFormat: TextFormat.Plain
        }
        Button {
            text: state.actionText
            visible: text.length > 0
            horizontalAlignment: HorizontalAlignment.Center
            onClicked: state.actionTriggered()
        }
        }
    }
}

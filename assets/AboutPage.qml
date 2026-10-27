import bb.cascades 1.4
Page {
    property variant appRoot: null
    property variant navigator: null
    titleBar: TitleBar { title: "关于" }
    ScrollView {
        scrollViewProperties.scrollMode: ScrollMode.Vertical
        Container {
            leftPadding: ui.du(3)
            rightPadding: ui.du(3)
            topPadding: ui.du(3)
            Label { text: "知乎 for BlackBerry 10"; multiline: true; textStyle.base: SystemDefaults.TextStyles.TitleText }
            Label { text: "0.3.3 · Cascades 原生版"; textStyle.base: SystemDefaults.TextStyles.SmallText }
            Label { text: "非官方客户端。知乎内容与商标归各自权利人所有。"; multiline: true }
            Label { text: "参考与许可" }
            Label { text: "接口与请求签名参考 zly2006 的 Zhihu++（AGPL-3.0）；日报接口参考 liuwanlin 的 ZhihuDaily Cascades。"; multiline: true; textStyle.base: SystemDefaults.TextStyles.BodyText }
            
        }
    }
}

import bb.cascades 1.4
import "Messages.js" as Messages
Page {
    id: page
    property variant appRoot: null
    property variant navigator: null
    property variant phoneLogin: null
    property string inputError: ""
    property bool phoneMode: loginMethod.selectedIndex === 1
    property bool busy: zhihu.verifying || (phoneLogin ? phoneLogin.busy : false)
    //here
    titleBar: TitleBar {
        id: loginMethod
        objectName: "loginMethod"
        kind: TitleBarKind.Segmented
        selectedIndex: 0
        options: [Option { text: "Cookie 登录"; enabled: !page.busy }, Option { text: "手机号登录"; enabled: !page.busy }]
        onSelectedIndexChanged: { page.inputError = ""; zhihu.clearAccountError(); }
    }
    onCreationCompleted: { zhihu.clearAccountError(); phoneLogin = zhihu.createPhoneLogin(page); }
    function submit() {
        if (busy) return;
        inputError = "";
        if (phoneMode) phoneLogin.signIn(phoneField.text, digitsField.text);
        else zhihu.verifyCookieValues(sessionField.text, deviceField.text);
    }
    function pasteValue(field) {
        var value = zhihu.clipboardText();
        if (!value || value.trim().length === 0) { inputError = "剪贴板里没有文本。"; return; }
        var values = zhihu.splitCookieValues(value);
        if (values.z_c0) sessionField.text = values.z_c0;
        if (values.d_c0) deviceField.text = values.d_c0;
        if (!values.z_c0 && !values.d_c0) field.text = value.trim();
        inputError = "";
    }
    actions: [
        ActionItem {
            objectName: "loginSubmitAction"
            title: page.phoneMode ? "登录" : "验证并保存"
            imageSource: "asset:///icons/maps_ic_done.png"
            ActionBar.placement: ActionBarPlacement.Signature
            enabled: !page.busy && (page.phoneMode ? phoneField.text.trim().length > 0 && digitsField.text.trim().length > 0 : sessionField.text.trim().length > 0 && deviceField.text.trim().length > 0)
            onTriggered: page.submit()
        }
//        ActionItem {
//            title: "粘贴"
//            objectName: "loginPasteAction"
//            imageSource: "asset:///icons/ic_copy.png"
//            ActionBar.placement: ActionBarPlacement.OnBar
//            enabled: !page.busy
//            onTriggered: {
//                if (page.phoneMode) phoneField.text = zhihu.clipboardText().trim();
//                else page.pasteValue(sessionField);
//            }
//        }
    ]
    ScrollView {
        scrollViewProperties.scrollMode: ScrollMode.Vertical
        Container {
            Container {
                visible: !page.phoneMode
                Header { title: "浏览器登录凭据" }
                Container {
                    leftPadding: ui.du(2)
                    rightPadding: ui.du(2)
                    Label {
                        text: "在电脑浏览器登录知乎，在开发者工具的 Application / Cookies / zhihu.com 中，分别复制 z_c0 和 d_c0 的值。"
                        multiline: true
                        textStyle.base: SystemDefaults.TextStyles.BodyText
                    }
                    Label { text: "z_c0"; textStyle.base: SystemDefaults.TextStyles.SubtitleText }
                    Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                    TextField {
                        id: sessionField
                        objectName: "loginSessionField"
                        hintText: "输入 z_c0 的值"
                        inputMode: TextFieldInputMode.Password
                        maximumLength: 8192
                        enabled: !page.busy
                        input.submitKey: SubmitKey.Next
                        input.onSubmitted: deviceField.requestFocus()
                        onTextChanging: { page.inputError = ""; zhihu.clearAccountError(); }
                        layoutProperties: StackLayoutProperties { spaceQuota: 1 }
                    }
                    Button {
                        objectName: "loginPasteSession"
                        text: "粘贴"
                        preferredWidth: ui.du(12)
                        minWidth: ui.du(12)
                        maxWidth: ui.du(12)
                        enabled: !page.busy
                        onClicked: page.pasteValue(sessionField)
                    }
                    }
                    Label { text: "d_c0"; textStyle.base: SystemDefaults.TextStyles.SubtitleText }
                    Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                    TextField {
                        id: deviceField
                        objectName: "loginDeviceField"
                        hintText: "输入 d_c0 的值"
                        inputMode: TextFieldInputMode.Password
                        maximumLength: 8192
                        enabled: !page.busy
                        input.submitKey: SubmitKey.Done
                        input.onSubmitted: page.submit()
                        onTextChanging: { page.inputError = ""; zhihu.clearAccountError(); }
                        layoutProperties: StackLayoutProperties { spaceQuota: 1 }
                    }
                    Button {
                        objectName: "loginPasteDevice"
                        text: "粘贴"
                        preferredWidth: ui.du(12)
                        minWidth: ui.du(12)
                        maxWidth: ui.du(12)
                        enabled: !page.busy
                        onClicked: page.pasteValue(deviceField)
                    }
                    }
//                    Label {
//                        text: "只需复制值本身；也可粘贴完整 Cookie 自动填入两项。document.cookie 可能读不到 z_c0，请从 Cookies 表中复制。"
//                        multiline: true
//                        textStyle.base: SystemDefaults.TextStyles.SmallText
//                        textStyle.color: Color.Gray
//                    }
                }
            }
            Container {
                visible: page.phoneMode
                Header { title: "短信验证码登录" }
                Container {
                    
                    leftPadding: ui.du(2)
                    rightPadding: ui.du(2)
                    topPadding: ui.du(2.0)
                    //                    Label { text: "使用中国大陆手机号接收登录验证码。"; multiline: true }
                    Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                    TextField {
                        id: phoneField
                        objectName: "loginPhoneField"
                        hintText: "手机号（+86）"
                        inputMode: TextFieldInputMode.PhoneNumber
                        maximumLength: 20
                        enabled: !page.busy && (!page.phoneLogin || !page.phoneLogin.captchaRequired)
                        input.submitKey: SubmitKey.Next
                        input.onSubmitted: digitsField.requestFocus()
                        layoutProperties: StackLayoutProperties { spaceQuota: 1 }
                    }

                    }
                    Button {
                        objectName: "loginSendDigits"
                        text: page.phoneLogin && page.phoneLogin.cooldown > 0 ? page.phoneLogin.cooldown + " 秒后重发" : "获取短信验证码"
                        enabled: !!page.phoneLogin && !page.busy && page.phoneLogin.cooldown === 0 && phoneField.text.trim().length > 0
                        onClicked: { page.inputError = ""; page.phoneLogin.requestDigits(phoneField.text); }
                    }
                    Container {
                        visible: !!page.phoneLogin && page.phoneLogin.captchaRequired
                        Label { text: "请先完成图形验证码"; multiline: true }
                        ImageView {
                            image: page.phoneLogin ? page.phoneLogin.captchaImage : null
                            preferredHeight: ui.du(16)
                            horizontalAlignment: HorizontalAlignment.Fill
                            scalingMethod: ScalingMethod.AspectFit
                        }
                        TextField { id: captchaField; hintText: "图形验证码"; enabled: !page.busy }
                        Button { text: "验证并发送短信"; enabled: !page.busy && captchaField.text.trim().length > 0; onClicked: page.phoneLogin.verifyCaptcha(captchaField.text) }
                        Button { text: "换一张验证码"; enabled: !page.busy; onClicked: page.phoneLogin.refreshCaptcha() }
                    }
                    Container {
                    horizontalAlignment: HorizontalAlignment.Fill
                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                    TextField {
                        id: digitsField
                        objectName: "loginDigitsField"
                        hintText: "短信验证码"
                        maximumLength: 8
                        inputMode: TextFieldInputMode.NumbersAndPunctuation
                        enabled: !page.busy
                        input.submitKey: SubmitKey.Done
                        input.onSubmitted: page.submit()
                        layoutProperties: StackLayoutProperties { spaceQuota: 1 }
                    }

                    }
                    Label {
                        visible: !!page.phoneLogin && page.phoneLogin.sent
                        text: "短信已发送，请输入验证码后点击登录。"
                        multiline: true
                        textStyle.color: Color.Gray
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                    }
                }
            }
            Container {
                leftPadding: ui.du(2)
                rightPadding: ui.du(2)
                bottomPadding: ui.du(2)
                ActivityIndicator { running: page.busy; visible: page.busy; horizontalAlignment: HorizontalAlignment.Center }
                Label {
                    visible: text.length > 0
                    text: page.inputError || (page.phoneMode && page.phoneLogin && page.phoneLogin.error ? Messages.message(page.phoneLogin.error) : Messages.message(zhihu.accountError))
                    multiline: true
                    textFormat: TextFormat.Plain
                }
//                Label { text: "验证成功后保存到本机账号管理，可随时删除。"; multiline: true; textStyle.base: SystemDefaults.TextStyles.SmallText; textStyle.color: Color.Gray }
            }
        }
    }
}

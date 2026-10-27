// Layout adapted from BBTiebaUI SettingsPage.qml.
import bb.cascades 1.4
import tech.lwl 1.0

Page {
    id: settingsPage

    
    property variant appRoot: null
    property variant navigator: null

    
    property bool uiReady: false
    property bool cacheAvailable: typeof zhihu !== "undefined"
    property int cachePercent: cacheAvailable ? Math.max(0, Math.min(100, zhihu.cacheUsagePercent)) : 0
    property string cacheSummary: cacheAvailable ? qsTr("已用 ") + zhihu.cacheUsageText : qsTr("已用 — / —")
    attachedObjects: [
        QTimer {
            id: cacheRefresh
            interval: 3000
            onTimeout: zhihu.refreshCacheUsage()
        }
    ]

    titleBar: TitleBar {
        title: qsTr("设置")
    }

    
    function cacheLimitIndex(mb) {
        if (mb >= 2048)
            return 2
        if (mb >= 1024)
            return 1
        return 0
    }

    
    function applyTheme(value) {
        if (value == VisualStyle.Dark)
            Application.themeSupport.setVisualStyle(VisualStyle.Dark)
        else
            Application.themeSupport.setVisualStyle(VisualStyle.Bright)
    }

    onCreationCompleted: {
        if (settingsPage.cacheAvailable) zhihu.refreshCacheUsage()
        settingsPage.uiReady = true
        // Refresh while downloads in other tabs finish in the background.
        cacheRefresh.start()
    }

    ScrollView {
        scrollRole: ScrollRole.Main

        Container {
            layout: StackLayout {
                orientation: LayoutOrientation.TopToBottom
            }
            Header {
                title: qsTr("外观")
            }

            Container {
                layout: StackLayout {
                    orientation: LayoutOrientation.LeftToRight
                }
                topPadding: ui.du(1.0)
                bottomPadding: ui.du(1.0)
                rightPadding: ui.du(1.0)
                leftPadding: ui.du(1.0)

                DropDown {
                    objectName: "settingsTheme"
                    title: qsTr("主题")
                    selectedIndex: (zhihu.dark) ? 1 : 0

                    options: [
                        Option {
                            text: qsTr("亮色")
                            value: VisualStyle.Bright
                        },
                        Option {
                            text: qsTr("暗色")
                            value: VisualStyle.Dark
                        }
                    ]

                    onSelectedOptionChanged: {
                        if (! settingsPage.uiReady)
                            return
                        if (! selectedOption)
                            return
                        settingsPage.applyTheme(selectedOption.value)
                        zhihu.saveSettings(selectedOption.value == VisualStyle.Dark, zhihu.largeText)
                    }
                }
            }
            Header {
                title: qsTr("缓存")
            }

            Container {
                topMargin: 5.0
                bottomMargin: 5.0

                horizontalAlignment: HorizontalAlignment.Fill
                leftMargin: ui.du(1.0)
                rightMargin: ui.du(1.0)
                leftPadding: ui.du(1.0)
                rightPadding: ui.du(1.0)

                topPadding: ui.du(1.0)
                bottomPadding: ui.du(1.0)

                Label {
                    text: settingsPage.cacheSummary
                    multiline: true
                    textStyle.fontSize: FontSize.Medium
                    textStyle.fontWeight: FontWeight.Bold
                    horizontalAlignment: HorizontalAlignment.Fill
                }
                ProgressIndicator {
                    fromValue: 0
                    toValue: 100
                    value: settingsPage.cachePercent
                    horizontalAlignment: HorizontalAlignment.Fill
                    topMargin: ui.du(0.75)
                    accessibility.name: qsTr("缓存使用比例")
                }
                DropDown {
                    objectName: "settingsCacheLimit"
                    title: qsTr("缓存上限")
                    enabled: settingsPage.cacheAvailable
                    selectedIndex: settingsPage.cacheAvailable ? settingsPage.cacheLimitIndex(zhihu.cacheLimitMb) : 0

                    options: [
                        Option {
                            text: "512MB"
                            value: 512
                        },
                        Option {
                            text: "1G"
                            value: 1024
                        },
                        Option {
                            text: "2G"
                            value: 2048
                        }
                    ]

                    onSelectedOptionChanged: {
                        if (! settingsPage.uiReady || !settingsPage.cacheAvailable)
                            return
                        if (! selectedOption)
                            return
                        if (zhihu.cacheLimitMb != selectedOption.value)
                            zhihu.cacheLimitMb = selectedOption.value
                        zhihu.refreshCacheUsage()
                    }
                }

                Container {
                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                    horizontalAlignment: HorizontalAlignment.Fill
                    topMargin: ui.du(1)
                    Button {
                        objectName: "settingsClearCache"
                        text: qsTr("清除缓存")
                        enabled: settingsPage.cacheAvailable
                        layoutProperties: StackLayoutProperties { spaceQuota: 1 }
                        leftMargin: ui.du(1)
                        onClicked: {
                            zhihu.clearCache()
                            zhihu.refreshCacheUsage()
                        }
                    }
                }

                Label {
                    text: qsTr("缓存日报、通知、头像等内容，超过上限时自动清理较旧的缓存。")
                    multiline: true
                    topMargin: 10.0
                    horizontalAlignment: HorizontalAlignment.Center
                    textStyle.fontSize: FontSize.Small
                    textStyle.color: Color.Gray
                    textStyle.textAlign: TextAlign.Center
                }

            }
            Header {
                title: qsTr("内容")
            }

            Container {
                topPadding: ui.du(1.0)
                bottomPadding: ui.du(1.0)
                rightPadding: ui.du(1.0)
                leftPadding: ui.du(1.0)
                layout: StackLayout {
                    orientation: LayoutOrientation.LeftToRight
                }

                Label {
                    text: qsTr("正文大字体")
                    verticalAlignment: VerticalAlignment.Center
                    layoutProperties: StackLayoutProperties {
                        spaceQuota: 1
                    }
                    textStyle {
                        base: SystemDefaults.TextStyles.BodyText
                    }
                }

                ToggleButton {
                    objectName: "settingsLargeText"
                    verticalAlignment: VerticalAlignment.Center
                    checked: settingsPage.cacheAvailable ? zhihu.largeText : false
                    enabled: settingsPage.cacheAvailable
                    onCheckedChanged: {
                        if (! settingsPage.uiReady || ! settingsPage.cacheAvailable)
                            return
                        zhihu.saveSettings(zhihu.dark, checked)
                    }
                }
            }

        }
    }
}

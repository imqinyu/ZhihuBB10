# 0.2.2 护眼联动、日期分组与阅读内容缓存

2026-10-25，BB10 10.3.1.995 模拟器 192.168.106.129。模拟器日期按其自身时钟显示。

## 行为

- SettingsPage 移除独立日报护眼开关；日报新闻页直接绑定 zhihu.dark。暗色主题启用护眼，切回亮色立即关闭，已经打开的正文也同步。启动与设置保存时同步旧日报配置值。
- Feed 使用原生 GroupDataModel 为最近浏览、收藏夹按完整日期分组，Header 与 BBTiebaUI 同类页面一致。排序为日期／浏览或收藏时间降序，点击 Header 无动作。
- 新浏览记录写 visitedAt，新收藏写 savedAt。旧版本没有保存时间的条目保留，放进“未记录日期”，不虚构日期；重新浏览或重新收藏会记录当前时间。
- 正文文字沿用按账号隔离的 detail JSON 缓存，正文内知乎 CDN 图片接入受上限约束的应用图片缓存；优先选择 data-original／data-actualsrc，避免把延迟加载占位图当正文图片。已有本地图片在再次打开文章时用于 WebView。
- 缓存是已读取的正文／图片，不预先批量下载所有收藏。用户明确要求保留收藏和浏览条目；它们不计入可回收缓存，也不会被清除或超限回收删除。已打开页面的内容可继续驻留内存，进行中的下载完成后可能重新生成缓存。

## 编译与静态

`build.ps1 -Configuration Device-Debug` 与 `build.ps1 -Configuration Simulator-Debug` 通过，见 build-device-0.2.2.log、build-simulator-0.2.2.log。包版本 0.2.2。

静态检查沿用已核实的 HTTPS 直连及 Carousel StackListLayout.orientation 两项例外；真实 SDK 和模拟器支持这些用法。旧验证副本的扫描报错不属于生产资产。新增 QML/C++ 通过原生加载和编译检查。

## 原生验证

独立 UI Check 链接生产控制器及 QML，使用无凭据的测试记录，不读取正式应用账号。

- 最近浏览与收藏各放三条：今日、昨日、旧版无日期。验证三个 Header、各组条目、倒序、条目总数，以及点击 Header 不改变导航栈。见 ui-check/history-0.2.2.log、saved-0.2.2.log 与 shots/history-date-headers-0.2.2.png、saved-date-headers-0.2.2.png。
- 同一篇已打开的日报，通过主题更改检查 isNewsEyeProtectionMode 为 true／false，原生截图确认灰底护眼与亮底正文切换。见 shots/daily-auto-dark-0.2.2.png、daily-auto-light-0.2.2.png。
- 生产控制器读取本地测试正文与图片，HTML 使用 file URL 替换延迟加载原图地址；WebView 用沙箱内本地基础地址加载正文，原生截图确认实际图片可见。加载状态变化不会重复载入相同正文。测试 ID 故意不存在，网络返回 404，而本地正文与图片仍显示；这不是线上文章错误。见 shots/cached-library-content-0.2.2.png、ui-check/content-final-0.2.2.log。
- 设置页无日报护眼开关，主题、字体、缓存上限可保存并恢复。清理验证覆盖两类日报磁盘缓存、列表和通知 JSON、正文 JSON、正文图片与头像，确认内容缓存移除，实际账号作用域的收藏／历史列表及设置保持原值。最终结果见 ui-check/final-0.2.2.log。

本轮未连接 Q20 真机。长时间使用和首次打开时尚未完成下载的图片不属于已经验证的离线缓存。

正式模拟器应用已停止后覆盖升级为 0.2.2.1，保留原有私有数据，启动日志确认首页正常载入。
独立 UI Check 已卸载；本轮启动的截图控制器已停止。

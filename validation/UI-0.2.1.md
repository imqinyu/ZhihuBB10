# 0.2.1 设置、缓存与列表验收

2026-10-25；BB10 10.3.1.995 模拟器 192.168.106.129。原生 Cascades 1.4，Qt 4.8。

## 修改

- FeedBody ListView 改为 `feedList`；标题最多两行，正文仍最多四行，图片与预览并排。
- FeedPage 用 Page.addAction/removeAction 管理公共入口，仅 home/follow 保留最近浏览、搜索、收藏夹；刷新仍在 overflow。
- SettingsPage 复用用户提供的 BBTiebaUI 设置页布局：外观／缓存／内容，主题下拉框、缓存数字与进度条、512MB／1G／2G 上限、清除按钮、两个内容开关。选项即时保存，并保留创建期间的写回保护。
- 内容开关接到正文大字体、日报护眼模式；大字体同时用于普通正文与新打开的日报正文。
- 缓存包含 feed JSON（含通知）、头像与缩略图、日报请求和图片。四类各占所选总预算的四分之一；本地 JSON 与图片按文件时间回收较旧条目，日报继续使用 Qt 磁盘缓存淘汰。统计显示当前文件大小，每三秒刷新。
- 清理只操作缓存，不清理账号、设置、收藏或浏览记录。正在进行的网络请求完成后可能重新产生缓存。

## 构建

`build.ps1 -Configuration Device-Debug` 和 `build.ps1 -Configuration Simulator-Debug` 均通过；BAR 检查包含生产 QML。日志：build-device-0.2.1.log、build-simulator-0.2.1.log。

静态脚本仍报告先前已核实的两项生产代码例外：Carousel 的 StackListLayout.orientation 存在于真实 SDK；HTTPS 是用户要求并经原生握手验证的直连方式。其他报错来自旧验证目录。新增页面无静态 API 错误，最终原生日志无绑定循环、ReferenceError、TypeError 或不存在的属性错误。

## 原生交互

使用独立 UI Check 应用链接生产控制器与生产 QML，不导入正式应用的账号；用私有命令文件驱动真实 QML 处理器。

- 长标题两行、正文四行、无图满宽与短正文自然高度：`shots/feed-two-line-title.png`。
- home/follow：4 个 action（3 个公共入口和 overflow 刷新）；history/saved/notifications：仅 overflow 刷新。每页动作计数见 `ui-check/final-0.2.1.log`。
- 设置亮暗主题截图：`shots/settings-light-0.2.1.png`、`shots/settings-dark-0.2.1.png`。最近浏览截图：`shots/history-actions-0.2.1.png`。
- 原生下拉框、开关验证主题、1G 上限、大字体、日报护眼模式可保存；应用重启后读取并恢复，`restored ok=true`。
- 129MB 测试图片超过 512MB 总预算下的 128MB 图片分额时被自动回收。
- 同时向两个真实 Qt 日报磁盘缓存和通知／图片缓存放入无凭据测试数据，经页面“清除缓存”按钮处理，占用由 2.4MB 降为 0.0MB；两类日报缓存均失效，通知 JSON 与图片文件删除，设置文件、收藏与历史测试记录逐字节／逐值保留。最终 `bounded=true cleared=true daily stores=2 daily=true library/settings kept=true`。
- 清理后日报首页与热门页仍可打开。最终所有 UI Check 命令 `ok=true`。

0.2.1 正式包随后覆盖升级模拟器，保留应用私有数据；独立测试应用清除。Q20 720×720 真机及长时间连续使用未在本轮验证。

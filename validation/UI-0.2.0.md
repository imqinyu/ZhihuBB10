# 0.2.0 列表与日报界面验收

## 改动

- `assets/FeedBody.qml`：标题移出图文横排，使用全行宽度；正文 `autoSize.maxLineCount: 4`；缩略图放在正文右侧，没图时预览占满宽度。
- `assets/daily/`：从用户提供的 ZhihuDaily-Cascades 复用首页、轮播、日期分组、热门、栏目网格、栏目文章、往日文章、正文与评论页面及原有样式资源。
- `assets/main.qml`：日报 Tab 使用原 `index.qml`；为原页面保留 `common` 和 `nav` 上下文。
- 日报首页三个 action 为热门、分类、往日文章，分类使用 Signature，其余两个 OnBar；均没有指定图标资源，系统目前显示默认占位图标。
- `src/daily/`：复用原请求器、图片控件和工具类；调整为 Application 创建后再初始化网络对象，补齐请求超时、回复回收和缓存目录创建，不关闭证书验证。

## 已验证

在 192.168.106.129 的 BB10 10.3.1.995 模拟器中，使用真实 Cascades 渲染器和生产 QML：

| 场景 | 结果 | 截图 |
|---|---|---|
| 长标题、有图正文、无图正文、短正文 | 标题全宽；两个长预览均为四行，短预览自然高度 | `shots/feed-four-lines.png` |
| 日报首页 | 轮播、圆点、日期分组、原列表样式、三个 action 正常 | `shots/daily-reused-home.png` |
| 热门 action | 原今日热门页打开，真实数据和缩略图正常 | `shots/daily-reused-hot.png` |
| 分类 action | 原栏目分类网格打开，真实数据和缩略图正常 | `shots/daily-reused-sections.png` |
| 栏目条目 | 可进入原栏目文章列表，日期与内容正常 | `shots/daily-reused-section-list.png` |
| 往日文章 action | 原日期选择标题与前后日 action 正常，已加载实际文章 | `shots/daily-reused-before.png` |
| 日报条目 | 原正文页可打开，封面、作者、正文和原底栏正常显示 | `shots/daily-reused-article.png` |
| 返回 | 从栏目列表和子页面返回，导航栈正常 | `ui-check/complete.log` |
| 连续搜索两次 | 不再出现旧 QObject.deleteLater 的 QML 调用错误；测试包未登录，因此业务响应为 403 | `ui-check/complete.log` |

列表截图使用特意构造的长文本和公开图片，以确认真实换行，而非靠字符数猜测行数；日报截图和数据来自真实接口。
UI 测试包独立于用户已登录的正式客户端，不包含账号凭据。

## 构建与复现

```powershell
cd D:\BBWorkspace\Zhihu
.\build.ps1
.\build.ps1 -Configuration Simulator-Debug
```

预期均退出 0，BAR 中包含 `native/assets/main.qml`、`native/assets/daily/pages/index.qml` 及全部日报资源。
原生 UI 检查程序位于 `validation/ui-check/`，复用生产应用启动、网络层与 QML，通过私有 `data/ui-command` 发送测试命令，不向正式应用添加测试入口。

## 静态扫描例外与未验范围

- skill 中的一处提示将 StackListLayout.orientation 判错；NDK `stacklistlayout.h` 与 `plugins.qmltypes` 都明确声明 `orientation`。因此轮播保持原工程的 `orientation: LayoutOrientation.LeftToRight`，原生加载也已通过，不能改为不存在的 `horizontal`。
- HTTPS 为用户已明确选择且已实测的连接方式。通用检查脚本的 HTTPS 黑名单不适用于此项目；没有把这些误报记成检查通过。
- 原 Common.qml 对 Application.applicationVersion 的非 NOTIFY 属性会产生一条绑定提示；不影响本次页面功能。
- Q20 真机、深色主题及日期展开选择器的完整触摸操作还需实机检查；本次不声称这些已验收。
- 部分旧栏目图片由上游 CDN 决定是否可访问，仍保留原工程的加载占位图。

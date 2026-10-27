# 来源与许可

- 界面框架、应用图标和 `assets/icons/`：沿用用户提供的 Zhihu 工程。
- `assets/SettingsPage.qml`：按用户要求复用本机 BBTiebaUI 设置页的外观／缓存／内容布局，
  将业务设置接到知乎与日报；保留三档缓存上限、进度条和正文大字体选项的布局。
- 最近浏览与收藏夹日期分组参考 BBTiebaUI 的 `ThreadLibraryPage.qml`，使用原生
  GroupDataModel、按完整日期分组和 Header；新条目记录各自的浏览／收藏时间。
- `assets/UserDetailPage.qml` 及 `assets/images/profile_background.jpg`：按用户要求参照
  BBTiebaUI 用户详情页的背景、头像、统计行和 SegmentedControl 布局，业务改为知乎提问／回答／文章。
  登录粘贴、账号复制／删除交互及 `icons/ic_copy.png` 同样参照该工程。
- `Zhihu::showImagePreview`：按用户要求参照 BBTiebaUI 的 `openImage` 及共享副本逻辑，
  使用常驻 InvokeManager 调用 `sys.pictures.card.previewer`，改为按真实文件头选择格式，
  在 card 关闭时清理本应用导出的副本。
- `assets/images/question_background.png`：用户提供的问题页题图。
- `src/signature.cpp`：由 Zhihu++ 的 `ZseSigner.kt` 与 `ZhihuFetchSignature.kt` 改写为 C++。
  原作者 zly2006 <i@zly2006.me>，Copyright (C) 2024–2026，AGPL-3.0-or-later。
  上游：https://github.com/zly2006/zhihu-plus-plus 。完整许可见根目录 `LICENSE`。
- `src/phone_login.cpp`、`src/phone_encrypt.cpp`：由同一 Zhihu++ 工程的
  `ZhihuPhoneLoginClient.kt`、`ZhihuMessageBodyEncryptor.kt` 移植为 Qt 4.8 异步实现，
  保留协议查表数据和字节变换。Copyright (C) 2024–2026 zly2006，AGPL-3.0-only。
- `assets/CommentItem.qml`：复用 liuwanlin 的日报 `pages/child/CommentList.qml`
  评论条目布局，接入统一的知乎／日报评论模型；`CommentsPage.qml` 去掉长短评分段。
- 推荐、关注、搜索、问题回答和通知的接口路径及字段，参考用户提供的
  `zhihu-plus-plus-master/shared/src/commonMain/kotlin/com/github/zly2006/zhihu/`。
  客户端的 C++ / Cascades 实现为本项目新写；没有移植 Android UI。
- `assets/daily/` 与 `src/daily/`：按用户要求复用 liuwanlin 的
  `ZhihuDaily-Cascades-master`，包括首页、轮播、列表项、热门、栏目分类、栏目文章、
  过往文章、正文及评论页面，以及配套样式、图片、请求器、图片控件和工具类。
  保留原有文件头与作者信息；修改仅涉及资源命名空间、宿主导航、三个首页 action、
  C++ 初始化时机、超时及缓存适配等。未接入原项目的赞助、开发者消息或应用菜单。

本项目包含 AGPL 派生代码，作为整体分发时应遵守所附 AGPL-3.0-or-later，提供相应源代码。
知乎商标及返回内容的权利归原权利人所有。

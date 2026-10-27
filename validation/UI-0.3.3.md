# 0.3.3 图片预览 card 与界面微调

2026-10-27，原生 Cascades 1.4 / Qt 4.8，模拟器 192.168.106.129。

## 实现

- DetailPage 中的正文图片生成原生预览链接，继续关闭 WebView JavaScript 和 Cookie。
  点击通过既有导航事件交给 C++；普通知乎链接仍走内容页，其他链接仍走浏览器。
  日报正文的图片消息与题图点击也接入同一个原生入口。
- 参照 BBTiebaUI，用常驻 InvokeManager、bb.action.VIEW 和 sys.pictures.card.previewer。
  系统 card 无法读取应用私有缓存，所以仅将选中图片导出至
  /accounts/1000/shared/misc/zhihu-preview，声明 access_shared 权限。
  关闭 card、调用失败或应用正常退出时清理副本；重新导出时也回收遗留的同名副本。
- 缓存统一用 .jpg 文件名，原生验证发现旧 Qt 的图片格式推断不能识别这份 PNG。
  改为检查 PNG、JPEG、GIF、BMP 文件头，给共享副本正确扩展名与 MIME，其他格式提示暂不支持。
  图片尚未缓存时沿用现有下载队列，完成后自动打开；凭据不发给图片 CDN。
  原生入口拒绝 file: 和非知乎图片域名，网页不能借此读取应用私有文件。
- CommentItem 在 ListItem.view 尚未建立时不再直接调用空 units；引用展开改用
  autoSize.resetMaxLineCount()，引用文字更换时重新折叠为两行。
- FeedBody 填满父容器，EmptyState 的内容宽度跟随实际视口，修复用户页偏左。
- 登录方式放进原生分段 TitleBar；Cookie、手机号及短信验证码的粘贴按钮与输入框同行。
  用户已注释的说明与底部粘贴 action 保持注释状态。

## 验证

- ARM 与 x86 构建、BAR 打包均通过，版本 0.3.3。日志 build-device-0.3.3.log、build-simulator-0.3.3.log。
- 原生登录布局检查确认 TitleBarKind.Segmented，正文不再包含 SegmentedControl；输入框和粘贴按钮
  具有同一行父容器。完整 Cookie 自动提取和剪贴板恢复检查通过。见 ui-check/login-0.3.3.log。
- 公开资料空列表夹具测量：视口宽 768，窗口宽 768；截图确认图标、标题、说明和按钮居中。
  见 ui-check/profile-0.3.3.log 与 shots/profile-empty-0.3.3.png。
- 评论夹具四条正确显示，引用展开后的 maxLineCount 为 -1；这是调用 SDK 重置方法的结果，
  不再向 setter 写入非法负值。见 ui-check/comments-0.3.3.log、shots/comments-0.3.3.png。
- 图片验证使用隔离应用中的缓存 PNG：从正文生成的链接进入相同的 openLink 生产路径，
  invocation 结果 error=0、errorCode=0，共享文件头正确；系统子 card 可关闭，
  childCardDone 后记录 shared copy removed=true，文件确实消失。
  见 ui-check/image-native-0.3.3.log、image-close-0.3.3.log。
- 本轮没有新增实际账号凭据；隔离应用的测试内容与正式账号、收藏和历史条目分开。
- 静态检查仍有三项已审阅的生产例外：Carousel 的 StackListLayout.orientation 已由 SDK
  确认存在；原网络层和手机号协议的 HTTPS 按用户要求保留证书校验。没有新增其他生产错误。

## 部署

确认正式应用停止后覆盖安装 0.3.3.1，启动 PID 1093963828；正式应用数据保留。
app-0.3.3.log 中场景创建成功，首页从 6 条追加至 12 条，没有新增 QML 运行错误。
隔离 QA 应用已停止并卸载，共享预览副本已清理，临时截图控制器已停止。

## 边界

图片 card 已在模拟器完成调用／关闭／清理链路验证，未在 Q20 或 Z30 真机验收。
PNG 使用本地夹具；本轮未逐个验证 JPEG／GIF／BMP 和下载未完成时的线上重试分支。
截图由隔离 QA 应用读取自身原生窗口，图片 card 的像素界面未截图。

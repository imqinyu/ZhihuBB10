# 真实账号只读联调

日期：2026-10-25。BB10 10.3.1.995 模拟器：192.168.106.129。
测试由用户提供的两个 Cookie 完成，未忽略证书错误，未使用电脑代理。
本文件不包含 Cookie、账号 ID、昵称、签名请求头或返回正文。

## 实测结果

| 能力 | BB10 实际请求 | HTTP | 结果 |
|---|---|---:|---|
| 账号验证 | `api.zhihu.com/me` | 200 | 有效用户资料，生产账号验证流程成功 |
| 推荐 | `api.zhihu.com/topstory/recommend` | 200 | 原始响应 6 条，生产列表模型 6 条 |
| 搜索 | `api.zhihu.com/search_v3` | 200 | 限定 5 条的探针返回 5 条；正式搜索参数解析 18 条可读内容 |
| 关注 | `api.zhihu.com/moments` | 200 | 原始响应 7 条，生产模型解析 2 条支持的回答/文章内容 |
| 通知 | `api.zhihu.com/notifications/v3/message/v3` | 200 | 生产通知类别模型 4 项 |
| 日报 | `news-at.zhihu.com/api/4/news/latest` | 200 | 4 条；该公开请求不携带账号 Cookie |
| 问题回答 | `api.zhihu.com/questions/{id}/feeds` | 200 | 正式模型解析 17 个回答 |
| 回答正文 | `api.zhihu.com/answers/{id}?include=content,...` | 200 | 正式正文渲染模型生成 4593 字符 HTML |

正式客户端的生产日志还记录了首页 `count=6` 到 `count=12` 的追加，说明分页请求与模型追加已实际执行。
所有数量都是该次请求快照，刷新后可以变化；关注、搜索和问题列表会跳过当前未支持的内容类型。

电脑端系统 curl 的账号、推荐和搜索也均返回 200。旧的 `www.zhihu.com/api/...` 路径在 BB10 对照实验中出现 SSL 握手失败（HTTP 尚未发生），因此客户端已采用经过实际请求验证的移动 API 路径，并将旧网页分页路径转换到相应移动接口。

## 可检查证据

- `auth-probe/results-computer.json`：电脑端状态、条数和账号有效性布尔值。
- `auth-probe/results-native.json`：旧网页路径与移动域名的原生对照结果。
- `auth-probe/results-native-mobile.json`：转换路径后的六个原生 API 均为 HTTP 200。
- `auth-probe/results-native-models.json`：生产网络层、真实账号验证、生产 Feed 模型的结果。分页能力以生产日志为准，模型快照中的 `hasMore` 可能取自该轮替换模型的中间信号。
- `client-authenticated-fixed.log`：正式客户端登录后加载真实数据及分页。
- `auth-probe/main.cpp`：复用 `src/zhihu.cpp` 和 `src/signature.cpp` 的原生测试程序，没有复制一套网络实现。

## 复现与凭据处理

`auth-probe/` 使用 NDK qmake 构建，配置、打包和部署方法同 HTTPS 探针。启动后等待从应用私有 `data/auth-input` 读取 Cookie；文件被消费后立即删除。
Cookie 通过标准输入和受保护临时文件传递，不作为命令行参数、源码或打包资源。临时电脑副本已删除，测试应用及其私有登录数据在取回无敏感结果后已卸载。

验证过的账号已导入模拟器正式 Zhihu 客户端的私有账号存储，方便用户继续测试；提供的 BAR 安装包本身不包含登录凭据。退出登录和移除账号入口位于账号管理中。

## 边界

- Q20 真机尚未测试。
- 当前验证的是核心读取能力，未验证发布、点赞、评论、私信、云端收藏等写操作。
- 完整页面交互、离线缓存恢复和所有内容类型尚未全部验收。

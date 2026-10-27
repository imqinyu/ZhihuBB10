# 原生 HTTPS 实测记录

日期：2026-10-25。目标：192.168.106.129，BB10 10.3.1.995 模拟器，Qt 4.8.6。
实测请求不带登录 Cookie，没有调用 `ignoreSslErrors()`，没有修改设备信任库。

## 证据

| 执行方式 | 结果 | 原始日志 |
|---|---|---|
| Qt 默认 TLS + 原生证书校验 | 图片 200；推荐、日报、搜索、回答 403；无证书错误 | `tls-probe/results-default.log` |
| Qt AnyProtocol | 图片 200；部分 API 403，部分握手失败 | `tls-probe/results-anyprotocol.log` |
| Qt SecureProtocols + 只允许 ECDHE/GCM | 测试域名均握手失败 | `tls-probe/results-gcm.log` |
| 系统 libcurl + OpenSSL 强制 TLS 1.2 + 可信 CA | 推荐和日报 HTTP 403，curl 返回成功，无证书错误 | `tls-probe/results-native-tls12.log` |
| 客户端 HTTPS 直连版 | QML 启动成功，首页显示安全验证要求 | `client-direct.log`、`shots/https-direct.png` |

初版 Python/电脑代理已从产品中撤除，相关临时试验文件不参与构建或打包。

默认探针记录的 cipher 为 `ECDHE-RSA-AES256-SHA`，Qt 的 `protocolString()` 输出 `SSLv3`。
这是 cipher 的协议标签，**不能当作实际协商出的 TLS 版本**。强制 TLS 1.2 的验证来自独立的 libcurl/SSL_CTX 探针。
libcurl 试验的根证书 PEM 来自当前 Windows 的可信服务器根证书集合，仅用于该试验，不向设备系统证书库写入。

## 复现

在 `tls-probe/` 中设置 `QNX_HOST`、`QNX_TARGET` 和 NDK `usr/bin` 的 PATH，执行：

```powershell
qmake -spec C:/NDKFiles/target_10_3_1_995/qnx6/usr/share/qt4/mkspecs/blackberry-x86-qcc CONFIG+=simulator CONFIG+=debug -o Makefile.simulator TlsProbe.pro
make -f Makefile.simulator
```

默认是 Qt 原生验证；增加 `CONFIG+=tls_gcm` 做 GCM 对照；增加 `CONFIG+=tls_curl` 做系统 libcurl 强制 TLS 1.2 对照。
设备版本用 `blackberry-armv7le-qcc` 与 `CONFIG+=device`。
使用 descriptor 的 `Simulator-Debug` 或 `Device-Debug` 配置打包。

```powershell
java -jar C:/NDKFiles/host_10_3_1_12/win32/x86/usr/lib/BarDeploy.jar -installApp -noSync -device 192.168.106.129 -password <开发密码> -package TlsProbe.bar
java -jar C:/NDKFiles/host_10_3_1_12/win32/x86/usr/lib/BarDeploy.jar -launchApp -device 192.168.106.129 -password <开发密码> -package TlsProbe.bar
java -jar C:/NDKFiles/host_10_3_1_12/win32/x86/usr/lib/BarDeploy.jar -getFile logs/log results.log -device 192.168.106.129 -password <开发密码> -package TlsProbe.bar
```

首次安装后等待探针输出 COMPLETE。覆盖安装前先终止应用，并确认终止成功，再增加版本号；不要在应用仍运行或 closing 时覆盖。
本机增量部署曾出现认证失败，`-noSync` 完整部署可成功。启动日志从 `logs/log` 获取。

## 尚未证明的内容

- 没有真实登录 Cookie，不能证明签名、Cookie 导入能解除 403。
- 没有 Q20 真机的 TLS/接口结果。
- 403 可能与登录态、请求特征、网络环境或服务端策略有关；仅凭这些响应不能确认具体触发因素。
- 原生客户端真实内容列表、正文、分页和账号切换的服务端联调未完成。

skill 的静态脚本会将任意 `https://` 当作错误；本次 HTTPS 是用户明确要求、且已做原生实验的例外，保留原始扫描输出，不把该规则的误报宣称为全量静态检查通过。

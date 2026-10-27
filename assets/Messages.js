.pragma library
function message(code) {
    var texts = {
        invalid_url: "接口地址无效。",
        redirect: "知乎重定向了请求，可能需要登录或完成安全验证。",
        tls_failed: "HTTPS 握手或证书校验失败，请检查设备时间与网络。",
        settings_saved: "设置已保存",
        cache_cleared: "缓存已清除",
        image_loading: "正在准备图片",
        image_load_failed: "图片加载失败，请稍后重试。",
        image_unsupported: "系统预览暂不支持这张图片的格式。",
        image_shared_failed: "无法准备图片，请检查共享文件权限。",
        image_preview_failed: "系统图片预览打开失败，请重试。",
        cache_clear_partial: "部分缓存仍在使用，请稍后重试。",
        login_required: "登录后可查看关注和通知，请从账号管理导入账号。",
        invalid_cookie: "登录凭据格式有误，请重新复制。",
        invalid_cookie_values: "请分别填写有效的 z_c0 和 d_c0，只需输入值本身。",
        invalid_phone: "请输入正确的中国大陆手机号。",
        invalid_digits: "请输入 4 至 8 位短信验证码。",
        phone_changed: "手机号已改变，请为当前号码重新获取验证码。",
        invalid_captcha: "图形验证码不正确，请重新输入或换一张。",
        missing_device_cookie: "未能取得设备凭据，请稍后重试。",
        missing_session_cookie: "这份 Cookie 未通过登录验证，且缺少 z_c0。document.cookie 无法读取 HttpOnly 登录凭据，请从浏览器 Network 请求头复制完整 Cookie。",
        credential_copied: "凭据已复制",
        clipboard_failed: "无法写入剪贴板",
        unauthorized: "登录已失效，请重新导入账号。",
        forbidden: "知乎要求安全验证，请在现代浏览器完成验证后重试。",
        rate_limited: "请求过于频繁，请稍后再试。",
        timeout: "连接超时，请检查网络。",
        network: "无法连接知乎，请检查网络。",
        invalid_response: "接口未返回预期内容，请稍后重新加载。",
        account_verified: "账号验证成功",
        saved: "已加入本地收藏",
        unsaved: "已移出本地收藏"
    };
    if (texts[code]) return texts[code];
    if (code.indexOf("server:") === 0) return code.substring(7);
    if (code.indexOf("http_") === 0) return "服务器返回错误（" + code.substring(5) + "），请稍后重试。";
    return code || "";
}

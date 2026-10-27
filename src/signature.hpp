// SPDX-License-Identifier: AGPL-3.0-or-later
// Adapted from Zhihu++ ZseSigner.kt (C) 2024-2026 zly2006 <i@zly2006.me>.
#ifndef ZHIHU_SIGNATURE_HPP
#define ZHIHU_SIGNATURE_HPP
#include <QByteArray>
#include <QUrl>
QByteArray zhihuSignature(const QUrl &url, const QByteArray &dc0);
#endif

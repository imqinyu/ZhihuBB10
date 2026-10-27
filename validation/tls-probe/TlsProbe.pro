APP_NAME = TlsProbe
CONFIG += qt warn_on cascades10
QT += network
contains(CONFIG, tls_curl) {
    DEFINES += ZHIHU_TLS_CURL_PROBE
    LIBS += -lcurl -lssl -lcrypto
}
contains(CONFIG, tls_gcm) { DEFINES += ZHIHU_TLS_GCM_PROBE }
SOURCES += main.cpp
HEADERS += probe.hpp

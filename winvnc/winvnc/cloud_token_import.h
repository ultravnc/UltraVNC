#pragma once

// CloudTokenImportListener: temporary loopback HTTP listener used to receive
// the portal token after the user logs in on https://portal.uvnc.com.
//
// Flow: dialog starts listener on 127.0.0.1:<ephemeral port>, opens the portal
// import page with that callback URL plus a random state nonce, then waits for
// the browser redirect carrying token=<token>&state=<nonce>.

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <chrono>
#include <cstdint>
#include <cstring>

#pragma comment(lib, "ws2_32.lib")
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")

class CloudTokenImportListener {
public:
    struct Result {
        bool ok = false;
        std::string token;
        std::string error;
    };

    ~CloudTokenImportListener() { Stop(); }

    // Bind a listen socket on 127.0.0.1:<any> and generate the state nonce.
    bool Start(uint16_t& outPort, std::string& outState) {
        listenSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listenSocket_ == INVALID_SOCKET) {
            lastError_ = "Cannot create socket";
            return false;
        }
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = 0;
        if (::bind(listenSocket_, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR ||
            listen(listenSocket_, 1) == SOCKET_ERROR) {
            lastError_ = "Cannot start local listener";
            Stop();
            return false;
        }
        sockaddr_in bound{};
        int len = sizeof(bound);
        if (getsockname(listenSocket_, (sockaddr*)&bound, &len) == SOCKET_ERROR) {
            lastError_ = "Cannot query listener port";
            Stop();
            return false;
        }
        outPort = ntohs(bound.sin_port);
        outState = state_ = GenerateState();
        return !state_.empty();
    }

    // Block until the browser callback arrives or timeoutMs elapses.
    Result WaitForCallback(int timeoutMs) {
        Result res;
        if (listenSocket_ == INVALID_SOCKET) {
            res.error = lastError_.empty() ? "Listener not started" : lastError_;
            return res;
        }

        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(listenSocket_, &fds);
        timeval tv{};
        tv.tv_sec = timeoutMs / 1000;
        tv.tv_usec = (timeoutMs % 1000) * 1000;
        if (select(0, &fds, nullptr, nullptr, &tv) <= 0) {
            res.error = "Timed out waiting for portal login";
            Stop();
            return res;
        }

        sockaddr_in client{};
        int clientLen = sizeof(client);
        SOCKET conn = accept(listenSocket_, (sockaddr*)&client, &clientLen);
        if (conn == INVALID_SOCKET) {
            res.error = "Accept failed";
            Stop();
            return res;
        }

        DWORD rcvTimeout = 5000;
        setsockopt(conn, SOL_SOCKET, SO_RCVTIMEO, (char*)&rcvTimeout, sizeof(rcvTimeout));
        std::string req;
        char buf[4096];
        while (req.find("\r\n\r\n") == std::string::npos && req.size() < 16384) {
            int n = recv(conn, buf, sizeof(buf), 0);
            if (n <= 0) break;
            req.append(buf, n);
        }

        if (!HandleRequest(req, res)) {
            if (res.error.empty()) res.error = "Invalid token import request";
            SendResponse(conn, 400, "Token import failed: invalid request.");
        } else {
            SendResponse(conn, 200,
                "Token imported into UltraVNC.<br>You can close this tab and return to the application.");
        }

        closesocket(conn);
        Stop();
        return res;
    }

    void Stop() {
        if (listenSocket_ != INVALID_SOCKET) {
            closesocket(listenSocket_);
            listenSocket_ = INVALID_SOCKET;
        }
    }

    // Build the portal URL that performs login + token redirect.
    static std::string BuildPortalUrl(const char* role, uint16_t port, const std::string& state) {
        std::string callback = "http://127.0.0.1:" + std::to_string(port) + "/callback";
        return std::string("https://portal.uvnc.com/matchmaker/import.php?role=")
            + role + "&callback=" + UrlEncode(callback) + "&state=" + UrlEncode(state);
    }

    static std::string UrlEncode(const std::string& s) {
        std::string out;
        char hex[4];
        for (char c : s) {
            if (isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.' || c == '~')
                out += c;
            else {
                snprintf(hex, sizeof(hex), "%%%02X", (unsigned char)c);
                out += hex;
            }
        }
        return out;
    }

private:
    static std::string GenerateState() {
        uint8_t bytes[16];
        if (BCryptGenRandom(nullptr, bytes, sizeof(bytes), BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0)
            return "";
        static const char* hexd = "0123456789abcdef";
        std::string out;
        out.reserve(32);
        for (uint8_t b : bytes) {
            out += hexd[b >> 4];
            out += hexd[b & 0x0f];
        }
        return out;
    }

    static std::string UrlDecode(const std::string& s) {
        std::string out;
        for (size_t i = 0; i < s.size(); i++) {
            if (s[i] == '%' && i + 2 < s.size()) {
                auto hv = [](char c) -> int {
                    if (c >= '0' && c <= '9') return c - '0';
                    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
                    return -1;
                };
                int hi = hv(s[i + 1]), lo = hv(s[i + 2]);
                if (hi >= 0 && lo >= 0) {
                    out += (char)((hi << 4) | lo);
                    i += 2;
                    continue;
                }
            }
            out += (s[i] == '+') ? ' ' : s[i];
        }
        return out;
    }

    bool HandleRequest(const std::string& req, Result& res) {
        // Expect: GET /callback?token=...&state=... HTTP/1.x
        if (req.compare(0, 4, "GET ") != 0) return false;
        size_t pathEnd = req.find(' ', 4);
        if (pathEnd == std::string::npos) return false;
        std::string path = req.substr(4, pathEnd - 4);
        if (path.compare(0, 10, "/callback?") != 0 && path != "/callback") return false;

        size_t q = path.find('?');
        if (q == std::string::npos) return false;
        std::string query = path.substr(q + 1);

        std::string token, state;
        size_t pos = 0;
        while (pos <= query.size()) {
            size_t amp = query.find('&', pos);
            std::string kv = query.substr(pos, amp == std::string::npos ? std::string::npos : amp - pos);
            size_t eq = kv.find('=');
            if (eq != std::string::npos) {
                std::string k = kv.substr(0, eq);
                std::string v = UrlDecode(kv.substr(eq + 1));
                if (k == "token") token = v;
                else if (k == "state") state = v;
            }
            if (amp == std::string::npos) break;
            pos = amp + 1;
        }

        if (state.empty() || state != state_) {
            res.error = "State mismatch";
            return false;
        }
        if (token.empty()) {
            res.error = "Portal returned no token";
            return false;
        }
        res.ok = true;
        res.token = token;
        return true;
    }

    static void SendResponse(SOCKET conn, int code, const char* message) {
        std::string body = std::string(
            "<html><body style=\"font-family:Segoe UI,Arial,sans-serif;background:#1a1a2e;"
            "color:#eee;text-align:center;padding-top:80px\">")
            + "<h2>" + message + "</h2></body></html>";
        std::string hdr = "HTTP/1.1 " + std::to_string(code)
            + (code == 200 ? " OK" : " Bad Request")
            + "\r\nContent-Type: text/html\r\nContent-Length: "
            + std::to_string(body.size()) + "\r\nConnection: close\r\n\r\n";
        std::string resp = hdr + body;
        send(conn, resp.c_str(), (int)resp.size(), 0);
    }

    SOCKET listenSocket_ = INVALID_SOCKET;
    std::string state_;
    std::string lastError_;
};

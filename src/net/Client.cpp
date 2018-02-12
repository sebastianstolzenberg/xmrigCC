/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2016-2017 XMRig       <support@xmrig.com>
 *
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <inttypes.h>
#include <iterator>
#include <stdio.h>
#include <string.h>
#include <utility>

#include "interfaces/IClientListener.h"
#include "log/Log.h"
#include "net/Client.h"
#include "net/Url.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"


#ifdef XMRIG_PROXY_PROJECT
#   include "proxy/JobResult.h"
#else
#   include "net/JobResult.h"
#endif


#ifdef _MSC_VER
#   define strncasecmp(x,y,z) _strnicmp(x,y,z)
#endif



int64_t Client::m_sequence = 1;


Client::Client(int id, const char *agent, IClientListener *listener) :
    m_quiet(false),
    m_agent(agent),
    m_listener(listener),
    m_id(id),
    m_retryPause(5000),
    m_failures(0),
    m_recvBufPos(0),
    m_state(UnconnectedState),
    m_expire(0)
{
    memset(m_ip, 0, sizeof(m_ip));
    memset(&m_hints, 0, sizeof(m_hints));

#   ifndef XMRIG_PROXY_PROJECT
    m_keepAliveTimer.data = this;
    uv_timer_init(uv_default_loop(), &m_keepAliveTimer);
#   endif
}


Client::~Client()
{
}


void Client::connect()
{
    m_connection = establishConnection(shared_from_this(),
                                       CONNECTION_TYPE_AUTO,
                                       m_url.host(), m_url.port());
    login();
}


/**
 * @brief Connect to server.
 *
 * @param url
 */
void Client::connect(const Url *url)
{
    setUrl(url);
    connect();
}


void Client::disconnect()
{
#   ifndef XMRIG_PROXY_PROJECT
    uv_timer_stop(&m_keepAliveTimer);
#   endif

    m_expire   = 0;
    m_failures = -1;

    close();
}


void Client::setUrl(const Url *url)
{
    if (!url || !url->isValid()) {
        return;
    }

    m_url = url;
}


void Client::tick(uint64_t now)
{
    if (m_expire == 0 || now < m_expire) {
        return;
    }

    if (m_connection) {
        LOG_DEBUG_ERR("[%s:%u] timeout", m_url.host(), m_url.port());
        reconnect();
    }
    else {
        connect();
    }
}


int64_t Client::submit(const JobResult &result)
{
#   ifdef XMRIG_PROXY_PROJECT
    const char *nonce = result.nonce;
    const char *data  = result.result;
#   else
    char nonce[9];
    char data[65];

    Job::toHex(reinterpret_cast<const unsigned char*>(&result.nonce), 4, nonce);
    nonce[8] = '\0';

    Job::toHex(result.result, 32, data);
    data[64] = '\0';
#   endif

    const int size = snprintf(m_sendBuf, sizeof(m_sendBuf),
                              "{"
                                  "\"id\":%" PRIu64 ","
                                  "\"jsonrpc\":\"2.0\","
                                  "\"method\":\"submit\","
                                  "\"params\":"
                                      "{"
                                          "\"id\":\"%s\","
                                          "\"job_id\":\"%s\","
                                          "\"nonce\":\"%s\","
                                          "\"result\":\"%s\""
                                      "}"
                               "}\n",
                                m_sequence, m_rpcId, result.jobId.data(), nonce, data);

    m_results[m_sequence] = SubmitResult(m_sequence, result.diff, result.actualDiff());
    return send(m_sendBuf, size);
}


bool Client::isCriticalError(const char *message)
{
    if (!message) {
        return false;
    }

    if (strncasecmp(message, "Unauthenticated", 15) == 0) {
        return true;
    }

    if (strncasecmp(message, "your IP is banned", 17) == 0) {
        return true;
    }

    if (strncasecmp(message, "IP Address currently banned", 27) == 0) {
        return true;
    }

    return false;
}


bool Client::parseJob(const rapidjson::Value &params, int *code)
{
    if (!params.IsObject()) {
        *code = 2;
        return false;
    }

    Job job(m_id, m_url.isNicehash());
    if (!job.setId(params["job_id"].GetString())) {
        *code = 3;
        return false;
    }

    if (!job.setBlob(params["blob"].GetString())) {
        *code = 4;
        return false;
    }

    if (!job.setTarget(params["target"].GetString())) {
        *code = 5;
        return false;
    }

    if (m_job == job) {
        if (!m_quiet) {
            LOG_WARN("[%s:%u] duplicate job received, reconnect", m_url.host(), m_url.port());
        }

        close();
        return false;
    }

    m_job = std::move(job);
    return true;
}


bool Client::parseLogin(const rapidjson::Value &result, int *code)
{
    const char *id = result["id"].GetString();
    if (!id || strlen(id) >= sizeof(m_rpcId)) {
        *code = 1;
        return false;
    }

    memset(m_rpcId, 0, sizeof(m_rpcId));
    memcpy(m_rpcId, id, strlen(id));

    return parseJob(result["job"], code);
}


int64_t Client::send(char* buf, size_t size)
{
    LOG_DEBUG("[%s:%u] send (%d bytes): \"%s\"", m_url.host(), m_url.port(), size, buf);
    if (!m_connection) {
        LOG_DEBUG_ERR("[%s:%u] send failed", m_url.host(), m_url.port());
        return -1;
    }

    m_connection->send(buf, size);

    m_expire = uv_now(uv_default_loop()) + kResponseTimeout;
    return m_sequence++;
}


void Client::close()
{
    m_connection.reset();
}


void Client::login()
{
    m_results.clear();

    rapidjson::Document doc;
    doc.SetObject();

    auto &allocator = doc.GetAllocator();

    doc.AddMember("id",      1,       allocator);
    doc.AddMember("jsonrpc", "2.0",   allocator);
    doc.AddMember("method",  "login", allocator);

    rapidjson::Value params(rapidjson::kObjectType);
    params.AddMember("login", rapidjson::StringRef(m_url.user()),     allocator);
    params.AddMember("pass",  rapidjson::StringRef(m_url.password()), allocator);
    params.AddMember("agent", rapidjson::StringRef(m_agent),          allocator);

    doc.AddMember("params", params, allocator);

    rapidjson::StringBuffer buffer(0, 512);
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    const size_t size = buffer.GetSize();
    if (size > (sizeof(m_buf) - 2)) {
        return;
    }

    memcpy(m_sendBuf, buffer.GetString(), size);
    m_sendBuf[size]     = '\n';
    m_sendBuf[size + 1] = '\0';

    send(m_sendBuf, size + 1);
}


void Client::parse(char *line, size_t len)
{
    startTimeout();

    line[len - 1] = '\0';

    LOG_DEBUG("[%s:%u] received (%d bytes): \"%s\"", m_url.host(), m_url.port(), len, line);

    rapidjson::Document doc;
    if (doc.ParseInsitu(line).HasParseError()) {
        if (!m_quiet) {
            LOG_ERR("[%s:%u] JSON decode failed: \"%s\"", m_url.host(), m_url.port(), rapidjson::GetParseError_En(doc.GetParseError()));
        }

        return;
    }

    if (!doc.IsObject()) {
        return;
    }

    const rapidjson::Value &id = doc["id"];
    if (id.IsInt64()) {
        parseResponse(id.GetInt64(), doc["result"], doc["error"]);
    }
    else {
        parseNotification(doc["method"].GetString(), doc["params"], doc["error"]);
    }
}


void Client::parseNotification(const char *method, const rapidjson::Value &params, const rapidjson::Value &error)
{
    if (error.IsObject()) {
        if (!m_quiet) {
            LOG_ERR("[%s:%u] error: \"%s\", code: %d", m_url.host(), m_url.port(), error["message"].GetString(), error["code"].GetInt());
        }
        return;
    }

    if (!method) {
        return;
    }

    if (strcmp(method, "job") == 0) {
        int code = -1;
        if (parseJob(params, &code)) {
            m_listener->onJobReceived(this, m_job);
        }

        return;
    }

    LOG_WARN("[%s:%u] unsupported method: \"%s\"", m_url.host(), m_url.port(), method);
}


void Client::parseResponse(int64_t id, const rapidjson::Value &result, const rapidjson::Value &error)
{
    if (error.IsObject()) {
        const char *message = error["message"].GetString();

        auto it = m_results.find(id);
        if (it != m_results.end()) {
            it->second.done();
            m_listener->onResultAccepted(this, it->second, message);
            m_results.erase(it);
        }
        else if (!m_quiet) {
            LOG_ERR("[%s:%u] error: \"%s\", code: %d", m_url.host(), m_url.port(), message, error["code"].GetInt());
        }

        if (id == 1 || isCriticalError(message)) {
            close();
        }

        return;
    }

    if (!result.IsObject()) {
        return;
    }

    if (id == 1) {
        int code = -1;
        if (!parseLogin(result, &code)) {
            if (!m_quiet) {
                LOG_ERR("[%s:%u] login error code: %d", m_url.host(), m_url.port(), code);
            }

            return close();
        }

        m_failures = 0;
        m_listener->onLoginSuccess(this);
        m_listener->onJobReceived(this, m_job);
        return;
    }

    auto it = m_results.find(id);
    if (it != m_results.end()) {
        it->second.done();
        m_listener->onResultAccepted(this, it->second, nullptr);
        m_results.erase(it);
    }
}


void Client::ping()
{
    const int size = snprintf(m_sendBuf, sizeof(m_sendBuf),
                              "{"
                                  "\"id\":%" PRId64 ","
                                  "\"jsonrpc\":\"2.0\","
                                  "\"method\":\"keepalived\","
                                  "\"params\":"
                                      "{"
                                          "\"id\":\"%s\""
                                      "}"
                              "}\n",
                              m_sequence, m_rpcId);
    send(m_sendBuf, size);
}


void Client::reconnect()
{
    close();

#   ifndef XMRIG_PROXY_PROJECT
    if (m_url.isKeepAlive()) {
        uv_timer_stop(&m_keepAliveTimer);
    }
#   endif

    if (m_failures == -1) {
        return m_listener->onClose(this, -1);
    }

    m_failures++;
    m_listener->onClose(this, (int) m_failures);

    m_expire = uv_now(uv_default_loop()) + m_retryPause;
}


void Client::startTimeout()
{
    m_expire = 0;

#   ifndef XMRIG_PROXY_PROJECT
    if (!m_url.isKeepAlive()) {
        return;
    }

    uv_timer_start(&m_keepAliveTimer,
                   [](uv_timer_t *handle) { getClient(handle->data)->ping(); },
                   kKeepAliveTimeout, 0);
#   endif
}

void Client::onReceived(char* data, std::size_t size)
{
//    if ((size_t) size > (sizeof(m_buf) - 8 - m_recvBufPos)) {
    //    TODO reconnect
//        return close();
//    }

    m_recvBufPos += size;

    char* end;
    char* start = data;
    size_t remaining = m_recvBufPos;

    while ((end = static_cast<char*>(memchr(start, '\n', remaining))) != nullptr) {
        end++;
        size_t len = end - start;
        parse(start, len);

        remaining -= len;
        start = end;
    }

    if (remaining == 0) {
        m_recvBufPos = 0;
        return;
    }

    if (start == data) {
        return;
    }

    memcpy(data, start, remaining);
    m_recvBufPos = remaining;
}

void Client::onError(const std::string& error)
{
    if (!m_quiet) {
        LOG_ERR("[%s:%u] error: \"%s\"", m_url.host(), m_url.port(), error);
    }
    close();
}

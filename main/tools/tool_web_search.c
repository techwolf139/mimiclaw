#include "tool_web_search.h"
#include "mimi_config.h"
#include "proxy/http_proxy.h"

#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_heap_caps.h"
#include "nvs.h"
#include "cJSON.h"

static const char *TAG = "web_search";

static char s_jina_key[256] = {0};

#define SEARCH_BUF_SIZE     (32 * 1024)

/* ── Response accumulator ─────────────────────────────────────── */

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} search_buf_t;

static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    search_buf_t *sb = (search_buf_t *)evt->user_data;
    if (evt->event_id == HTTP_EVENT_ON_DATA) {
        size_t needed = sb->len + evt->data_len;
        if (needed < sb->cap) {
            memcpy(sb->data + sb->len, evt->data, evt->data_len);
            sb->len += evt->data_len;
            sb->data[sb->len] = '\0';
        }
    }
    return ESP_OK;
}

/* ── Init ─────────────────────────────────────────────────────── */

esp_err_t tool_web_search_init(void)
{
    /* Start with build-time default */
    if (MIMI_SECRET_JINA_KEY[0] != '\0') {
        strncpy(s_jina_key, MIMI_SECRET_JINA_KEY, sizeof(s_jina_key) - 1);
    }

    /* NVS overrides take highest priority (set via CLI) */
    nvs_handle_t nvs;
    if (nvs_open(MIMI_NVS_SEARCH, NVS_READONLY, &nvs) == ESP_OK) {
        char tmp[256] = {0};
        size_t len = sizeof(tmp);
        if (nvs_get_str(nvs, MIMI_NVS_KEY_API_KEY, tmp, &len) == ESP_OK && tmp[0]) {
            strncpy(s_jina_key, tmp, sizeof(s_jina_key) - 1);
        }
        nvs_close(nvs);
    }

    if (s_jina_key[0]) {
        ESP_LOGI(TAG, "Jina search initialized (key configured)");
    } else {
        ESP_LOGW(TAG, "No search API key. Use CLI: set_search_key <KEY>");
    }
    return ESP_OK;
}

/* ── URL-encode a query string ────────────────────────────────── */

static size_t url_encode(const char *src, char *dst, size_t dst_size)
{
    static const char hex[] = "0123456789ABCDEF";
    size_t pos = 0;

    for (; *src && pos < dst_size - 3; src++) {
        unsigned char c = (unsigned char)*src;
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
            dst[pos++] = c;
        } else if (c == ' ') {
            dst[pos++] = '+';
        } else {
            dst[pos++] = '%';
            dst[pos++] = hex[c >> 4];
            dst[pos++] = hex[c & 0x0F];
        }
    }
    dst[pos] = '\0';
    return pos;
}

/* ── Format markdown results ───────────────────────────────────── */

static void format_results(const char *markdown, char *output, size_t output_size)
{
    if (!markdown || markdown[0] == '\0') {
        snprintf(output, output_size, "No web results found.");
        return;
    }

    /* Jina returns markdown. We extract first few sections. */
    size_t off = 0;
    int link_count = 0;
    const int max_links = 5;
    
    /* Copy markdown content, limiting to output_size */
    off += snprintf(output + off, output_size - off, "Search results:\n\n");

    /* Parse markdown links: [title](url) */
    const char *ptr = markdown;
    while (*ptr && off < output_size - 1 && link_count < max_links) {
        if (ptr[0] == '[') {
            const char *title_start = ptr + 1;
            const char *title_end = strstr(title_start, "](");
            if (title_end) {
                const char *url_start = title_end + 2;
                const char *url_end = strstr(url_start, ")");
                if (url_end && url_end - url_start < 512) {
                    /* Extract title */
                    size_t title_len = title_end - title_start;
                    if (title_len > 100) title_len = 100;
                    
                    /* Extract URL */
                    size_t url_len = url_end - url_start;
                    if (url_len > 200) url_len = 200;

                    off += snprintf(output + off, output_size - off,
                        "%d. %.*s\n   %.*s\n\n",
                        link_count + 1,
                        (int)title_len, title_start,
                        (int)url_len, url_start);
                    
                    link_count++;
                    ptr = url_end + 1;
                    continue;
                }
            }
        }
        ptr++;
    }

    if (link_count == 0) {
        /* If no links found, just copy first part of content */
        size_t copy_len = strlen(markdown);
        if (copy_len > output_size - 50) copy_len = output_size - 50;
        off += snprintf(output + off, output_size - off, "%.*s", (int)copy_len, markdown);
    }
}

/* ── Direct HTTPS request ─────────────────────────────────────── */

static esp_err_t search_direct(const char *url, search_buf_t *sb)
{
    esp_http_client_config_t config = {
        .url = url,
        .event_handler = http_event_handler,
        .user_data = sb,
        .timeout_ms = 15000,
        .buffer_size = 4096,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) return ESP_FAIL;

    esp_http_client_set_header(client, "Accept", "text/markdown");
    char auth_header[280];
    snprintf(auth_header, sizeof(auth_header), "Bearer %s", s_jina_key);
    esp_http_client_set_header(client, "Authorization", auth_header);

    esp_err_t err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK) return err;
    if (status != 200) {
        ESP_LOGE(TAG, "Search API returned %d", status);
        return ESP_FAIL;
    }
    return ESP_OK;
}

/* ── Proxy HTTPS request ──────────────────────────────────────── */

static esp_err_t search_via_proxy(const char *path, search_buf_t *sb)
{
    proxy_conn_t *conn = proxy_conn_open("s.jina.ai", 443, 15000);
    if (!conn) return ESP_ERR_HTTP_CONNECT;

    char header[768];
    int hlen = snprintf(header, sizeof(header),
        "GET %s HTTP/1.1\r\n"
        "Host: s.jina.ai\r\n"
        "Accept: text/markdown\r\n"
        "Authorization: Bearer %s\r\n"
        "Connection: close\r\n\r\n",
        path, s_jina_key);

    if (proxy_conn_write(conn, header, hlen) < 0) {
        proxy_conn_close(conn);
        return ESP_ERR_HTTP_WRITE_DATA;
    }

    /* Read full response */
    char tmp[4096];
    size_t total = 0;
    while (1) {
        int n = proxy_conn_read(conn, tmp, sizeof(tmp), 15000);
        if (n <= 0) break;
        size_t copy = (total + n < sb->cap - 1) ? (size_t)n : sb->cap - 1 - total;
        if (copy > 0) {
            memcpy(sb->data + total, tmp, copy);
            total += copy;
        }
    }
    sb->data[total] = '\0';
    sb->len = total;
    proxy_conn_close(conn);

    /* Check status */
    int status = 0;
    if (total > 5 && strncmp(sb->data, "HTTP/", 5) == 0) {
        const char *sp = strchr(sb->data, ' ');
        if (sp) status = atoi(sp + 1);
    }

    /* Strip headers */
    char *body = strstr(sb->data, "\r\n\r\n");
    if (body) {
        body += 4;
        size_t blen = total - (body - sb->data);
        memmove(sb->data, body, blen);
        sb->len = blen;
        sb->data[sb->len] = '\0';
    }

    if (status != 200) {
        ESP_LOGE(TAG, "Search API returned %d via proxy", status);
        return ESP_FAIL;
    }
    return ESP_OK;
}

/* ── Execute ──────────────────────────────────────────────────── */

esp_err_t tool_web_search_execute(const char *input_json, char *output, size_t output_size)
{
    if (s_jina_key[0] == '\0') {
        snprintf(output, output_size, "Error: No search API key configured. Set MIMI_SECRET_JINA_KEY in mimi_secrets.h");
        return ESP_ERR_INVALID_STATE;
    }

    /* Parse input to get query */
    cJSON *input = cJSON_Parse(input_json);
    if (!input) {
        snprintf(output, output_size, "Error: Invalid input JSON");
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *query = cJSON_GetObjectItem(input, "query");
    if (!query || !cJSON_IsString(query) || query->valuestring[0] == '\0') {
        cJSON_Delete(input);
        snprintf(output, output_size, "Error: Missing 'query' field");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Jina searching: %s", query->valuestring);

    /* Build URL */
    char encoded_query[256];
    url_encode(query->valuestring, encoded_query, sizeof(encoded_query));
    cJSON_Delete(input);

    char path[384];
    snprintf(path, sizeof(path), "/?q=%s", encoded_query);

    /* Allocate response buffer from PSRAM */
    search_buf_t sb = {0};
    sb.data = heap_caps_calloc(1, SEARCH_BUF_SIZE, MALLOC_CAP_SPIRAM);
    if (!sb.data) {
        snprintf(output, output_size, "Error: Out of memory");
        return ESP_ERR_NO_MEM;
    }
    sb.cap = SEARCH_BUF_SIZE;

    /* Make HTTP request */
    esp_err_t err;
    if (http_proxy_is_enabled()) {
        err = search_via_proxy(path, &sb);
    } else {
        char url[512];
        snprintf(url, sizeof(url), "https://s.jina.ai%s", path);
        err = search_direct(url, &sb);
    }

    if (err != ESP_OK) {
        free(sb.data);
        snprintf(output, output_size, "Error: Search request failed");
        return err;
    }

    /* Format markdown results */
    format_results(sb.data, output, output_size);
    free(sb.data);

    ESP_LOGI(TAG, "Search complete, %d bytes result", (int)strlen(output));
    return ESP_OK;
}

esp_err_t tool_web_search_set_key(const char *api_key)
{
    nvs_handle_t nvs;
    ESP_ERROR_CHECK(nvs_open(MIMI_NVS_SEARCH, NVS_READWRITE, &nvs));
    ESP_ERROR_CHECK(nvs_set_str(nvs, MIMI_NVS_KEY_API_KEY, api_key));
    ESP_ERROR_CHECK(nvs_commit(nvs));
    nvs_close(nvs);

    strncpy(s_jina_key, api_key, sizeof(s_jina_key) - 1);
    ESP_LOGI(TAG, "Search API key saved");
    return ESP_OK;
}

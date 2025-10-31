#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_http_server.h"
#include "esp_system.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <ctype.h>

#include "setup_wifi.h"
/* ESP_ERROR_CHECK stản dấu kiểm tra lỗi của các hàm ESP-IDF.
Trả về mã lỗi nếu hàm trả về khác ESP_OK (0) và in thông báo lỗi.
*/
// ---------- Globals and status ----------
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
bool s_connected = false;

static const char *TAG = "ESP_WEB";
#define WIFI_MAXIMUM_RETRY 5
static int s_retry_num = 0;

// ---------- HTML giao diện cho cấu hình Wi-Fi (mobile friendly) ----------
static const char *html_page =
"<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
"<meta charset='UTF-8'><title>ESP32 WiFi Setup</title>"
"<style>body{font-family:Arial,Helvetica,sans-serif;margin:0;padding:12px;background:#f6f6f6}"
"h2{margin:8px 0} .net{padding:10px;margin:6px 0;background:#fff;border-radius:6px;display:flex;justify-content:space-between;align-items:center}"
"button{padding:8px 12px;border:none;background:#007AFF;color:#fff;border-radius:6px} input{padding:8px;width:100%;box-sizing:border-box;margin-top:6px} #status{margin-top:10px}</style>"
"</head><body>"
"<h2>ESP32 Wi-Fi Setup</h2>"
"<div id=wifiList>Loading networks...</div>"
"<div style='margin-top:10px'>Selected: <span id=sel>None</span></div>"
"<div id=form style='margin-top:10px;display:none'>"
"<input id=pwd type=password placeholder='Password' />"
"<div style='margin-top:8px'><button id=connect>Connect</button></div>"
"</div>"
"<div id=status></div>"
"<script>\n"
"async function scan(){\n"
"  document.getElementById('wifiList').innerText='Scanning...';\n"
"  try{\n"
"    let r=await fetch('/scan');\n"
"    let j=await r.json();\n" 
"    let html='';\n"
"    if(j.length==0) html='<div>No networks found</div>';\n"
"    for(let i=0;i<j.length;i++){\n"
"      let s=j[i].ssid;\n"
"      html+='<div class=\"net\"><span>'+s+'</span><button onclick=\"select(\\''+encodeURIComponent(s)+'\\')\">Select</button></div>';\n"
"    }\n"
"    document.getElementById('wifiList').innerHTML=html;\n"
"  }catch(e){document.getElementById('wifiList').innerText='Scan failed';}\n"
"}\n"
"function select(ssid){\n"
"  document.getElementById('sel').innerText=decodeURIComponent(ssid);\n"
"  document.getElementById('form').style.display='block';\n"
"  window._selected=decodeURIComponent(ssid);\n"
"}\n"
"document.getElementById('connect').addEventListener('click', async ()=>{\n"
"  let ssid=window._selected; let pwd=document.getElementById('pwd').value;\n"
"  document.getElementById('status').innerText='Connecting...';\n"
"  let body='ssid='+encodeURIComponent(ssid)+'&pass='+encodeURIComponent(pwd);\n"
"  try{\n"
"    let r=await fetch('/connect',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded;charset=UTF-8'},body});\n"
"    let t=await r.text();\n"
"    document.getElementById('status').innerText=t;\n"
"    // poll status\n"
"    for(let i=0;i<10;i++){\n"
"      await new Promise(r=>setTimeout(r,1000));\n"
"      let s=await (await fetch('/status')).text();\n"
"      if(s.indexOf('connected')>=0){document.getElementById('status').innerText='Connected!';break}\n"
"      document.getElementById('status').innerText='Trying... ('+(i+1)+')';\n"
"    }\n"
"  }catch(e){document.getElementById('status').innerText='Connect request failed';}\n"
"});\n"
"window.onload=scan;\n"
"</script></body></html>";


// ---------- Handler trang chính ----------
static esp_err_t root_get_handler(httpd_req_t *req) 
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html_page, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// ---------- Handler xử lý khi nhấn nút (giữ để tương thích cũ) ----------
static esp_err_t action_handler(httpd_req_t *req)
{
    char buf[64];
    if (httpd_req_get_url_query_str(req, buf, sizeof(buf)) == ESP_OK) {
        char cmd[16];
        if (httpd_query_key_value(buf, "cmd", cmd, sizeof(cmd)) == ESP_OK) {
            if (strcmp(cmd, "on") == 0) {
                ESP_LOGI(TAG, "=>Button ON pressed!");
            } else if (strcmp(cmd, "off") == 0) {
                ESP_LOGI(TAG, "=>Button OFF pressed!");
            } else {
                ESP_LOGW(TAG, "Unknown command: %s", cmd);
            }
        }
    }
    httpd_resp_sendstr(req, "OK");
    return ESP_OK;
}

// ---------- WiFi scan handler (returns JSON array of SSIDs) ----------
static esp_err_t scan_handler(httpd_req_t *req)
{
    wifi_scan_config_t scan_config = {
        .ssid = 0, // 0: quét tất cả SSID, không chỉ định SSID cụ thể
        .bssid = 0, // 0: Bỏ qua không lọc theo BSSID 
        .channel = 0, // 0: quét tất cả các kênh
        .show_hidden = false // false: không hiển thị mạng ẩn
    };
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true)); // blocking
    uint16_t ap_num = 20;  // tối đa 20 AP (access point - điểm truy cập)
    wifi_ap_record_t ap_info[20]; 
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_num, ap_info));

    // build simple JSON
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr_chunk(req, "[");
    for (int i = 0; i < ap_num; i++) {
        char esc_ssid[33];
        strncpy(esc_ssid, (char *)ap_info[i].ssid, sizeof(esc_ssid)-1);
        esc_ssid[32]=0;
        char buf[128];
        snprintf(buf, sizeof(buf), "{\"ssid\":\"%s\"}", esc_ssid);
        httpd_resp_sendstr_chunk(req, buf);
        if (i < ap_num - 1) httpd_resp_sendstr_chunk(req, ",");
    }
    httpd_resp_sendstr_chunk(req, "]");
    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}

// ---------- Event handlers ----------
// static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
// {
//     if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
//         ESP_LOGI(TAG, "WiFi disconnected");
//         s_connected = false;
//         xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

//         esp_wifi_connect();
//         ESP_LOGI(TAG, "Reconnecting to WiFi...");
        
//         static int retry_count = 0;
//         if(retry_count < 5) {
//             retry_count++;
//         } else {
//             ESP_LOGI(TAG, "Max retries reached - Re-enabling AP interface for configuration");
//             ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
//             retry_count = 0;
//         }
//     }
// }
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) // arg: Dữ liệu phụ truyền vào khi đk handle, 
//event_base: Nhóm sự kiện (WIFI_EVENT,MQTT_EVENT,...)
// event_id: id cụ thể của sự kiện(WIFI_EVENT_STA_START, WIFI_EVENT_STA_DISCONNECTED,...),
// event_data: dữ liệu liên quan đến sự kiện
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "WiFi disconnected");
        s_connected = false;
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        
        if (s_retry_num < WIFI_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retry to connect to the AP, attempt %d/%d", s_retry_num, WIFI_MAXIMUM_RETRY);
        } else {
            ESP_LOGI(TAG, "Failed to connect after %d attempts. Enabling AP mode for reconfiguration", WIFI_MAXIMUM_RETRY);
            // Bật lại chế độ AP
            wifi_config_t wifi_config = {
                .ap = {
                    .ssid = "ESP32_TEST",
                    .ssid_len = 0,
                    .channel = 1,
                    .password = "12345678",
                    .max_connection = 4,
                    .authmode = WIFI_AUTH_WPA_WPA2_PSK
                },
            };
            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
            ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
            ESP_LOGI(TAG, "AP mode re-enabled. SSID: ESP32_TEST, Password: 12345678");
        }
    }
}

static void ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) // internet protocol (IP)
{
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI(TAG, "Got IP - Disabling AP interface");
        s_connected = true;
        s_retry_num = 0; 
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

        // Disable access point mode after successful station connection
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_LOGI(TAG, "AP interface disabled - Device now in pure Station mode");
    }
}

// ---------- Save credentials to NVS ----------
static esp_err_t save_credentials(const char* ssid, const char* pass) // lưu SSID/PASS vào NVS
{
    nvs_handle_t h;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &h);
    if (err != ESP_OK) return err;
    nvs_set_str(h, "ssid", ssid);
    nvs_set_str(h, "pass", pass);
    nvs_commit(h);
    nvs_close(h);
    return ESP_OK;
}

// ---------- Try to connect using provided SSID/PASS ----------
static void wifi_connect_sta(const char* ssid, const char* pass) // thử kết nối WiFi ở chế độ STA  (station: truy cập vào wifi của người dùng)
{
    wifi_config_t sta_conf = {0};
    strncpy((char*)sta_conf.sta.ssid, ssid, sizeof(sta_conf.sta.ssid)-1);
    strncpy((char*)sta_conf.sta.password, pass, sizeof(sta_conf.sta.password)-1);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_conf));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_err_t err = esp_wifi_connect();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "esp_wifi_connect failed: %d", err);
    } else {
        ESP_LOGI(TAG, "Connecting to SSID: %s", ssid);
    }
}
static void url_decode(char *dst, const char *src) {
    char a, b;
    while (*src) {
        if (*src == '%') {
            if ((a = src[1]) && (b = src[2]) && isxdigit(a) && isxdigit(b)) {
                a = tolower(a);
                b = tolower(b);
                a = (a >= 'a') ? a - 'a' + 10 : a - '0';
                b = (b >= 'a') ? b - 'a' + 10 : b - '0';
                *dst++ = (char)(16 * a + b);
                src += 3;
            } else {
                *dst++ = *src++;
            }
        } else if (*src == '+') {
            *dst++ = ' ';  // '+' → ' '
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}
// ---------- /connect handler (POST) ----------
static esp_err_t connect_handler(httpd_req_t *req) 
{
    int ret, remaining = req->content_len;
    char buf[128];
    if (remaining <= 0 || remaining >= sizeof(buf)) {
        httpd_resp_sendstr(req, "Invalid request");
        return ESP_FAIL;
    }
    ret = httpd_req_recv(req, buf, remaining);
    if (ret <= 0) {
        httpd_resp_sendstr(req, "Failed to read body");
        return ESP_FAIL;
    }
    buf[ret]=0;
    char ssid[64] = {0}, pass[65] = {0};
    httpd_query_key_value(buf, "ssid", ssid, sizeof(ssid));
    httpd_query_key_value(buf, "pass", pass, sizeof(pass));
    if (strlen(ssid) == 0) {
        httpd_resp_sendstr(req, "Missing SSID");
        return ESP_OK;
    }
    ESP_LOGI(TAG, "Raw SSID: %s, PASS: %s", ssid, pass);
    url_decode(ssid, ssid);
    url_decode(pass, pass);
    ESP_LOGI(TAG, "Decoded SSID: %s, PASS: %s", ssid, pass);
    save_credentials(ssid, pass);
    wifi_connect_sta(ssid, pass);
    httpd_resp_sendstr(req, "Connecting (saved)");
    return ESP_OK;
}

// ---------- /status handler ----------
static esp_err_t status_handler(httpd_req_t *req)
{
    if (s_connected) httpd_resp_sendstr(req, "connected");
    else httpd_resp_sendstr(req, "disconnected");
    return ESP_OK;
}

// ---------- Khởi động web server ----------
static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t root = {
            .uri = "/", .method = HTTP_GET, .handler = root_get_handler
        };
        httpd_uri_t action = {
            .uri = "/action", .method = HTTP_GET, .handler = action_handler
        };
        httpd_uri_t scan = {
            .uri = "/scan", .method = HTTP_GET, .handler = scan_handler
        };
        httpd_uri_t connect = {
            .uri = "/connect", .method = HTTP_POST, .handler = connect_handler
        };
        httpd_uri_t status = {
            .uri = "/status", .method = HTTP_GET, .handler = status_handler
        };
        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &action);
        httpd_register_uri_handler(server, &scan);
        httpd_register_uri_handler(server, &connect);
        httpd_register_uri_handler(server, &status);
    }
    return server;
}

// ---------- Cấu hình WiFi ở chế độ Access Point (APSTA) ----------
// AP mode used for portal; STA will be configured on demand
static void wifi_init_softap(void)
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "ESP32_TEST",
            .ssid_len = 0,
            .channel = 1,
            .password = "12345678",
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, " WiFi AP started. SSID: ESP32_TEST  PASS: 12345678");
}

// ---------- Try to load saved credentials and connect on boot ----------
static void try_connect_saved() // thử kết nối với các wifi đã lưu khi khởi động 
{
    nvs_handle_t h;
    if (nvs_open("storage", NVS_READONLY, &h) == ESP_OK) {
        size_t required = 0;
        char ssid[33] = {0}, pass[65] = {0};
        if (nvs_get_str(h, "ssid", NULL, &required) == ESP_OK && required < sizeof(ssid)) {
            nvs_get_str(h, "ssid", ssid, &required);
            required = sizeof(pass);
            if (nvs_get_str(h, "pass", NULL, &required) == ESP_OK && required < sizeof(pass)) {
                nvs_get_str(h, "pass", pass, &required);
            }
            ESP_LOGI(TAG, "Found saved SSID: %s", ssid);
            if (strlen(ssid)>0) wifi_connect_sta(ssid, pass);
        }
        nvs_close(h);
    }
}


void setup_wifi_init(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init()); // Khởi tạo mạng ESP
    ESP_ERROR_CHECK(esp_event_loop_create_default()); // Nhận respond từ các sự kiện (wifi, mqtt,...)

    s_wifi_event_group = xEventGroupCreate();
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL);

    esp_netif_create_default_wifi_ap();
    esp_netif_create_default_wifi_sta();
    wifi_init_softap();  // Khởi tạo AP (với chế độ APSTA) chế độ ap (access point - điểm truy cập) dùng để cấu hình 
    try_connect_saved(); // nếu có credentials đã lưu, thử kết nối
    start_webserver();   // Mở web server

    ESP_LOGI(TAG, " Web server started! Connect to WiFi AP 'ESP32_TEST' and open http://192.168.4.1/");
 }
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
#include "gpio_cf.h"
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

// JavaScript từ file evsafe-onboarding.html được nối thành string để embed vào ESP32

// JavaScript từ file evsafe-onboarding.html được nối thành C string để embed vào ESP32

const char* html_page = 
"let wifiNetworks = [];\n"
"let selectedWifi = null;\n"
"let lastJsonResponse = null;\n"
"\n"
"function showJsonResponse() {\n"
"  if (lastJsonResponse) {\n"
"    document.getElementById('jsonContent').textContent = JSON.stringify(lastJsonResponse, null, 2);\n"
"  } else {\n"
"    document.getElementById('jsonContent').textContent = 'Chưa có dữ liệu JSON. Hãy quét WiFi trước.';\n"
"  }\n"
"  document.getElementById('jsonPopup').style.display = 'flex';\n"
"}\n"
"\n"
"function closeJsonPopup() {\n"
"  document.getElementById('jsonPopup').style.display = 'none';\n"
"}\n"
"\n"
"async function scanWifiNetworks() {\n"
"  const wifiList = document.getElementById('wifiList');\n"
"  wifiList.innerHTML = '<div style=\"text-align: center; padding: 20px; color: #888;\">Đang quét WiFi...</div>';\n"
"  try {\n"
"    const response = await fetch('/scan');\n"
"    const networks = await response.json();\n"
"    lastJsonResponse = {\n"
"      timestamp: new Date().toISOString(),\n"
"      url: '/scan',\n"
"      status: response.status,\n"
"      data: networks\n"
"    };\n"
"    if (networks.length === 0) {\n"
"      wifiList.innerHTML = '<div style=\"text-align: center; padding: 20px; color: #888;\">Không tìm thấy mạng WiFi nào</div>';\n"
"      return;\n"
"    }\n"
"    wifiNetworks = networks;\n"
"    generateWifiList();\n"
"  } catch (error) {\n"
"    console.error('Scan failed:', error);\n"
"    wifiList.innerHTML = '<div style=\"text-align: center; padding: 20px; color: #ff6b6b;\">Quét WiFi thất bại. Đang sử dụng dữ liệu mẫu.</div>';\n"
"    setTimeout(() => { generateWifiList(); }, 2000);\n"
"  }\n"
"}\n"
"\n"
"function getSignalBars(strength) {\n"
"  const bars = ['📶', '📶📶', '📶📶📶', '📶📶📶📶', '📶📶📶📶📶'];\n"
"  return bars[strength - 1] || '📶';\n"
"}\n"
"\n"
"function generateWifiList() {\n"
"  const wifiList = document.getElementById('wifiList');\n"
"  wifiList.innerHTML = '';\n"
"  wifiNetworks.forEach((network, index) => {\n"
"    const wifiItem = document.createElement('div');\n"
"    wifiItem.className = 'wifi-item';\n"
"    wifiItem.onclick = () => openWifiPopup(network);\n"
"    const lockIcon = network.hasPassword ? '🔒' : '';\n"
"    const signalBars = getSignalBars(network.signalStrength);\n"
"    wifiItem.innerHTML = `<p class=\"wifi-name\">${network.ssid || network.name}</p><div class=\"wifi-icons\"><span style=\"margin-right: 4px;\">${lockIcon}</span><span class=\"wifi-icon\" style=\"font-size: 12px;\">${signalBars}</span></div>`;\n"
"    wifiList.appendChild(wifiItem);\n"
"  });\n"
"}\n"
"\n"
"function openWifiPopup(network) {\n"
"  selectedWifi = network;\n"
"  const networkName = network.ssid || network.name;\n"
"  document.getElementById('popupTitle').textContent = networkName;\n"
"  if (network.hasPassword) {\n"
"    document.getElementById('popupSubtitle').textContent = 'Nhập mật khẩu để kết nối';\n"
"    document.getElementById('wifiPassword').style.display = 'block';\n"
"    document.getElementById('wifiPassword').focus();\n"
"  } else {\n"
"    document.getElementById('popupSubtitle').textContent = 'Mạng WiFi không có mật khẩu';\n"
"    document.getElementById('wifiPassword').style.display = 'none';\n"
"  }\n"
"  document.getElementById('wifiPopup').style.display = 'flex';\n"
"}\n"
"\n"
"function closeWifiPopup() {\n"
"  document.getElementById('wifiPopup').style.display = 'none';\n"
"  document.getElementById('wifiPassword').value = '';\n"
"  selectedWifi = null;\n"
"}\n"
"\n"
"function connectWifi() {\n"
"  if (selectedWifi) {\n"
"    const password = document.getElementById('wifiPassword').value;\n"
"    const networkName = selectedWifi.ssid || selectedWifi.name;\n"
"    if (selectedWifi.hasPassword && !password) {\n"
"      alert('Vui lòng nhập mật khẩu WiFi');\n"
"      return;\n"
"    }\n"
"    alert(`Đang kết nối tới ${networkName}...`);\n"
"    closeWifiPopup();\n"
"    console.log('Connecting to:', networkName, 'with password:', password);\n"
"    console.log('Network info:', selectedWifi);\n"
"  }\n"
"}\n"
"\n"
"document.getElementById('wifiPopup').addEventListener('click', function(e) {\n"
"  if (e.target === this) { closeWifiPopup(); }\n"
"});\n"
"\n"
"document.getElementById('jsonPopup').addEventListener('click', function(e) {\n"
"  if (e.target === this) { closeJsonPopup(); }\n"
"});\n"
"\n"
"document.getElementById('wifiPassword').addEventListener('keypress', function(e) {\n"
"  if (e.key === 'Enter') { connectWifi(); }\n"
"});\n"
"\n"
"document.addEventListener('DOMContentLoaded', function() {\n"
"  scanWifiNetworks();\n"
"});\n";


// ---------- Handler trang chính ----------
static esp_err_t root_get_handler(httpd_req_t *req) 
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
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
// static esp_err_t scan_handler(httpd_req_t *req)
// {
//     wifi_scan_config_t scan_config = {
//         .ssid = 0, // 0: quét tất cả SSID, không chỉ định SSID cụ thể
//         .bssid = 0, // 0: Bỏ qua không lọc theo BSSID 
//         .channel = 0, // 0: quét tất cả các kênh
//         .show_hidden = false // false: không hiển thị mạng ẩn
//     };
//     ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true)); // blocking
//     uint16_t ap_num = 20;  // tối đa 20 AP (access point - điểm truy cập)
//     wifi_ap_record_t ap_info[20]; 
//     ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_num, ap_info));

//     // build simple JSON
//     httpd_resp_set_type(req, "application/json");
//     httpd_resp_sendstr_chunk(req, "[");
//     for (int i = 0; i < ap_num; i++) {
//         char esc_ssid[33];
//         strncpy(esc_ssid, (char *)ap_info[i].ssid, sizeof(esc_ssid)-1);
//         esc_ssid[32]=0;
//         char buf[128];
//         snprintf(buf, sizeof(buf), "{\"ssid\":\"%s\"}", esc_ssid);
//         httpd_resp_sendstr_chunk(req, buf);
//         if (i < ap_num - 1) httpd_resp_sendstr_chunk(req, ",");
//     }
//     httpd_resp_sendstr_chunk(req, "]");
//     httpd_resp_sendstr_chunk(req, NULL);
//     return ESP_OK;
// }
static esp_err_t scan_handler(httpd_req_t *req)
{
    wifi_scan_config_t scan_config = {
        .ssid = 0,
        .bssid = 0, 
        .channel = 0,
        .show_hidden = false
    };
    
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));
    uint16_t ap_num = 20;
    wifi_ap_record_t ap_info[20];
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_num, ap_info));

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr_chunk(req, "[");
    
    for (int i = 0; i < ap_num; i++) {
        char esc_ssid[33];
        strncpy(esc_ssid, (char*)ap_info[i].ssid, sizeof(esc_ssid)-1);
        esc_ssid[32] = 0;
        
        // Tính signal strength (1-5 bars)
        int signal_strength = 1;
        if (ap_info[i].rssi >= -50) signal_strength = 5;      // Excellent
        else if (ap_info[i].rssi >= -60) signal_strength = 4; // Good  
        else if (ap_info[i].rssi >= -70) signal_strength = 3; // Fair
        else if (ap_info[i].rssi >= -80) signal_strength = 2; // Poor
        else signal_strength = 1;                             // Very poor
        
        // Kiểm tra có cần password không
        bool has_password = (ap_info[i].authmode != WIFI_AUTH_OPEN);
        
        char buf[200];
        snprintf(buf, sizeof(buf), 
            "{\"ssid\":\"%s\",\"signalStrength\":%d,\"hasPassword\":%s,\"rssi\":%d}", 
            esc_ssid, 
            signal_strength,
            has_password ? "true" : "false",
            ap_info[i].rssi
        );
        
        httpd_resp_sendstr_chunk(req, buf);
        if (i < ap_num - 1) httpd_resp_sendstr_chunk(req, ",");
    }
    
    httpd_resp_sendstr_chunk(req, "]");
    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}

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
                    .ssid = "CHTLAB-EVsafe",
                    .ssid_len = 0,
                    .channel = 1,
                    .password = "",
                    .max_connection = 4,
                    .authmode = WIFI_AUTH_OPEN
                },
            };
            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
            ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
            ESP_LOGI(TAG, "AP mode re-enabled. SSID: CHTLAB-EVsafe");
        }
    }
}
void open_webserver()
{
    ESP_LOGI(TAG, "Enabling AP mode for reconfiguration");
    // Bật lại chế độ AP
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "CHTLAB-EVsafe",
            .ssid_len = 0,
            .channel = 1,
            .password = "",
            .max_connection = 4,
            .authmode = WIFI_AUTH_OPEN
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_LOGI(TAG, "AP mode re-enabled. SSID: CHTLAB-EVsafe");
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
//    ESP_LOGI(TAG, "Raw SSID: %s, PASS: %s", ssid, pass);
    url_decode(ssid, ssid);
    url_decode(pass, pass);
    // ESP_LOGI(TAG, "Decoded SSID: %s, PASS: %s", ssid, pass);
    save_credentials(ssid, pass);
    wifi_connect_sta(ssid, pass);
    httpd_resp_sendstr(req, "Connecting (saved)");
    return ESP_OK;
}

// ---------- /status handler ----------
static esp_err_t status_handler(httpd_req_t *req)
{
    if (s_connected){
        gpio_set_level(LED_DECTEC_MQTT,1);
        httpd_resp_sendstr(req, "connected");
    }
    else {
        gpio_set_level(LED_DECTEC_MQTT,0);
        httpd_resp_sendstr(req, "disconnected");
    }
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
            .ssid = "CHTLAB-EVsafe",
            .ssid_len = 0,
            .channel = 1,
            .password = "",
            .max_connection = 4,
            .authmode = WIFI_AUTH_OPEN,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, " WiFi AP started. SSID: CHTLAB-EVsafe");
}

// ---------- Try to load saved credentials and connect on boot ----------
void try_connect_saved() // thử kết nối với các wifi đã lưu khi khởi động 
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
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
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
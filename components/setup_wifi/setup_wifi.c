#include <string.h>
#include <stdio.h>
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
#include "esp_http_client.h"

#include "setup_wifi.h"
#include "system_manage.h"
#include "mqtt_wifi.h"
#include "gpio_cf.h"
// extern variable
int wifi_state = 0; //0 disconnect, 1 connect, 2 setup
bool act_handle = false;
/* ESP_ERROR_CHECK stản dấu kiểm tra lỗi của các hàm ESP-IDF.
Trả về mã lỗi nếu hàm trả về khác ESP_OK (0) và in thông báo lỗi.
*/
// ---------- Globals and status ----------
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
bool s_connected = false;
static char buffer[128];
static const char *TAG = "ESP_WEB";
#define WIFI_MAXIMUM_RETRY 2
static int s_retry_num = 0;

// ---------- Handler trang chính ----------
static esp_err_t root_get_handler(httpd_req_t *req) 
{
    act_handle = true;
    printf("access to interface\r\n");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html_page_2, HTTPD_RESP_USE_STRLEN);
    vTaskDelay(500/portTICK_PERIOD_MS);
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
// }
static esp_err_t scan_handler(httpd_req_t *req)
{
    act_handle = true;
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

void publish_infor_wifi(void){
    wifi_ap_record_t ap_info;
    esp_err_t ret = esp_wifi_sta_get_ap_info(&ap_info);
    char ssid[33], MAC[33], ip[33];
    int rssid;
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Connected SSID: %s", (char*)ap_info.ssid);
        ESP_LOGI(TAG, "RSSI: %d dBm", ap_info.rssi);
        snprintf(ssid,sizeof(ssid),"%s",(char *)ap_info.ssid);
        rssid = ap_info.rssi;
    } else {
        ESP_LOGW(TAG, "Not connected or can't get AP info (err=%d)", ret);
        return;
    }
   
    uint8_t mac[6];
    if (esp_wifi_get_mac(WIFI_IF_STA, mac) == ESP_OK) {
        ESP_LOGI(TAG, "STA MAC: %02x:%02x:%02x:%02x:%02x:%02x",
                 mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
        snprintf(MAC,sizeof(MAC),"%02x:%02x:%02x:%02x:%02x:%02x",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    }
    else return;
     esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif) {
        esp_netif_ip_info_t ip_info;
        if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
            ESP_LOGI(TAG, "IP: " IPSTR, IP2STR(&ip_info.ip));
            snprintf(ip, sizeof(ip), IPSTR, IP2STR(&ip_info.ip));

        } else {
            ESP_LOGW(TAG, "No IP info yet");
            return;
        }
    } else {
        ESP_LOGW(TAG, "Can't get netif handle (WIFI_STA_DEF)");
        return;
    }
    mqtt_publish_wifi_infor(ssid,MAC,ip,rssid);   
}
void exit_accesspoint(){
    act_handle = false;
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_LOGI(TAG, "AP interface disabled - Device now in pure Station mode");
}
void reopen_network(){
    snprintf(buffer,128,"Evsafe_%s",device_name);
    wifi_config_t wifi_config = {
        .ap = {
            // .ssid = buffer,
            .ssid_len = 0,
            .channel = 1,
            .password ="",
            .max_connection = 4,
            .authmode = WIFI_AUTH_OPEN
        },
    };
    wifi_state=2;
    strncpy((char *)wifi_config.ap.ssid, buffer, sizeof(wifi_config.ap.ssid) - 1);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_LOGI(TAG, "AP mode re-enabled. SSID: Evsafe_%s",device_name);
}
// static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) // arg: Dữ liệu phụ truyền vào khi đk handle, 
// //event_base: Nhóm sự kiện (WIFI_EVENT,MQTT_EVENT,...)
// // event_id: id cụ thể của sự kiện(WIFI_EVENT_STA_START, WIFI_EVENT_STA_DISCONNECTED,...),
// // event_data: dữ liệu liên quan đến sự kiện
// {
//     if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
//         esp_wifi_connect();
//     } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
//         ESP_LOGI(TAG, "WiFi disconnected");
//         s_connected = false;
//         wifi_state=0;
//         xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        
//        // if (s_retry_num < WIFI_MAXIMUM_RETRY) {
//         //    esp_wifi_connect();
//        //     s_retry_num++;
//        //     ESP_LOGI(TAG, "Retry to connect to the AP, attempt %d/%d", s_retry_num, WIFI_MAXIMUM_RETRY);
//        // } else {
//        //     ESP_LOGI(TAG, "Failed to connect after %d attempts. Enabling AP mode for reconfiguration", WIFI_MAXIMUM_RETRY);
//             //Bật lại chế độ AP
//             wifi_mode_t mode;
//             esp_wifi_get_mode(&mode);

//             if (mode == WIFI_MODE_AP) {
//                 ESP_LOGI("WIFI", "Đang ở chế độ Access Point");
//                 wifi_state=2;
//             } else if (mode == WIFI_MODE_STA) {
//                 ESP_LOGI("WIFI", "Đang ở chế độ Station");
//                 reopen_network();
//             } else if (mode == WIFI_MODE_APSTA) {
//                 ESP_LOGI("WIFI", "Đang ở chế độ AP + STA ");
//                 wifi_state=2;
//             } else {
//                 ESP_LOGI("WIFI", "Wi-Fi đang tắt hoặc không xác định");
//             }
//         // }
//     }
// }
static wifi_event_handler(void *arg, esp_event_handler_t event_base,int32_t event_id, void * event_data){
    if(event_base == WIFI_EVENT){
        switch (event_id)
        {
        case WIFI_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG, "WiFi disconnected");
            s_connected = false;
            wifi_state=0;
            xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

            wifi_mode_t mode;
            esp_wifi_get_mode(&mode);

            if (mode == WIFI_MODE_AP) {
                ESP_LOGI("WIFI", "Đang ở chế độ Access Point");
                wifi_state=2;
            } else if (mode == WIFI_MODE_STA) {
                ESP_LOGI("WIFI", "Đang ở chế độ Station");
                reopen_network();
            } else if (mode == WIFI_MODE_APSTA) {
                ESP_LOGI("WIFI", "Đang ở chế độ AP + STA ");
                wifi_state=2;
            } else {
                ESP_LOGI("WIFI", "Wi-Fi đang tắt hoặc không xác định");
            }
            break;
        case WIFI_EVENT_AP_STACONNECTED:
            ESP_LOGI(TAG,"User access to AP");
            act_handle=true;
        case WIFI_EVENT_AP_STADISCONNECTED:
            ESP_LOGI(TAG,"Out from setup wifi mode");
            act_handle=false;
        default:
            break;
        }
    }
}
bool check_internet() {
    esp_http_client_config_t config = {
        .url = "http://clients3.google.com/generate_204",
        .timeout_ms = 5000,
        .skip_cert_common_name_check = true,
        .keep_alive_enable = false,
        .transport_type = HTTP_TRANSPORT_OVER_TCP
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return false;
    }

    esp_err_t err = esp_http_client_perform(client);
    bool has_internet = false;
    
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP Status = %d", status);
        if (status == 204) {
            has_internet = true;
        }
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
    
    esp_http_client_cleanup(client);
    return has_internet;
}

static void ip_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) // internet protocol (IP)
{
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI(TAG, "Got IP - Disabling AP interface");
        s_connected = true;
        s_retry_num = 0; 
        act_handle = false;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        stop_my_timer();
        // Disable access point mode after successful station connection
        vTaskDelay(3000/portTICK_PERIOD_MS);
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
    esp_wifi_disconnect();
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
    if (s_connected) {
       // ESP_LOGE(TAG,"send wifi state connect to http");
        httpd_resp_sendstr(req, "connected");
    }
    else {
        //ESP_LOGE(TAG,"send wifi state disconnect to http");
        httpd_resp_sendstr(req, "failed");
    }
    return ESP_OK;
}

// ---------- Khởi động web server ----------
static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192; 

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

    snprintf(buffer,128,"Evsafe_%s",device_name);
    wifi_config_t wifi_config = {
        .ap = {
            // .ssid = buffer,
            .ssid_len = 0,
            .channel = 1,
            .password = "",
            .max_connection = 4,
            .authmode =  WIFI_AUTH_OPEN
        },
    };
    strncpy((char *)wifi_config.ap.ssid, buffer, sizeof(wifi_config.ap.ssid) - 1);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, " WiFi AP started. SSID: Evsafe_%s ",device_name);
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

    ESP_LOGI(TAG, " Web server started! Connect to WiFi AP 'Evsafe_%s' and open http://192.168.4.1/",device_name);
 }
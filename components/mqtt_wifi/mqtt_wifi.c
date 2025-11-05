#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <esp_log.h>
#include <esp_err.h>
#include <mqtt_client.h>

#include "mqtt_wifi.h"
#include "system_manage.h"
#include "config_parameter.h"
#include "encrypt_decrypt.h"
#include "setup_wifi.h"
#include "esp_wifi.h"

// define global variables
static const char *TAG = "MQTT_WIFI";
static char mqtt_client_id[64] = "";
static char *mqtt_address = "mqtt://broker.chtlab.us:1883";
static char *mqtt_username = "username";
static char *mqtt_password = "password";
static char json[1024] = "";
static char buffer[1024] = "";
static char cmd[256];
static bool first_pub_version=false;

static int count=0;
RTC_DATA_ATTR static esp_mqtt_client_handle_t client;


static esp_err_t mqtt_subscribe(esp_mqtt_client_handle_t client_id, const char *topic, int qos)
{
    int msg_id = esp_mqtt_client_subscribe_single(client_id, topic, qos);
    ESP_LOGI(TAG, "Subscribed to topic %s with message ID: %d", topic, msg_id);
    if(msg_id == -1)    return ESP_FAIL;
    else                return ESP_OK;

}
static esp_err_t mqtt_publish(esp_mqtt_client_handle_t client_id, const char *topic, const char *data, int len, int qos, int retain)
{
    int msg_id = esp_mqtt_client_publish(client_id, topic, data, len, qos, retain);
    ESP_LOGI(TAG, "Published message with ID: %d", msg_id);
    if(msg_id == -1)    return ESP_OK;
    else                return ESP_FAIL;
}
void mqtt_publish_data(char *data,char *topic){
    //snprintf(mqtt_client_id, sizeof(mqtt_client_id), "%s_%s",DEVICE_NAME,device_name);
    if(client==NULL && client){
        ESP_LOGE(TAG, "MQTT client is not initialized");
        return;
    }
    else ESP_LOGI(TAG, "MQTT publish");

  //  printf("client: %p\r\n",(char*)client);
    esp_mqtt_client_publish(client, topic, data, strlen(data), 1, 0);
    //ESP_ERROR_CHECK(mqtt_publish(client,topic,data,strlen(data),1,0));
}
static void check_wifi(){
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
}
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    // // client = event->client;
   //  printf("client: %p\r\n",( char * )client);
    // your_context_t *context = handler_args;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        // Subscribe to a topic upon successful connection
        snprintf(cmd, sizeof(cmd), "%s/SmartEVsafe", SUB);
        mqtt_subscribe(client, cmd, 1);
        snprintf(cmd, sizeof(cmd), "%s_%s",DEVICE_NAME,device_name);
        mqtt_subscribe(client, cmd, 1);
        wifi_state=1;
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        count++;
        if(act_handle==false && count >= 5){
            try_connect_saved();
            count=0;
        }
        else ESP_LOGI(TAG,"Setup wifi");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        publish_infor_wifi();
        if(first_pub_version==false)
        {
         mqtt_publish_version(HW_VERSION,FW_VERSION,0);
         first_pub_version=true;
        }
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        memcpy(buffer, event->data, event->data_len);
        buffer[event->data_len] = '\0'; 
        convert_to_json(buffer);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        check_wifi();
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}
void mqtt_init(char *mqtt_address, char *client_id,char *username, char *password)
{
    ESP_LOGI(TAG, "MQTT configuration function called");
    const esp_mqtt_client_config_t mqtt_cfg = {
    .broker.address.uri = mqtt_address,
    .credentials.client_id = client_id,
    .credentials.username = username,
    .credentials.authentication.password = password,
    .session.keepalive = 120,
    };
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}

void mqtt_start(){
    snprintf(mqtt_client_id, sizeof(mqtt_client_id), "%s_%s",DEVICE_NAME,device_name);
    mqtt_init(mqtt_address, mqtt_client_id, mqtt_username, mqtt_password);
}

//---------------prepare data to publish mqtt -----------------
// cmd_type:205_all
void mqtt_publish_data_power(float *value_power,int *value_vol){
    snprintf(json,sizeof(json),"{\n"
        "  \"data\": {\n"
        "    \"gun1\": { \"power\": %f,\"vol\": %d },\n"
        "    \"gun2\": { \"power\": %f,\"vol\": %d },\n"
        "    \"gun3\": { \"power\": %f,\"vol\": %d },\n"
        "    \"gun4\": { \"power\": %f,\"vol\": %d },\n"
        "    \"gun5\": { \"power\": %f,\"vol\": %d },\n"
        "    \"gun6\": { \"power\": %f,\"vol\": %d }\n"
        "  }\n"
        "}",value_power[0],value_vol[0],
            value_power[1],value_vol[1],
            value_power[2],value_vol[2],
            value_power[3],value_vol[3],
            value_power[4],value_vol[4],
            value_power[5],value_vol[5]);
        char *json_encrypted = encrypt_data(json,device_name,205);
    if (json_encrypted==NULL)
    {
        printf("205: Can't encrypted, mqtt publish failed\r\n");
    }
    else {
        //mqtt_pub("UP4G/SmartEVsafe",json_encrypted);
        strcpy(buffer,json_encrypted);
        // xQueueSend(publish_queue_handle,buffer,portMAX_DELAY);
        snprintf(cmd,sizeof(cmd),"%s/SmartEVsafe",PUB);
        mqtt_publish(client,cmd,buffer,strlen(buffer),1,0);
    }
    free(json_encrypted);
}
// cmd_type: 201
void mqtt_publish_version(char *verHW,char *verFW,int status){
    snprintf(json,sizeof(json),"{\n"
        "  \"vesion\": {\n"
        "    \"verFW\": \"%s\", \"verHW\": \"%s\", \"status\":%d\n"
        "  }\n"
        "}",verFW,verHW,status);
    char *json_encrypted = encrypt_data(json,device_name,201);
    if (json_encrypted==NULL){
        printf("201: Can't encrypted, mqtt publish failed\r\n");
    }
    else {
        // if(read_enable){
        strcpy(buffer,json_encrypted);
        //     xQueueSend(publish_queue_handle,buffer,portMAX_DELAY);
        // }
        // else {
        //     snprintf(cmd,256,"%s/SmartEVsafe",PUB);
        //     mqtt_pub(cmd,json_encrypted);
        // }
        // if(first_pub_version){
        // xQueueSend(publish_queue_handle,buffer,portMAX_DELAY);
        // }
        // else
        // {
        snprintf(cmd, sizeof(cmd), "%s/SmartEVsafe", PUB);
        mqtt_publish(client,cmd,buffer,strlen(buffer),1,0);
       // }
    }
    free(json_encrypted);
}
// cmd_type: 202
void mqtt_publish_gpsposition(float latitude_decimal,float longitude_decimal){
    snprintf(json,sizeof(json),"{\n"
        "  \"gps\": {\n"
        "    \"longitude\": %f,\n"
        "    \"latitude\": %f\n"
        "  }\n"
        "}",latitude_decimal,longitude_decimal);
    char *json_encrypted =  encrypt_data(json,device_name,202);
    if(json_encrypted==NULL){
        printf("202: Can't encrypted, mqtt publish failed\r\n");
    }
    else{
        strcpy(buffer,json_encrypted);
        snprintf(cmd, sizeof(cmd), "%s/SmartEVsafe", PUB);
        mqtt_publish(client,cmd,buffer,strlen(buffer),1,0);
    }
    free(json_encrypted);
}
//cmd_type: 203
void mqtt_publish_temp(double temp){
    snprintf(json,sizeof(json),"{\n"
        "  \"data\": {\n"
        "    \"temp\": %f\n"
        "  }\n"
        "}",temp);
    char *json_encrypted = encrypt_data(json,device_name,203);
    if (json_encrypted==NULL)
    {
        printf("203: Can't encrypted, mqtt publish failed\r\n");
    }
    else {
          // mqtt_pub("UP4G/SmartEVsafe",json_encrypted);
        strcpy(buffer,json_encrypted);
        snprintf(cmd,sizeof(cmd),"%s/SmartEVsafe",PUB);
        mqtt_publish(client,cmd,buffer,strlen(buffer),1,0);
    }
    free(json_encrypted);
}
//cmd_type: 204
// void publish_errorcode(int error,int port_num){
//     char *json = make_json_errorcode(error,port_num);
//     mqtt_publish_encrypted(json,device_name,204);
//     free(json);
// }
//cmd_type:205
void mqtt_publish_data_1gun_only(int gun_number,float power,float vol){
    snprintf(json,sizeof(json),"{\n"
    "  \"data\": {\n"
    "    \"gun%d\": {\n"
    "      \"power\": %f,\n"
    "      \"vol\": %f\n"
    "    }\n"
    "  }\n"
    "}",gun_number,power,vol);
    char *json_encrypted = encrypt_data(json,device_name,205);
    if (json_encrypted==NULL)
    {
        printf("205: Can't encrypted, mqtt publish failed\r\n");
    }
    else {
        //   mqtt_pub("UP4G/SmartEVsafe",json_encrypted);
        strcpy(buffer,json_encrypted);
        snprintf(cmd, sizeof(cmd), "%s/SmartEVsafe", PUB);
        mqtt_publish(client,cmd,buffer,strlen(buffer),1,0);
    }
    free(json_encrypted);   
}
//cmd_type: 206
void mqtt_publish_wifi_infor(char *ssid,char *mac,char*ip,int rssi){
    snprintf(json,sizeof(json),"{\n"
    "  \"SSID\": \"%s\",\n"
    "  \"MAC\": \"%s\",\n"
    "  \"IP\": \"%s\",\n"
    "  \"Rssi\": %d\n"
    "}",ssid,mac,ip,rssi);
    char *json_encrypted = encrypt_data(json,device_name,206);
    if (json_encrypted==NULL)
    {
        printf("206: Can't encrypted, mqtt publish failed\r\n");
    }
    else {
        //   mqtt_pub("UP4G/SmartEVsafe",json_encrypted);
        strcpy(buffer,json_encrypted);
        snprintf(cmd, sizeof(cmd), "%s/SmartEVsafe", PUB);
        mqtt_publish(client,cmd,buffer,strlen(buffer),1,0);
    }
    free(json_encrypted);
}
// cmd_type: 207
// 1-idle 2-charging 3-error 4-complete
void mqtt_publish_gun_status(int *value_status)
{
    snprintf(json,sizeof(json),"{\n"
        "  \"data\" : {\n"
        "    \"gun1\" : {\n"
        "      \"status\" : %d\n"
        "    },\n"
        "    \"gun2\" : {\n"
        "      \"status\" : %d\n"
        "    },\n"
        "    \"gun3\" : {\n"
        "      \"status\" : %d\n"
        "    },\n"
        "    \"gun4\" : {\n"
        "      \"status\" : %d\n"
        "    },\n"
        "    \"gun5\" : {\n"
        "      \"status\" : %d\n"
        "    },\n"
        "    \"gun6\" : {\n"
        "      \"status\" : %d\n"
        "    }\n"
        "  }\n"
        "}",value_status[0],value_status[1],value_status[2],
        value_status[3],value_status[4],value_status[5]);
    char *json_encrypted = encrypt_data(json,device_name,207);
    if (json_encrypted==NULL)
    {
        printf("207: Can't encrypted, mqtt publish failed\r\n");
    }
      else {
        //   mqtt_pub("UP4G/SmartEVsafe",json_encrypted);
        strcpy(buffer,json_encrypted);
        snprintf(cmd, sizeof(cmd), "%s/SmartEVsafe", PUB);
        mqtt_publish(client,cmd,buffer,strlen(buffer),1,0);
    }
    free(json_encrypted);
}
void mqtt_respond_change_gate(int gate, int state, int cmd_d) {
    snprintf(json, BUF_SIZE_MQTT,
         "{\n"
         "  \"data\": {\n"
         "    \"V\": %d,\n"
         "    \"S\": %d,\n"
         "    \"Cmd\": %d\n"
         "  }\n"
         "}",
             gate, state, cmd_d);
    char *json_encrypted = encrypt_data(json,device_name,208);
    if (json_encrypted==NULL)
    {
        printf("208: Can't encrypted, mqtt publish failed\r\n");
    }
      else {
        //   mqtt_pub("UP4G/SmartEVsafe",json_encrypted);
        strcpy(buffer,json_encrypted);
        //xQueueSend(publish_queue_handle,buffer,portMAX_DELAY);
        snprintf(cmd,sizeof(cmd),"%s/SmartEVsafe",PUB);
        mqtt_publish(client,cmd,buffer,strlen(buffer),1,0);
    }
    free(json_encrypted);
    //mqtt_publish_encrypted(json, device_name, 208);
}



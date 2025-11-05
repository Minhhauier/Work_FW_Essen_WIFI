#ifndef MQTT_H
#define MQTT_H

#include <mqtt_client.h>
void mqtt_init(char *mqtt_address, char *client_id,char *username, char *password);
// static esp_err_t mqtt_subscribe(esp_mqtt_client_handle_t client, const char *topic, int qos);
// static esp_err_t mqtt_publish(esp_mqtt_client_handle_t client, const char *topic, const char *data, int len, int qos, int retain);
void mqtt_start();
void mqtt_publish_data_power(float *value_power,int *value_vol);
void mqtt_publish_version(char *verHW,char *verFW,int status);
void mqtt_publish_gpsposition(float latitude_decimal,float longitude_decimal);
void mqtt_publish_temp(double temp);
void mqtt_publish_data_1gun_only(int gun_number,float power,float vol);
void mqtt_publish_wifi_infor(char *ssid,char *mac,char*ip,int rssi);
void mqtt_publish_gun_status(int *value_status);
void mqtt_respond_change_gate(int gate, int state, int cmd);
void mqtt_publish_data(char *data,char *topic);
#endif
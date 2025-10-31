#include <string.h>
#include <stdio.h>
#include <config_parameter.h>
#include <encrypt_decrypt.h>

static char JSON[];

char *get_subarray(char *data, char *start, char end) {
    char *st = strstr(data, start);
    if (st != NULL) {
        char *en = strchr((char *)st, end);
        st = st + strlen(start) + 2;
        if (en != NULL) {
            int len = en - st - 1;
            char *DATA = malloc(len + 1);
            memcpy(DATA, st, len);
            DATA[len] = '\0';
            return DATA;
        }
        return NULL;
    } else
        return NULL;
}

void convert_to_json(const char *data) {
    if(data==NULL) return;
    int check = 0;
    char *hex_iv = NULL;
    char *hex_data = NULL;
    char *hex_tag = NULL;
    char *arry_tam = NULL;
    char *mess = NULL;
    char final_data[1024];

//    char *start_1 = strstr((char *)data, "+QMTRECV:");
    char *start = strchr((char *)data, '{');
    if (start == NULL) check = 1;
    // char *end = strstr((char *)data, "+CMQTTRXEND: 0");
    char *end = (char*) data + strlen(data);
    if (end == NULL) check = 1;
    int json_len = end - start;
    if (check == 0) {
        memcpy(JSON, start, json_len);
        JSON[json_len] = '\0';
        hex_iv = get_subarray(JSON, "\"iv\"", ',');
      //  printf("hex:%s\r\n",hex_iv);
        arry_tam = get_subarray(JSON, "\"iv\"", '}');
      //  printf("arr: %s\r\n",arry_tam);
        if (arry_tam == NULL) {
            printf("Đọc không thành công, xem lại định dạng chuỗi truyền\r\n");
            free(hex_iv);
            free(arry_tam);
            return;
        }
        hex_data = get_subarray(arry_tam, "\"data\"", ',');
        //printf("data: %s\r\n",hex_data);
        hex_tag = get_subarray(JSON, "\"tag\"", '}');
       // printf("tag: %s\r\n",hex_tag);
        char *p = (char *)JSON;
        char *end = strstr(p, ":{");
        int len_data = end - p;
        mess = malloc(len_data + 1);
        if (strlen(data) > 0) {
            memcpy(mess, p, len_data);
            mess[len_data] = '\0';
        } else
            printf("Mess is NULL\r\n");
         //if (mess != NULL) printf("mess: %s\r\n", mess);
        if (hex_iv == NULL || hex_data == NULL || hex_tag == NULL || mess == NULL) {
            printf("Đọc không thành công, xem lại định dạng chuỗi truyền\r\n");
            free(hex_iv);
            free(hex_data);
            free(hex_tag);
            free(arry_tam);
            free(mess);
        } else {
            unsigned char *Data_json = decrypt_data(hex_iv, hex_data, hex_tag);
           // printf("rx1: %s\r\n", Data_json);
            snprintf(final_data, sizeof(final_data), "%s:%s}", mess, Data_json);
            printf("Data received: %s\r\n", final_data);
            printf("===\r\n");
            parse_json((const char *)final_data);
            free(hex_iv);
            free(hex_data);
            free(hex_tag);
            free(arry_tam);
            free(Data_json);
            free(mess);
        }
    }
}
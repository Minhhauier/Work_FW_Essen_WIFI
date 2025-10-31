#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "mbedtls/base64.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/gcm.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "encrypt_decrypt.h"

// #include "Global.h"
//  Hàm in hex
void print_hex(const char *label, unsigned char *data, size_t len) {
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02x", data[i]);
    }
    printf("\n");
}
char *convert_int_to_hex(const char *label, unsigned char *data, size_t len) {
    char *hex_data = NULL;
    hex_data = malloc(len * 2 + 1);
    for (size_t i = 0; i < len; i++) {
        sprintf(&hex_data[i * 2], "%02x", data[i]);
    }
    hex_data[len * 2] = '\0';
    // printf("%s: %s\r\n",label,hex_data);
    return hex_data;
}
unsigned char key_encrypt[32] = {
    0x35, 0x70, 0x7A, 0x63, 0x74, 0x68, 0x61, 0x50,
    0x54, 0x34, 0x6C, 0x4C, 0x36, 0x4C, 0x49, 0x66,
    0x38, 0x59, 0x47, 0x32, 0x47, 0x30, 0x79, 0x50,
    0x45, 0x47, 0x77, 0x67, 0x36, 0x70, 0x5A, 0x78};
/*key: hằng số bất biến khi biên dịch sẽ để trong flash/ROM không tốn RAM động
chỉ đọc được không thể sửa được */
const unsigned char key[32] = {
    0x35, 0x70, 0x7A, 0x63, 0x74, 0x68, 0x61, 0x50,
    0x54, 0x34, 0x6C, 0x4C, 0x36, 0x4C, 0x49, 0x66,
    0x38, 0x59, 0x47, 0x32, 0x47, 0x30, 0x79, 0x50,
    0x45, 0x47, 0x77, 0x67, 0x36, 0x70, 0x5A, 0x78};
/**/
void generate_iv(unsigned char *iv, size_t iv_len) {
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0);
    mbedtls_ctr_drbg_random(&ctr_drbg, iv, iv_len);
    mbedtls_entropy_free(&entropy);
    mbedtls_ctr_drbg_free(&ctr_drbg);
}

// Hàm mã hóa JSON
int encryptJSON(const char *json, const unsigned char *key, unsigned char **encrypted, unsigned char *iv, unsigned char *tag) {
    if (!json || !key || !encrypted || !iv || !tag) {
        printf("some thing null\r\n");
        return -1;
    }

    mbedtls_gcm_context gcm;
    mbedtls_gcm_init(&gcm);

    // Thiết lập khóa AES-256
    if (mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, 256) != 0) {
        mbedtls_gcm_free(&gcm);
        printf("setup AES-256 key failed\r\n");
        return -1;
    }

    // Sinh IV ngẫu nhiên
    generate_iv(iv, IV_SIZE);

    // Lấy độ dài dữ liệu JSON
    int json_len = strlen(json);
    *encrypted = (unsigned char *)malloc(json_len);
    //*encrypted = (unsigned char *)heap_caps_malloc(json_len, MALLOC_CAP_SPIRAM);
    if (!*encrypted) {
        mbedtls_gcm_free(&gcm);
        printf("Không cấp phát được bộ nhớ\r\n");
        return -1;
    }

    // Mã hóa JSON với AES-256-GCM
    if (mbedtls_gcm_crypt_and_tag(&gcm, MBEDTLS_GCM_ENCRYPT, json_len, iv, IV_SIZE, NULL, 0,
                                  (const unsigned char *)json, *encrypted, TAG_SIZE, tag) != 0) {
        free(*encrypted);
        mbedtls_gcm_free(&gcm);
        printf("Khong ma hoa duoc json\r\n");
        return -1;
    }

    mbedtls_gcm_free(&gcm);
    return json_len;
}

// Hàm giải mã JSON
int decryptJSON(const unsigned char *encrypted, size_t encrypted_len, const unsigned char *key,
                const unsigned char *iv, const unsigned char *tag, unsigned char **decrypted) {
    if (!encrypted || !key || !iv || !tag || !decrypted) {
        printf(" loi key loi\n ");
        return -1;
    }

    mbedtls_gcm_context gcm;
    mbedtls_gcm_init(&gcm);

    // Thiết lập khóa AES-256
    if (mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key, 256) != 0) {
        mbedtls_gcm_free(&gcm);
        printf(" loi key loi 2\n ");
        return -1;
    }

    // Cấp phát bộ nhớ cho dữ liệu giải mã
     *decrypted = (unsigned char *)malloc(encrypted_len + 1);
    //*decrypted = (unsigned char *)heap_caps_malloc(encrypted_len + 1, MALLOC_CAP_SPIRAM);
    if (!*decrypted) {
        mbedtls_gcm_free(&gcm);
        printf(" loi key loi 3\n ");
        return -1;
    }

    // Giải mã JSON bằng AES-256-GCM
    if (mbedtls_gcm_auth_decrypt(&gcm, encrypted_len, iv, IV_SIZE, NULL, 0, tag, TAG_SIZE,
                                 encrypted, *decrypted) != 0) {
        // free(*decrypted);
        heap_caps_free(*decrypted);
        mbedtls_gcm_free(&gcm);
        printf(" loi key loi 4\n ");
        return -1;
    }

    mbedtls_gcm_free(&gcm);
    (*decrypted)[encrypted_len] = '\0';  // Đảm bảo chuỗi kết thúc đúng cách
    return encrypted_len;
}

void main_encrypt(void) {
    //  const char *json = "{\"ok\": true}";  // Chuỗi JSON cần mã hóa
    // const char *json = "{\"total\": 1, \"leds\": [{\"r\": 255, \"g\": 0, \"b\": 0, \"brightness\": 100, \"num_leds\": 10}]}";
    // const char* json = "{\n"
    // "  \"frequency_data\": [1000, 2000, 3000, 4000, 5000],\n"
    // "  \"power_data\": [-30.5, -28.7, -25.1, -23.0, -20.8],\n"
    // "  \"time_sleep\": [100, 200, 300, 400, 500],\n"
    // "  \"detect\": 1\n"
    // "}";
    const char *json =
        "{\n"
        "  \"gun1\": { \"action\": 2, \"order_time\": 220, \"limit_time\": 7000 },\n"
        "  \"gun2\": { \"action\": 2, \"order_time\": 220, \"limit_time\": 7000 },\n"
        "  \"gun3\": { \"action\": 2, \"order_time\": 0,   \"limit_time\": 7000 },\n"
        "  \"gun4\": { \"action\": 2, \"order_time\": 220, \"limit_time\": 7000 },\n"
        "  \"gun5\": { \"action\": 2, \"order_time\": 220, \"limit_time\": 7000 },\n"
        "  \"gun6\": { \"action\": 2, \"order_time\": 220, \"limit_time\": 7000 }\n"
        "}";
    unsigned char iv[IV_SIZE];
    unsigned char *encrypted = NULL;
    unsigned char tag[TAG_SIZE];

    // Mã hóa JSON
    int encrypted_len = encryptJSON(json, key, &encrypted, iv, tag);

    if (encrypted_len < 0) {
        printf("Mã hóa thất bại!\n");
        return;
    }
    // In ra IV, dữ liệu mã hóa và Tag
    print_hex("IV", iv, IV_SIZE);
    vTaskDelay(2 / portTICK_PERIOD_MS);
    print_hex("Data", encrypted, encrypted_len);
    vTaskDelay(2 / portTICK_PERIOD_MS);
    print_hex("Tag", tag, TAG_SIZE);
    // printf("%s\r\n",encrypted);
    free(encrypted);
}

void hexstr_to_bytes(const char *hexstr, uint8_t *out_bytes, size_t out_len) {
    for (size_t i = 0; i < out_len; i++) {
        sscanf(hexstr + 2 * i, "%2hhx", &out_bytes[i]);
    }
}
void test_decrypt() {
    const char *hex_iv = "e6e2e9089ff53fb359c2edebd568f99f";
    const char *hex_data = "17c28ae55e2e20129526e86dfb0e97aa5dd6ef995acdd643";
    const char *hex_tag = "99635245f4538dc975208c5aeb530b4e";
    //{"serial_number":"PLWabc1235","command_type":20,"data":{"iv":"e9fa8f623d8913e068826f580fb7fea8","data":"25c65c76c7bbb4ba8c410cbe594820da6b057c2897f16b7f3bff6024a0b49bb1766f4b399f4d25785ae0035e6a5777e9ad7418f4726f1682d912135cdbbdb748035d385d9af0a240dbe2416a792c8a33d406aef56ee0280ba34757a681177881910312e70e1b0975f1b2471793e6dc2a20c21f562ef3dbe5638fc466ce5dd45827e45a748eb57b3f1c87d904de7358e55a24cd5034453cf2f1383becae930e502d3bc1d749c602eadf3bada2753720c38f5009e284c2dbca855110e4269b4a5f8e22bc58322d9ebc7db4b133ccbd522535f53ba70ae418a3ebe28904a0ecbb4c89aedc555e7ef1d2362f2b2bb45f7424c5e036fff739f2172674711156023bb07e2641a687b865296f165a11ad508861464b919b1c8787b2ffe571f46a6622da7d37fe45816a47dc143bf0aed7b7af33190fa4cb94d84065a2d368e3db854591db724e7a2c37211294b94a2b7802bbcb8c022484edea88f454aa9037a6ae604cd27adb1a16b0e2126cba9a6c65daf217dee356b584e1638ce7a8d13b8a8b6d6af1872ee8250e1a1fbabe8ea3760ab225be9fb597cd7c018b05a87edabec6555e7f137d759e3353189917af0771849ae1f1817515acbb434ccd1d6f4ebba893c41e8c4c16357dfc9d164d0639b7181059cef8e4a6aacd67875b21567ebf577e0564a43bc8bcc76c49b102ccc5eb50d0e5cd4b6a8758aecd7dda4797bbe9e4f4e3f9fda0aaa740c16aabdbae852c02c7595d19c3c503696eff774f1eea276957131efaf686e7cac14655b62a4aa4f0621aa7b3f0dc659b4ac4a3659b780b07a93ba5994293615acdb3c286519b66a6db3f7b7746830133fbac6b1cd6591fd56c6a3c06ab3d4bfbd95ea58deeca412a9355b492f23f859eda6c9513cdb0","tag":"71616390c953e9c066f67dc690664f6b"}}

    size_t iv_len = strlen(hex_iv) / 2;
    size_t data_len = strlen(hex_data) / 2;
    size_t tag_len = strlen(hex_tag) / 2;

    uint8_t iv[iv_len];
    uint8_t data[data_len];
    uint8_t tag[tag_len];

    hexstr_to_bytes(hex_iv, iv, iv_len);
    hexstr_to_bytes(hex_data, data, data_len);
    hexstr_to_bytes(hex_tag, tag, tag_len);

    // // key
    // const unsigned char key[32] = {
    //     0x35, 0x70, 0x7A, 0x63, 0x74, 0x68, 0x61, 0x50,
    //     0x54, 0x34, 0x6C, 0x4C, 0x36, 0x4C, 0x49, 0x66,
    //     0x38, 0x59, 0x47, 0x32, 0x47, 0x30, 0x79, 0x50,
    //     0x45, 0x47, 0x77, 0x67, 0x36, 0x70, 0x5A, 0x78
    // };

    unsigned char *decrypted = NULL;
    int decrypted_len = decryptJSON(data, data_len, key, iv, tag, &decrypted);

    if (decrypted_len > 0) {
        printf("Decrypted JSON test: %s\n", decrypted);
        heap_caps_free(decrypted);  // hoặc free(), tùy malloc method
    } else {
        printf("Giải mã thất bại!\n");
    }
}

unsigned char *decrypt_data(char *hex_iv, char *hex_data, char *hex_tag) {
    size_t iv_len = strlen(hex_iv) / 2;
    size_t data_len = strlen(hex_data) / 2;
    size_t tag_len = strlen(hex_tag) / 2;

    uint8_t iv[iv_len];
    uint8_t data[data_len];
    uint8_t tag[tag_len];

    hexstr_to_bytes(hex_iv, iv, iv_len);
    hexstr_to_bytes(hex_data, data, data_len);
    hexstr_to_bytes(hex_tag, tag, tag_len);
    unsigned char *decrypted = NULL;
    int decrypted_len = decryptJSON(data, data_len, key, iv, tag, &decrypted);

    if (decrypted_len > 0) {
        // printf("Decrypted JSON test: %s\n", decrypted)
        decrypted[decrypted_len] = '\0';
        return decrypted;
        // heap_caps_free(decrypted);  // hoặc free(), tùy malloc method
    } else {
        printf("Giải mã thất bại!\n");
        return NULL;
    }
}

char *encrypt_data(const char *json_data, char *ser_num, int cmd_type) {
    unsigned char iv[IV_SIZE];
    unsigned char *encrypted = NULL;
    unsigned char tag[TAG_SIZE];
    char *data_encrypted = NULL;

    // Mã hóa JSON
    int encrypted_len = encryptJSON(json_data, key, &encrypted, iv, tag);

    char *IV, *Data, *Tag;
    if (encrypted_len < 0) {
        printf("Mã hóa thất bại!\n");
        return NULL;
    }
    IV = convert_int_to_hex("iv", iv, IV_SIZE);
    // vTaskDelay(2 / portTICK_PERIOD_MS);
    Data = convert_int_to_hex("data", encrypted, encrypted_len);
    // vTaskDelay(2 / portTICK_PERIOD_MS);
    Tag = convert_int_to_hex("tag", tag, TAG_SIZE);
    size_t json_size = IV_SIZE * 2 + TAG_SIZE * 2 + encrypted_len * 2 + 512;
    data_encrypted = malloc(json_size + 1);

    snprintf(data_encrypted, json_size, "{\"serial_number\":\"%s\",\"command_type\":%d,\"data\":{\"iv\":\"%s\",\"data\":\"%s\",\"tag\":\"%s\"}}", ser_num, cmd_type, IV, Data, Tag);
    //  printf("%s\r\n",data_encrypted);
    free(IV);free(Data);free(Tag);
    free(encrypted);
    return data_encrypted;
}


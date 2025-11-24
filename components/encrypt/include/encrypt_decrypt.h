#ifndef ENCRYPT_DECRYPT
#define ENCRYPT_DECRYPT

#define IV_SIZE                     16
#define TAG_SIZE                    16 
extern unsigned char                key_encrypt[32];
extern const unsigned char          key[32];
unsigned char *decrypt_data(char *hex_iv, char *hex_data, char *hex_tag);
/**
 * @brief use for encrypting the json data, to publish to mqtt broker
 * form after encrypting: {
 *                          serial_number: ser_num,
 *                          command_type: cmd_type,
 *                          data:{
 *                              @brief //json_data after encrypting
 *                              hev_iv:....,
 *                              hex_data:.....,
 *                              hex_tag:.......
 *                           }
 *                        }
 */
char *encrypt_data(const char *json_data, char *ser_num, int cmd_type);
char *get_subarray(char *data, char *start, char end);
void parse_json(const char *json_str);
void convert_to_json(const char *data);
char *encrypt_data_resond_ping(const char *json_data);

#endif
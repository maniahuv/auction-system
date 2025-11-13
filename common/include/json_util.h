#ifndef JSON_UTIL_H
#define JSON_UTIL_H

#include "cJSON.h"
cJSON* parse_json(const char *json_string);
void free_json_object(cJSON *json_obj);
int get_json_int(cJSON *json_obj, const char *key, int *out_value);
const char* get_json_string(cJSON *json_obj, const char *key);
char* create_error_response(int error_code, const char *message);
char* create_ok_response();


#endif 

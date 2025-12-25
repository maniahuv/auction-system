#include "json_util.h"
#include "protocol.h" 
#include <stdlib.h>

cJSON* parse_json(const char *json_string) {
    if (json_string == NULL) {
        return NULL;
    }
    cJSON *json_obj = cJSON_Parse(json_string);
    return json_obj;
}

void free_json_object(cJSON *json_obj) {
    if (json_obj != NULL) {
        cJSON_Delete(json_obj);
    }
}

int get_json_int(cJSON *json_obj, const char *key, int *out_value) {
    if (json_obj == NULL || key == NULL || out_value == NULL) {
        return 0; 
    }
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(json_obj, key);
    if (cJSON_IsNumber(item)) {
        *out_value = item->valueint;
        return 1; 
    }
    return 0; 
}

const char* get_json_string(cJSON *json_obj, const char *key) {
    if (json_obj == NULL || key == NULL) {
        return NULL; 
    }
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(json_obj, key);
    if (cJSON_IsString(item) && (item->valuestring != NULL)) {
        return item->valuestring;
    }
    return NULL; 
}

char* create_error_response(int error_code, const char *message) {
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) return NULL;

    cJSON_AddNumberToObject(root, "type", S2C_GENERIC_ERROR);
    cJSON_AddNumberToObject(root, "error_code", error_code);
    cJSON_AddStringToObject(root, "message", message);

    char *json_string = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    
    return json_string; 
}

char* create_ok_response() {
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) return NULL;

    cJSON_AddNumberToObject(root, "type", S2C_GENERIC_OK);
    cJSON_AddStringToObject(root, "message", "Operation successful");

    char *json_string = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    return json_string; 
}

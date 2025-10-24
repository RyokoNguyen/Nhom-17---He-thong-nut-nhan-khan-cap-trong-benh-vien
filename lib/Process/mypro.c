#include "var.h"
#include "mypro.h"
#include "config.h"
#include "defines.h"

#include <string.h>
#include <strings.h>



/* Lấy giá trị chuỗi cho key từ JSON rất đơn giản: {"id":"room1","status":"done"} */
static int json_get_string(const char *json, const char *key, char *out, int outsz){
    if(!json || !key || !out || outsz < 2){
        return -1;
    }
    char pat[32]; 
    int n = snprintf(pat, sizeof(pat), "\"%s\"", key);
    if(n <= 0 || n >= (int)sizeof(pat)) return -2;

    const char *k = strstr(json, pat);
    if(!k){
        return -3;
    }
    const char *p = strchr(k + n, ':');        
    if(!p){
        return -4;
    }
    while(*p == ':' || *p == ' ' || *p == '\t'){
        p++; // bỏ trắng
    }
    if(*p != '\"'){
        return -5;                  // chỉ nhận string
    }
    p++;
    int i=0;
    while(*p && *p != '\"' && i<outsz-1){
        out[i++] = *p++;
    }
    if(*p != '\"'){
        return -6;
    }
    out[i] = 0;
    return 0;
}

static void to_lower_inplace(char *s){
    for(; *s; ++s){ 
        if(*s>='A' && *s<='Z'){
            *s = (char)(*s + 32); 
        }
    }
}

static int room_index_from_id(const char *id){
    // chấp nhận "room1".."room4" (không phân biệt hoa/thường)
    if(!id) return -1;
    if(strcasecmp(id, "room1") == 0){
        return 0;
    }else if(strcasecmp(id, "room2") == 0){
        return 1;
    }else if(strcasecmp(id, "room3") == 0){
        return 2;
    }else if(strcasecmp(id, "room4") == 0){
        return 3;
    }
    return -1;
}

void handle_json_room_status(const char *payload /* NUL-terminated */, const char *topic){
    char id[16]={0}, status[16]={0};
    log_i("Topic: %s | Msg: %s\r\n", topic, payload);

    if(json_get_string(payload, "id", id, sizeof(id)) != 0){
        printf("JSON missing id\n"); 
        return;
    }
    if(json_get_string(payload, "status", status, sizeof(status)) != 0){ 
        printf("JSON missing status\n"); 
        return; 
    }
    to_lower_inplace(id);
    to_lower_inplace(status);

    int idx = room_index_from_id(id);
    if(idx < 0){ 
        printf("Unknown room id: %s\n", id); 
        return; 
    }

    if(strcmp(status, "waiting") == 0){
        LIGHT_OFF;
    }else if(strcmp(status, "done") == 0){
        LIGHT_OFF;
    }else if(strcmp(status, "approved") == 0){
        LIGHT_ON;
    }
}
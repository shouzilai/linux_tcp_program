#ifndef __HUP_PARSE_H__
#define __HUP_PARSE_H__

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define HUP_REQ_HDR_H 0xAA
#define HUP_REQ_HDR_L 0xDD
#define HUP_ACK_HDR_H 0xBB
#define HUP_ACK_HDR_L 0xDD


typedef struct hup_protocol_type {
    int hdr_h;
    int hdr_l;
    int cmd;
    int data_len;
    uint8_t* data_addr;
    int xor8;

} hup_protocol_type_t, *hup_protocol_type_p;


struct hup_protocol_assist {
    int is_response;
    int state;
    int read_len;
    int xor8;
    void (*hup_success_handle_func)(hup_protocol_type_p);

};

typedef enum {
    EM_HUP_STATE_HDR_H,
    EM_HUP_STATE_HDR_L,
    EM_HUP_STATE_CMD,
    EM_HUP_STATE_DATA_LEN,
    EM_HUP_STATE_DATA,
    EM_HUP_STATE_CRC,

} hup_protocol_state_em;                    // 帧结构分支  

typedef enum {
    EM_HUP_TYPE_REQ,
    EM_HUP_TYPE_ACK,

} hup_protocol_type_em;                     // 帧结构类型(应答或者请求)


int hup_init(hup_protocol_type_p hup_p, int buf_size, void (*hup_success_handle_func)(hup_protocol_type_p));

int hup_deinit(hup_protocol_type_p hup_p);

hup_protocol_type_p hup_pack(hup_protocol_type_em sort, int cmd, int len, uint8_t* addr);

int hup_parse(hup_protocol_type_p hup_frame, int data);

#endif // __HUP_PARSE_H__
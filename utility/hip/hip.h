#ifndef __HIP_H__
#define __HIP_H__

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HIP_PAYLOAD_SIZE    1024

#define HIP_VERSION         0x30
#define HIP_DEV_TYPE        0x01
#define HIP_DEV_ID          0x000012345678abcd

/*

                            [ HIP  Protocol ]
------------------------------------------------------------------------
|0 1 2 3 4 5 6 7 | 0 1 2 3 4 5 6 7 | 0 1 2 3 4 5 6 7 | 0 1 2 3 4 5 6 7 |
------------------------------------------------------------------------
|    version     |     commond     |      length     |    device type  | 
------------------------------------------------------------------------
|                              device id                               |
------------------------------------------------------------------------
|                              device id                               |
------------------------------------------------------------------------
|                                 seq                                  |
------------------------------------------------------------------------
|    resrved     |
------------------

*/

typedef struct hip_protocol_type
{
    uint8_t  version;
    uint8_t  command;
    uint8_t  length;
    uint8_t  device_type;
    uint64_t device_id;
    uint32_t seq;
    uint8_t* payload;
} hip_protocol_type_t, *hip_protocol_type_p;

typedef struct hip_protocol_handle
{
    hip_protocol_type_t hip_frame;
    uint8_t             state;
    uint8_t             read_len;
    uint8_t             dev_type;
    uint64_t            dev_id;
    uint32_t            cur_seq;
    void (*hip_success_handle_func)(hip_protocol_type_p);
} hip_protocol_handle_t, *hip_protocol_handle_p;

typedef enum
{
    EM_HIP_STATE_VER,
    EM_HIP_STATE_CMD,
    EM_HIP_STATE_LEN,
    EM_HIP_STATE_DEV_TYPE,
    EM_HIP_STATE_DEV_ID,
    EM_HIP_STATE_SEQ,
    EM_HIP_STATE_PL,
} hip_protocol_state_em;

typedef enum
{
    EM_HIP_CMD_DETECT_REQUEST = 0x05,
    EM_HIP_CMD_USER_LOGIN,
    EM_HIP_CMD_KEEP_ALIVE,
    EM_HIP_CMD_UART_PASS_THROUGH,
    EM_HIP_CMD_WAIT,
} hip_protocol_cmd_em;

int hip_init(hip_protocol_handle_p hip, void (*hip_success_handle_func)(hip_protocol_type_p));

int hip_deinit(hip_protocol_handle_p hip);

int hip_pack(hip_protocol_handle_p hip, hip_protocol_cmd_em cmd, uint8_t* frame_buf,
                                uint8_t frame_len, uint8_t* data_addr, uint8_t data_len);

int hip_parse(hip_protocol_handle_p hip, uint8_t data_byte);

#endif // __HIP_H__
#pragma once

#include <stddef.h>
#include <stdint.h>

#define TLV_HEADER_SIZE 2 // 1 byte for type, 1 byte for length
#define TLV_MAX_PAYLOAD_SIZE 32
#define TLV_MAX_SIZE (TLV_HEADER_SIZE + TLV_MAX_PAYLOAD_SIZE)

typedef struct {
    uint8_t type;
    uint8_t length;
    uint8_t payload[TLV_MAX_PAYLOAD_SIZE];
} msg_t;


size_t tlv_serialize(uint8_t *out_buf, size_t out_buf_size, const msg_t *tlv);
size_t tlv_deserialize(const uint8_t *in_buf, size_t in_buf_size, msg_t *tlv);



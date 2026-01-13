#include "tlv.h"
#include <string.h>


size_t tlv_serialize(uint8_t *out_buf, size_t out_buf_size, const msg_t *tlv) {
    if(!out_buf || !tlv) return 0;
    if(out_buf_size < tlv->length + TLV_HEADER_SIZE) return 0;

    out_buf[0] = tlv->type;
    out_buf[1] = tlv->length;

    if(tlv->length > 0) {
        memcpy(&out_buf[2], tlv->payload, tlv->length);
    }

    return tlv->length + TLV_HEADER_SIZE;
}

size_t tlv_deserialize(const uint8_t *in_buf, size_t in_buf_size, msg_t *tlv) {
    if(!in_buf || !tlv) return 0;
    if(in_buf_size < TLV_HEADER_SIZE) return 0;

    tlv->type = in_buf[0];
    tlv->length = in_buf[1];

    if(tlv->length > 0) {
        memcpy(tlv->payload, &in_buf[2], tlv->length);
    }

    return tlv->length + TLV_HEADER_SIZE;
}
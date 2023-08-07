#include "accel.h"
#include <stdint.h>
typedef unsigned char uint8_t;

int tryDecode(const uint8_t *data, int data_size, uint8_t *msg) {
//    #pragma HLS interface ap_ctrl_none port=return
//    #pragma HLS interface axis port=data
//    #pragma HLS interface axis port=msg

// For pointer type
#pragma HLS INTERFACE m_axi port=data offset=slave bundle=gmem0 num_write_outstanding=16 num_read_outstanding=16 max_write_burst_length=16 max_read_burst_length=    16 depth=16 latency=125

#pragma HLS INTERFACE m_axi port=msg offset=slave bundle=gmem0 num_write_outstanding=16 num_read_outstanding=16 max_write_burst_length=16 max_read_burst_length=    16 depth=16 latency=125


#pragma HLS INTERFACE s_axilite port=data bundle=control
#pragma HLS INTERFACE s_axilite port=data_size bundle=control
#pragma HLS INTERFACE s_axilite port=msg bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control


    if (data_size == 1 && data[0] == 0x04) {
        msg[0] = data[0];
        return 1;
    }

    for (int i = 0; i < data_size; i++) {
        #pragma HLS pipeline II=1
        if (data[i] == '\n') {
            if (i > 0 && data[i - 1] == '\r') {
                for (int j = 0; j < i - 1; j++) {
                    #pragma HLS unroll factor=8
                    msg[j] = data[j];
                }
                return i + 1;
            } else {
                for (int j = 0; j < i; j++) {
                    #pragma HLS unroll factor=8
                    msg[j] = data[j];
                }
                return i + 1;
            }
        }
    }
    return 0;
}

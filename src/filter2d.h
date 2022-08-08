
#include "ap_int.h"
#include "ap_axi_sdata.h"

#include <hls_stream.h>

#define WIDTH 	128
#define HEIGHT	128

typedef int DTYPE;

void filter2d_accel(DTYPE* img_in, DTYPE* kernel, DTYPE* img_out, int rows, int cols);

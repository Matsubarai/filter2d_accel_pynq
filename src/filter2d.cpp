#include <iostream>
using namespace std;
#include <math.h>
#include "filter2d.h"

static DTYPE filter2d_kernel(DTYPE WB[3][3], DTYPE K[3][3])
{
#pragma HLS PIPELINE
	int row, col;
	short int out_pix = 0;
	for(row = 0; row < 3; row++)
		for(col = 0; col < 3; col++)
			out_pix += WB[row][col] * K[row][col];

	return (DTYPE) out_pix;
}

static void filter2d_move(DTYPE WB[3][3], DTYPE top, DTYPE mid, DTYPE btm)
{
	WB[0][0] = WB[0][1];
	WB[1][0] = WB[1][1];
	WB[2][0] = WB[2][1];
	WB[0][1] = WB[0][2];
	WB[1][1] = WB[1][2];
	WB[2][1] = WB[2][2];
	WB[0][2] = top;
	WB[1][2] = mid;
	WB[2][2] = btm;
}

void filter2d_accel(DTYPE* img_in, DTYPE* kernel, DTYPE* img_out, int rows, int cols)
{

#pragma HLS INTERFACE mode=m_axi port=kernel depth=9 //bundle=KRNL
#pragma HLS INTERFACE mode=m_axi port=img_in depth=16384 //bundle=IMG
#pragma HLS INTERFACE mode=m_axi port=img_out depth=16384 //bundle=IMG
#pragma HLS INTERFACE mode=s_axilite port=rows bundle=CTRL
#pragma HLS INTERFACE mode=s_axilite port=cols bundle=CTRL
#pragma HLS INTERFACE mode=s_axilite port=return bundle=CTRL
    //Write your code here

	DTYPE K[3][3];
#pragma HLS ARRAY_PARTITION variable=K complete dim=0

	DTYPE LineBuffer[3][WIDTH];
#pragma HLS ARRAY_PARTITION variable=LineBuffer complete dim=1

	DTYPE WindowBuffer[3][3];
#pragma HLS ARRAY_PARTITION variable=WindowBuffer complete dim=0

	int row, col;

	DTYPE top, mid, btm;//line buffer row index

	for(row = 0; row < 3; row++)
		for(col = 0; col < 3; col++){
			K[row][col] = *kernel++;
		}

	DTYPE _filter2d;

	int lb_r = 2;
	const int lb_r_i[3][3] = {{1, 2, 0}, {2, 0, 1}, {0, 1, 2}};
#pragma HLS ARRAY_PARTITION variable=lb_r_i complete dim=0

	for(col = 0; col < cols << 1; col++)
	{
#pragma HLS LOOP_TRIPCOUNT min=1 max=256
#pragma HLS pipeline
		if(col < cols)
			LineBuffer[0][col] = *img_in++;
		else
			LineBuffer[1][col - cols] = *img_in++;
	}

	for(row = 2; row < rows; row++)
#pragma HLS LOOP_TRIPCOUNT min=1 max=126
		for(col = 0; col < cols; col++)
		{
#pragma HLS LOOP_TRIPCOUNT min=1 max=128
#pragma HLS pipeline
			LineBuffer[lb_r][col] = *img_in++;
			if(col > 2){
				top = LineBuffer[lb_r_i[lb_r][0]][col];
				mid = LineBuffer[lb_r_i[lb_r][1]][col];
				btm = LineBuffer[lb_r_i[lb_r][2]][col];
				filter2d_move(WindowBuffer, top, mid, btm);
				_filter2d = filter2d_kernel(WindowBuffer, K);
				*img_out = _filter2d;
				img_out++;
				if(col == cols - 1){
					lb_r++;
					if(lb_r == 3) lb_r = 0;
				}
			}else{
				WindowBuffer[0][col] = LineBuffer[lb_r_i[lb_r][0]][col];
				WindowBuffer[1][col] = LineBuffer[lb_r_i[lb_r][1]][col];
				WindowBuffer[2][col] = LineBuffer[lb_r_i[lb_r][2]][col];
				if(col == 2){
					_filter2d = filter2d_kernel(WindowBuffer, K);
					*img_out = _filter2d;
					img_out++;
				}
			}
		}
	}
}

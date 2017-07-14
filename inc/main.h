#ifndef __H264_H__
#define __H264_H__
#include "nal.h"
#include "slicehdr.h"
typedef struct __frame {
  int Lwidth,Lheight,Lpitch;
  int Cwidth,Cheight,Cpitch;
  unsigned char *L, *C[2];
} frame;
typedef struct _MB_frame {

  unsigned char L[256];
  unsigned char C[2][64]; 

} MB_frame;

#define L_pixel(f,x,y)   (f->L[(y)*f->Lpitch+(x)])
#define Cr_pixel(f,x,y) (f->C[1][(y)*f->Cpitch+(x)])
#define Cb_pixel(f,x,y) (f->C[0][(y)*f->Cpitch+(x)])
#define C_pixel(f,iCbCr,x,y) (f->C[iCbCr][(y)*f->Cpitch+(x)])

#define H264_WIDTH(info)  ((info)&0xFFFF)
#define H264_HEIGHT(info) ((info)>>16)
int h264_open(char *filename);

frame *h264_decode_frame(int verbose);
void h264_rewind();
void h264_close();

int h264_frame_no();
//NOC_related
nal_unit get_nalu_func();
int decode_hdr(nal_unit reieved_nalu,slice_header *sh,seq_parameter_set sps,pic_parameter_set pps);
int my_h264_open(char *filename,seq_parameter_set *sps,pic_parameter_set *pps);
#endif /*__H264_H__*/

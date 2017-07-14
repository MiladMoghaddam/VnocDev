#include "common.h"
#include "input.h"
#include "nal.h"
#include "cavlc.h"
#include "params.h"
#include "slicehdr.h"
#include "slice.h"
#include "residual.h"
#include "mode_pred.h" 
#include "main.h"
#include <iostream>
using namespace std;

frame *this_frame=NULL, *ref=NULL;
mode_pred_info *mpi=NULL;
int frame_no;
nal_unit nalu;
seq_parameter_set sps;
pic_parameter_set pps;
slice_header sh;

int my_h264_open(char *filename,seq_parameter_set *sps,pic_parameter_set *pps) {
  int have_sps=0,have_pps=0;
  if(!input_open(filename)) {
    fprintf(stderr,"H.264 Error: Cannot open input file!\n");
    return 0;
  }
  init_code_tables();
  frame_no=0;
  while(get_next_nal_unit(&nalu)) {
	//cin.get();
    switch(nalu.nal_unit_type) {
      case 7:  // sequence parameter set //
		//printf ("nalu.nal_unit_type = case 7");
		//cin.get();

        if(have_sps)
          fprintf(stderr,"H.264 Warning: Duplicate sequence parameter set, skipping!\n");
        else {
          decode_seq_parameter_set(sps);
          have_sps=1;
        }
        break;
      case 8:  // picture parameter set //

        if(!have_sps)
          fprintf(stderr,"H.264 Warning: Picture parameter set without sequence parameter set, skipping!\n");
        else if(have_pps)
          fprintf(stderr,"H.264 Warning: Duplicate picture parameter set, skipping!\n");
        else {
          decode_pic_parameter_set(pps);
          have_pps=1;
          if(check_unsupported_features(sps,pps)) {
            fprintf(stderr,"H.264 Error: Unsupported features found in headers!\n");
            input_close();
            return 0;
          }
          this_frame=alloc_frame(sps->PicWidthInSamples,sps->FrameHeightInSamples);
          ref=alloc_frame(sps->PicWidthInSamples,sps->FrameHeightInSamples);
          mpi=alloc_mode_pred_info(sps->PicWidthInSamples,sps->FrameHeightInSamples);
          return (sps->FrameHeightInSamples<<16)|sps->PicWidthInSamples;
        }
        break;
      case 1: case 5:  // coded slice of a picture //
        fprintf(stderr,"H.264 Warning: Pictures sent before headers!\n");
        break;
      default:  // unsupported NAL unit type //
        fprintf(stderr,"H.264 Warning: NAL unit with unsupported type, skipping!\n");
    }
  }
  fprintf(stderr,"H.264 Error: Unexpected end of file!\n");
  return 0;
}



nal_unit get_nalu_func(){
	get_next_nal_unit(&nalu);
	return nalu;
}	 


int decode_hdr(nal_unit reieved_nalu,slice_header *sh,seq_parameter_set sps,pic_parameter_set pps){
	
    if(reieved_nalu.nal_unit_type==1 || reieved_nalu.nal_unit_type==5) {		
      ++frame_no;	
      decode_slice_header(sh,&sps,&pps,&reieved_nalu);
	}
}
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
int h264_open(char *filename) {
  int have_sps=0,have_pps=0;
  if(!input_open(filename)) {
    fprintf(stderr,"H.264 Error: Cannot open input file!\n");
    return 0;
  }
  init_code_tables();
  frame_no=0;
  while(get_next_nal_unit(&nalu)) {
	//cin.get();
    switch(nalu.nal_unit_type) {
      case 7:  // sequence parameter set //

        if(have_sps)
          fprintf(stderr,"H.264 Warning: Duplicate sequence parameter set, skipping!\n");
        else {
          decode_seq_parameter_set(&sps);
          have_sps=1;
        }
        break;
      case 8:  // picture parameter set //

        if(!have_sps)
          fprintf(stderr,"H.264 Warning: Picture parameter set without sequence parameter set, skipping!\n");
        else if(have_pps)
          fprintf(stderr,"H.264 Warning: Duplicate picture parameter set, skipping!\n");
        else {
          decode_pic_parameter_set(&pps);
          have_pps=1;
          if(check_unsupported_features(&sps,&pps)) {
            fprintf(stderr,"H.264 Error: Unsupported features found in headers!\n");
            input_close();
            return 0;
          }
          this_frame=alloc_frame(sps.PicWidthInSamples,sps.FrameHeightInSamples);
          ref=alloc_frame(sps.PicWidthInSamples,sps.FrameHeightInSamples);
          mpi=alloc_mode_pred_info(sps.PicWidthInSamples,sps.FrameHeightInSamples);
          return (sps.FrameHeightInSamples<<16)|sps.PicWidthInSamples;
        }
        break;
      case 1: case 5:  // coded slice of a picture //
        fprintf(stderr,"H.264 Warning: Pictures sent before headers!\n");
        break;
      default:  // unsupported NAL unit type //
        fprintf(stderr,"H.264 Warning: NAL unit with unsupported type, skipping!\n");
    }
  }
  fprintf(stderr,"H.264 Error: Unexpected end of file!\n");
  return 0;
}



frame *h264_decode_frame(int verbose) {
	int i;
	//NOC (1) get_nal and decode
  if(get_next_nal_unit(&nalu)){ //if
	//get_next_nal_unit(&nalu);
    if(nalu.nal_unit_type==1 || nalu.nal_unit_type==5) {		
      perf_enter("slice decoding");
      ++frame_no;
      decode_slice_header(&sh,&sps,&pps,&nalu);
}
  }	
	
	
  printf ("! get_next_nal_unit(&nalu) ");

  return NULL;
}

void h264_rewind() {
  input_rewind();
  frame_no=0;
}

void h264_close() {
  free_frame(this_frame);
  free_frame(ref);
  free_mode_pred_info(mpi);
  input_close();
}


int h264_frame_no() {
  return frame_no;
}


///////////////////////////////////////////////////////////////////////////////

#ifdef BUILD_TESTS

int _test_main(int argc, char *argv[]) {
  FILE *out;
  frame *f;
  int info;

  info=h264_open("../streams/in.264");
  if(!info) return 1;

  if(!(out=fopen("../streams/out","wb"))) {
    fprintf(stderr,"Error: Cannot open output file!\n");
    return 1;
  }

  printf("H.264 stream, %dx%d pixels\n",H264_WIDTH(info),H264_HEIGHT(info));

  while((f=h264_decode_frame(1))) {
    printf("\n");
    fwrite(f->L,f->Lpitch,f->Lheight,out);
    fwrite(f->C[0],f->Cpitch,f->Cheight,out);
    fwrite(f->C[1],f->Cpitch,f->Cheight,out);
if(frame_no>=1) break;
  }
  
  fclose(out);
  h264_close();
  return 0;
}

#endif

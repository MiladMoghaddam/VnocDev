#ifndef __SLICE_H__
#define __SLICE_H__

#include "params.h"
#include "nal.h"
#include "slicehdr.h"
#include "common.h"
#include "mode_pred.h"
#include "mbmodes.h"

void decode_MicrobBlock(slice_header *sh,seq_parameter_set *sps, pic_parameter_set *pps,mode_pred_info *mpi,int CurrMbAddr,int frame_no,mb_mode *mb,nal_unit *nalu,int *mb_pos_x,int *mb_pos_y,int *QPy,int *QPc, int LumaACLevel[16][16], int LumaDCLevel[16],int Intra4x4ScanOrder[16][2],int ChromaDCLevel[2][4],int ChromaACLevel[2][4][16]);

void decode_slice_data_loop(slice_header *sh,
                       seq_parameter_set *sps, pic_parameter_set *pps,
                       nal_unit *nalu,
                       frame *this_frame, frame *ref,
                       mode_pred_info *mpi);

void decode_slice_data(slice_header *sh,
                       seq_parameter_set *sps, pic_parameter_set *pps,
                       nal_unit *nalu,
                       frame *this_frame, frame *ref,
                       mode_pred_info *mpi, int CurrMbAddr);

void decode_module(slice_header *sh,seq_parameter_set *sps, pic_parameter_set *pps,mode_pred_info *mpi,int CurrMbAddr,int frame_no,mb_mode *mb,nal_unit *nalu,int *mb_pos_x,int *mb_pos_y,int *QPy,int *QPc, int LumaACLevel[16][16],int LumaDCLevel[16],int Intra4x4ScanOrder[16][2],int ChromaDCLevel[2][4],int ChromaACLevel[2][4][16]);

void decode_module_MB(slice_header *sh,seq_parameter_set *sps, pic_parameter_set *pps,mode_pred_info *mpi,int CurrMbAddr,mb_mode *mb,nal_unit *nalu,int *mb_pos_x,int *mb_pos_y,int *QPy,int *QPc, int LumaACLevel[16][16],int LumaDCLevel[16],int ChromaDCLevel[2][4],int ChromaACLevel[2][4][16],int *intra_chroma_pred_mode);

void intra_prediction_module_MB(int MbPartPredMode,int mb_pos_x,int mb_pos_y,frame *this_frame,mode_pred_info *mpi,int QPy,int LumaACLevel[][16],int intra_chroma_pred_mode,int constrained_intra_pred_flag,int Intra16x16PredMode,int *LumaDCLevel_0,int ChromaDCLevel[2][4],int ChromaACLevel[2][4][16],int CodedBlockPatternChroma, int QPc);

void inter_prediction_module_MB(int MbPartPredMode,frame *this_frame,frame *ref,mode_pred_info *mpi,int mb_pos_x,int mb_pos_y,int LumaACLevel[][16],int QPy,int skip_run_mode,int CurrMbAddr,int PicWidthInMbs,int ChromaDCLevel[2][4],int ChromaACLevel[2][4][16],int CodedBlockPatternChroma,int QPc);

void dump_luma_chroma(int LumaACLevel[16][16],int LumaDCLevel[16],int ChromaACLevel[2][4][16],int ChromaDCLevel[2][4]);
#endif /*__SLICE_H__*/

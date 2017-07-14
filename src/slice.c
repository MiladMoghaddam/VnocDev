#include "common.h"
#include "input.h"
#include "cavlc.h"
#include "params.h"
#include "mbmodes.h"
#include "residual.h"
#include "mode_pred.h"
#include "intra_pred.h"
#include "mocomp.h"
#include "block.h"
#include "slicehdr.h"
#include "slice.h"
#include "iostream"
using namespace std;

extern int frame_no;

// needed from mbmodes.c
extern int CodedBlockPatternMapping_Intra4x4[];
extern int CodedBlockPatternMapping_Inter[];

int Intra4x4ScanOrder[16][2]={
  { 0, 0},  { 4, 0},  { 0, 4},  { 4, 4},
  { 8, 0},  {12, 0},  { 8, 4},  {12, 4},
  { 0, 8},  { 4, 8},  { 0,12},  { 4,12},
  { 8, 8},  {12, 8},  { 8,12},  {12,12}
};

int QPcTable[22]=
  {29,30,31,32,32,33,34,34,35,35,36,36,37,37,37,38,38,38,39,39,39,39};


// some macros for easier access to the various ModePredInfo structures
#define LumaDC_nC     get_luma_nC(mpi,mb_pos_x,mb_pos_y)
#define LumaDC_nC_withPointer     get_luma_nC(mpi,*mb_pos_x,*mb_pos_y)
#define LumaAC_nC     get_luma_nC(mpi,mb_pos_x+Intra4x4ScanOrder[i8x8*4+i4x4][0],mb_pos_y+Intra4x4ScanOrder[i8x8*4+i4x4][1])
#define LumaAC_nC_withPointer     get_luma_nC(mpi,*mb_pos_x+Intra4x4ScanOrder[i8x8*4+i4x4][0],*mb_pos_y+Intra4x4ScanOrder[i8x8*4+i4x4][1])
#define ChromaDC_nC   -1
#define ChromaAC_nC   get_chroma_nC(mpi,mb_pos_x+(i4x4&1)*8,mb_pos_y+(i4x4>>1)*8,iCbCr)
#define ChromaAC_nC_withPointer   get_chroma_nC(mpi,*mb_pos_x+(i4x4&1)*8,*mb_pos_y+(i4x4>>1)*8,iCbCr)
#define LumaAdjust    ModePredInfo_TotalCoeffL(mpi,(mb_pos_x+Intra4x4ScanOrder[i8x8*4+i4x4][0])>>2,(mb_pos_y+Intra4x4ScanOrder[i8x8*4+i4x4][1])>>2) =
#define LumaAdjust_withPointer    ModePredInfo_TotalCoeffL(mpi,(*mb_pos_x+Intra4x4ScanOrder[i8x8*4+i4x4][0])>>2,(*mb_pos_y+Intra4x4ScanOrder[i8x8*4+i4x4][1])>>2) =
#define ChromaAdjust  ModePredInfo_TotalCoeffC(mpi,(mb_pos_x+(i4x4&1)*8)>>3,(mb_pos_y+(i4x4>>1)*8)>>3,iCbCr) =
#define ChromaAdjust_withPointer  ModePredInfo_TotalCoeffC(mpi,(*mb_pos_x+(i4x4&1)*8)>>3,(*mb_pos_y+(i4x4>>1)*8)>>3,iCbCr) =
#define Intra4x4PredMode(i) ModePredInfo_Intra4x4PredMode(mpi,(mb_pos_x+Intra4x4ScanOrder[i][0])>>2,(mb_pos_y+Intra4x4ScanOrder[i][1])>>2)
#define Intra4x4PredMode_withPointer(i) ModePredInfo_Intra4x4PredMode(mpi,(*mb_pos_x+Intra4x4ScanOrder[i][0])>>2,(*mb_pos_y+Intra4x4ScanOrder[i][1])>>2)

void dump_luma_chroma(int LumaACLevel[16][16],int LumaDCLevel[16],int ChromaACLevel[2][4][16],int ChromaDCLevel[2][4]){
	printf("LumaACLevel:\n");
	for (int i=0;i<16;i++)
		for (int j=0;j<16;j++)
			printf("%d, ",LumaACLevel[i][j]);

	printf("LumaDCLevel:\n");
	for (int i=0;i<16;i++)
		printf("%d, ",LumaDCLevel[i]);

	printf("ChromaACLevel:\n");
	for (int i=0;i<2;i++)
		for (int j=0;j<4;j++)
			for (int k=0;k<16;k++)
				printf("%d, ",ChromaACLevel[i][j][k]);

	printf("ChromaDCLevel:\n");
	for (int i=0;i<2;i++)
		for (int j=0;j<4;j++)
				printf("%d, ",ChromaDCLevel[i][j]);
	
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////NOC related////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void intra_prediction_module_MB(int MbPartPredMode,int mb_pos_x,int mb_pos_y,frame *this_frame,mode_pred_info *mpi,int QPy,int LumaACLevel[][16],int intra_chroma_pred_mode,int constrained_intra_pred_flag,int Intra16x16PredMode,int *LumaDCLevel_0,int ChromaDCLevel[2][4],int ChromaACLevel[2][4][16],int CodedBlockPatternChroma, int QPc)
	{
		
        if(MbPartPredMode==Intra_4x4) {  ///////////////// Intra_4x4_Pred /////////////
		  //printf("21:	if(mb.MbPartPredMode[0]==Intra_4x4)\n");
          int i;
	//printf("1\n");
	//for (int i=0;i<16;i++)
	//	for (int j=0;j<16;j++){
	//		printf("(%d,%d,%d) ",(*this_frame).L[(mb_pos_y+j)*(*this_frame).Lpitch+(mb_pos_x+i)],(*this_frame).C[0][((mb_pos_y+j)>>1)*(*this_frame).Cpitch+((mb_pos_x+i)>>1)],(*this_frame).C[1][((mb_pos_y+j)>>1)*(*this_frame).Cpitch+((mb_pos_x+i)>>1)]);
	//	}
		//cin.get();
 		
         for(i=0; i<16; ++i) {
            int x=(mb_pos_x)+Intra4x4ScanOrder[i][0];
            int y=(mb_pos_y)+Intra4x4ScanOrder[i][1];
            perf_enter("intra prediction");
            Intra_4x4_Dispatch(this_frame,mpi,x,y,i);//each of 16 luma subblock is predicted
            perf_enter("block entering");
            enter_luma_block(&LumaACLevel[i][0],this_frame,x,y,QPy,0);//each of 16 luma subblock is transformed
          }
	//printf("2\n");
	//for (int i=0;i<16;i++)
	//	for (int j=0;j<16;j++){
	//		printf("(%d,%d,%d) ",(*this_frame).L[(mb_pos_y+j)*(*this_frame).Lpitch+(mb_pos_x+i)],(*this_frame).C[0][((mb_pos_y+j)>>1)*(*this_frame).Cpitch+((mb_pos_x+i)>>1)],(*this_frame).C[1][((mb_pos_y+j)>>1)*(*this_frame).Cpitch+((mb_pos_x+i)>>1)]);
	//	}
		//cin.get();
 

          Intra_Chroma_Dispatch(this_frame,mpi,intra_chroma_pred_mode,(mb_pos_x)>>1,(mb_pos_y)>>1,constrained_intra_pred_flag);//Intra Chroma prediction 
 		

        } else if(MbPartPredMode==Intra_16x16) {  ////// Intra_16x16_Pred //////////////////
		  //printf("22:	if(mb.MbPartPredMode[0]==Intra_16x16)\n");
          int i,j;
          perf_enter("intra prediction");
          Intra_16x16_Dispatch(this_frame,mpi,Intra16x16PredMode,mb_pos_x,mb_pos_y,constrained_intra_pred_flag);//whole macroblock is predicted
          perf_enter("block entering");
		  //transform			
          transform_luma_dc(LumaDCLevel_0,&LumaACLevel[0][0],QPy);//16 luma subblock are transformed
          for(i=0; i<16; ++i) {
            int x=(mb_pos_x)+Intra4x4ScanOrder[i][0];
            int y=(mb_pos_y)+Intra4x4ScanOrder[i][1];
            enter_luma_block(&LumaACLevel[i][0],this_frame,x,y,QPy,1);
          }
		  
          perf_enter("block entering");
          Intra_Chroma_Dispatch(this_frame,mpi,intra_chroma_pred_mode,(mb_pos_x)>>1,(mb_pos_y>>1),constrained_intra_pred_flag);//Intra Chroma prediction 
          // act as if all transform blocks inside this_frame macroblock were
          // predicted using the Intra_4x4_DC prediction mode
          // (without constrained_intra_pred, we'd have to do the same for
          // inter MBs)
          for(i=0; i<4; ++i) for(j=0; j<4; ++j)
            ModePredInfo_Intra4x4PredMode(mpi,((mb_pos_x)>>2)+j,((mb_pos_y)>>2)+i)=2;
		}


     if(CodedBlockPatternChroma) { ////////////////////// Chroma Residual
 		  //printf("24:	if(mb.CodedBlockPatternChroma)\n");
          int iCbCr,i;
          perf_enter("block entering");
          for(iCbCr=0; iCbCr<2; ++iCbCr) {
            transform_chroma_dc(&ChromaDCLevel[iCbCr][0],QPc);
            for(i=0; i<4; ++i)
              ChromaACLevel[iCbCr][i][0]=ChromaDCLevel[iCbCr][i];
            for(i=0; i<4; ++i)
              enter_chroma_block(&ChromaACLevel[iCbCr][i][0],this_frame,iCbCr,
                (mb_pos_x>>1)+Intra4x4ScanOrder[i][0],
                (mb_pos_y>>1)+Intra4x4ScanOrder[i][1],
              QPc,1);
          }
        }




	}	

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void inter_prediction_module_MB(int MbPartPredMode,frame *this_frame,frame *ref,mode_pred_info *mpi,int mb_pos_x,int mb_pos_y,int LumaACLevel[][16],int QPy,int skip_run_mode, int CurrMbAddr,int PicWidthInMbs,int ChromaDCLevel[2][4],int ChromaACLevel[2][4][16],int CodedBlockPatternChroma,int QPc)
	{
	if (skip_run_mode==1){
		mb_pos_x=(CurrMbAddr)%PicWidthInMbs;
    	mb_pos_y=(CurrMbAddr)/PicWidthInMbs;
    	ModePredInfo_MbMode(mpi,mb_pos_x,mb_pos_y)=P_Skip;
    	mb_pos_x<<=4; mb_pos_y<<=4;
		printf("[%d] %d,%d MB: P_Skip\n",(CurrMbAddr),mb_pos_x,mb_pos_y);
		Derive_P_Skip_MVs(mpi,mb_pos_x,mb_pos_y);
		printf("20: skip: MotionCompensateMB\n");
    	MotionCompensateMB(this_frame,ref,mpi,mb_pos_x,mb_pos_y);
	}
	else	
		if(MbPartPredMode!=Intra_4x4 and MbPartPredMode!=Intra_16x16){
		  //printf("23:	Inter_*_Pred\n");
         int i;


          perf_enter("inter prediction");
          MotionCompensateMB(this_frame,ref,mpi,mb_pos_x,mb_pos_y);
          perf_enter("block entering");
          for(i=0; i<16; ++i) {
            int x=mb_pos_x+Intra4x4ScanOrder[i][0];
            int y=mb_pos_y+Intra4x4ScanOrder[i][1];
            enter_luma_block(&LumaACLevel[i][0],this_frame,x,y,QPy,0);
          }
         


     	  if(CodedBlockPatternChroma) { ////////////////////// Chroma Residual
 		  	printf("24:	if(mb.CodedBlockPatternChroma)\n");
          	int iCbCr,i;
          	perf_enter("block entering");
          	for(iCbCr=0; iCbCr<2; ++iCbCr) {
          	  transform_chroma_dc(&ChromaDCLevel[iCbCr][0],QPc);
          	  for(i=0; i<4; ++i)
          	    ChromaACLevel[iCbCr][i][0]=ChromaDCLevel[iCbCr][i];
          	  for(i=0; i<4; ++i)
          	    enter_chroma_block(&ChromaACLevel[iCbCr][i][0],this_frame,iCbCr,
          	      (mb_pos_x>>1)+Intra4x4ScanOrder[i][0],
          	      (mb_pos_y>>1)+Intra4x4ScanOrder[i][1],
          	    QPc,1);
          	}
          }

		}

	}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void intra_prediction_module(int MbPartPredMode,int mb_pos_x,int mb_pos_y,int Intra4x4ScanOrder[][2],frame *this_frame,mode_pred_info *mpi,int QPy,int LumaACLevel[][16],int intra_chroma_pred_mode,int constrained_intra_pred_flag,int Intra16x16PredMode,int *LumaDCLevel_0,int ChromaDCLevel[2][4],int ChromaACLevel[2][4][16],int CodedBlockPatternChroma, int QPc)
	{
		
        if(MbPartPredMode==Intra_4x4) {  ///////////////// Intra_4x4_Pred /////////////
		  //printf("21:	if(mb.MbPartPredMode[0]==Intra_4x4)\n");
          int i;
          for(i=0; i<16; ++i) {
            int x=(mb_pos_x)+Intra4x4ScanOrder[i][0];
            int y=(mb_pos_y)+Intra4x4ScanOrder[i][1];
            perf_enter("intra prediction");
            Intra_4x4_Dispatch(this_frame,mpi,x,y,i);//each of 16 luma subblock is predicted
            perf_enter("block entering");
            enter_luma_block(&LumaACLevel[i][0],this_frame,x,y,QPy,0);//each of 16 luma subblock is transformed
          }
          Intra_Chroma_Dispatch(this_frame,mpi,intra_chroma_pred_mode,(mb_pos_x)>>1,(mb_pos_y)>>1,constrained_intra_pred_flag);//Intra Chroma prediction 

        } else if(MbPartPredMode==Intra_16x16) {  ////// Intra_16x16_Pred //////////////////
		  //printf("22:	if(mb.MbPartPredMode[0]==Intra_16x16)\n");
          int i,j;
          perf_enter("intra prediction");
          Intra_16x16_Dispatch(this_frame,mpi,Intra16x16PredMode,mb_pos_x,mb_pos_y,constrained_intra_pred_flag);//whole macroblock is predicted
          perf_enter("block entering");
		  //transform			
          transform_luma_dc(LumaDCLevel_0,&LumaACLevel[0][0],QPy);//16 luma subblock are transformed
          for(i=0; i<16; ++i) {
            int x=(mb_pos_x)+Intra4x4ScanOrder[i][0];
            int y=(mb_pos_y)+Intra4x4ScanOrder[i][1];
            enter_luma_block(&LumaACLevel[i][0],this_frame,x,y,QPy,1);
          }
		  
          perf_enter("block entering");
          Intra_Chroma_Dispatch(this_frame,mpi,intra_chroma_pred_mode,(mb_pos_x)>>1,(mb_pos_y>>1),constrained_intra_pred_flag);//Intra Chroma prediction 
          // act as if all transform blocks inside this_frame macroblock were
          // predicted using the Intra_4x4_DC prediction mode
          // (without constrained_intra_pred, we'd have to do the same for
          // inter MBs)
          for(i=0; i<4; ++i) for(j=0; j<4; ++j)
            ModePredInfo_Intra4x4PredMode(mpi,((mb_pos_x)>>2)+j,((mb_pos_y)>>2)+i)=2;
		}


     if(CodedBlockPatternChroma) { ////////////////////// Chroma Residual
 		  //printf("24:	if(mb.CodedBlockPatternChroma)\n");
          int iCbCr,i;
          perf_enter("block entering");
          for(iCbCr=0; iCbCr<2; ++iCbCr) {
            transform_chroma_dc(&ChromaDCLevel[iCbCr][0],QPc);
            for(i=0; i<4; ++i)
              ChromaACLevel[iCbCr][i][0]=ChromaDCLevel[iCbCr][i];
            for(i=0; i<4; ++i)
              enter_chroma_block(&ChromaACLevel[iCbCr][i][0],this_frame,iCbCr,
                (mb_pos_x>>1)+Intra4x4ScanOrder[i][0],
                (mb_pos_y>>1)+Intra4x4ScanOrder[i][1],
              QPc,1);
          }
        }




	}	

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void inter_prediction_module(int MbPartPredMode,frame *this_frame,frame *ref,mode_pred_info *mpi,int mb_pos_x,int mb_pos_y,int Intra4x4ScanOrder[][2],int LumaACLevel[][16],int QPy,int skip_run_mode, int mb_skip_run, int *CurrMbAddr, int MbCount,int PicWidthInMbs,int frame_no,int ChromaDCLevel[2][4],int ChromaACLevel[2][4][16],int CodedBlockPatternChroma, int QPc)
	{
	  ////////////////mb_skip_run motion compensate//////////////////
	  ///////////////////////////////////////////////////////////////	
	
	int mb_pos_x_skip,mb_pos_y_skip;
	if (skip_run_mode==1){
      for(; mb_skip_run; --mb_skip_run, ++(*CurrMbAddr)) {//++(*CurrMbAddr)
		printf ("\nCurrMbAddr = %d\n", (*CurrMbAddr));
        if((*CurrMbAddr)>=MbCount) return;
        mb_pos_x_skip=(*CurrMbAddr)%PicWidthInMbs;
        mb_pos_y_skip=(*CurrMbAddr)/PicWidthInMbs;
        ModePredInfo_MbMode(mpi,mb_pos_x_skip,mb_pos_y_skip)=P_Skip;
        mb_pos_x_skip<<=4; mb_pos_y_skip<<=4;
printf("[%d:%d] %d,%d MB: P_Skip\n",frame_no,(*CurrMbAddr),mb_pos_x_skip,mb_pos_y_skip);
        Derive_P_Skip_MVs(mpi,mb_pos_x_skip,mb_pos_y_skip);
		printf("20: skip: MotionCompensateMB\n");
        MotionCompensateMB(this_frame,ref,mpi,mb_pos_x_skip,mb_pos_y_skip);
//L_pixel(this_frame,mb_pos_x+8,mb_pos_y+8)=255;
		//cin.get();
      }
	}
	
	///////////////////////////////////////////////////////////////////////
	/*
	if (skip_run_mode==1){
		mb_pos_x_skip=(*CurrMbAddr)%PicWidthInMbs;
        mb_pos_y_skip=(*CurrMbAddr)/PicWidthInMbs;
        ModePredInfo_MbMode(mpi,mb_pos_x_skip,mb_pos_y_skip)=P_Skip;
        mb_pos_x_skip<<=4; mb_pos_y_skip<<=4;
printf("[%d:%d] %d,%d MB: P_Skip\n",frame_no,(*CurrMbAddr),mb_pos_x_skip,mb_pos_y_skip);
        Derive_P_Skip_MVs(mpi,mb_pos_x_skip,mb_pos_y_skip);
		printf("20: skip: MotionCompensateMB\n");
        MotionCompensateMB(this_frame,ref,mpi,mb_pos_x_skip,mb_pos_y_skip);
	}
	*/
	/*
	if (skip_run_mode==1){
		mb_pos_x=(CurrMbAddr)%PicWidthInMbs;
    	mb_pos_y=(CurrMbAddr)/PicWidthInMbs;
    	ModePredInfo_MbMode(mpi,mb_pos_x,mb_pos_y)=P_Skip;
    	mb_pos_x<<=4; mb_pos_y<<=4;
		printf("[%d:%d] %d,%d MB: P_Skip\n",frame_no,(CurrMbAddr),mb_pos_x,mb_pos_y);
		Derive_P_Skip_MVs(mpi,mb_pos_x,mb_pos_y);
		printf("20: skip: MotionCompensateMB\n");
    	MotionCompensateMB(this_frame,ref,mpi,mb_pos_x,mb_pos_y);
	}
	*/
	else	
		if(MbPartPredMode!=Intra_4x4 and MbPartPredMode!=Intra_16x16){
		  //printf("23:	Inter_*_Pred\n");
         int i;

//{int x,y;for(y=0;y<4;++y){for(x=0;x<4;++x){i=((mb_pos_y>>2)+y)*mpi->TbPitch+(mb_pos_x>>2)+x;
//printf("|%4d,%-4d",mpi->MVx[i],mpi->MVy[i]);}printf("\n");}}

          perf_enter("inter prediction");
          MotionCompensateMB(this_frame,ref,mpi,mb_pos_x,mb_pos_y);
          perf_enter("block entering");
          for(i=0; i<16; ++i) {
            int x=mb_pos_x+Intra4x4ScanOrder[i][0];
            int y=mb_pos_y+Intra4x4ScanOrder[i][1];
            enter_luma_block(&LumaACLevel[i][0],this_frame,x,y,QPy,0);
          }
         


     	  if(CodedBlockPatternChroma) { ////////////////////// Chroma Residual
 		  	printf("24:	if(mb.CodedBlockPatternChroma)\n");
          	int iCbCr,i;
          	perf_enter("block entering");
          	for(iCbCr=0; iCbCr<2; ++iCbCr) {
          	  transform_chroma_dc(&ChromaDCLevel[iCbCr][0],QPc);
          	  for(i=0; i<4; ++i)
          	    ChromaACLevel[iCbCr][i][0]=ChromaDCLevel[iCbCr][i];
          	  for(i=0; i<4; ++i)
          	    enter_chroma_block(&ChromaACLevel[iCbCr][i][0],this_frame,iCbCr,
          	      (mb_pos_x>>1)+Intra4x4ScanOrder[i][0],
          	      (mb_pos_y>>1)+Intra4x4ScanOrder[i][1],
          	    QPc,1);
          	}
          }

		}

	}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void decode_module_MB(slice_header *sh,seq_parameter_set *sps, pic_parameter_set *pps,mode_pred_info *mpi,int CurrMbAddr,mb_mode *mb,nal_unit *nalu,int *mb_pos_x,int *mb_pos_y,int *QPy,int *QPc, int LumaACLevel[16][16],int LumaDCLevel[16],int ChromaDCLevel[2][4],int ChromaACLevel[2][4][16],int *intra_chroma_pred_mode){

  //int CurrMbAddr=sh->first_mb_in_slice*(1+sh->MbaffFrameFlag);
  //int moreDataFlag=1;
  //int prevMbSkipped=0;
  //int MbCount=mpi->MbWidth*mpi->MbHeight;
  //int mb_skip_run;
  int mb_qp_delta;
  //int QPy,QPc;
  *intra_chroma_pred_mode=0;

  
  //mb_mode mb;
  sub_mb_mode sub[4];

  // transform coefficient levels  
  //int LumaDCLevel[16];      // === Intra16x16DCLevel
  //int LumaACLevel[16][16];  // === Intra16x16ACLevel
  //int ChromaDCLevel[2][4];
  //int ChromaACLevel[2][4][16];



	  //printf ("\nCurrMbAddr = %d\n", CurrMbAddr);
      decode_mb_mode(mb,sh->slice_type,get_unsigned_exp_golomb());
      *mb_pos_x=CurrMbAddr%sps->PicWidthInMbs;
      *mb_pos_y=CurrMbAddr/sps->PicWidthInMbs;
      ModePredInfo_MbMode(mpi,*mb_pos_x,*mb_pos_y)=mb->mb_type;
      *mb_pos_x<<=4; *mb_pos_y<<=4;
	  //printf("\n[%d:%d] %d,%d ",frame_no,CurrMbAddr,*mb_pos_x,*mb_pos_y);_dump_mb_mode(mb);

		printf("NORMAL macroblock\n");
//cin.get();
        if(mb->MbPartPredMode[0]!=Intra_4x4 &&
           mb->MbPartPredMode[0]!=Intra_16x16 &&
           mb->NumMbPart==4)
        { // sub_mb_pred() ////////////////////////////////////////////////////
		  //printf("NORMAL macroblock: sub_mb_pred() : NOT Intra_4x4 and Intra_16x16\n");
			
          int mbPartIdx,subMbPartIdx;
          for(mbPartIdx=0; mbPartIdx<4; ++mbPartIdx)
            decode_sub_mb_mode(&sub[mbPartIdx],sh->slice_type,
                               get_unsigned_exp_golomb());
{int i;printf("sub_mb_pred():");for(i=0;i<4;++i)printf(" %s(%d)",_str_sub_mb_type(sub[i].sub_mb_type),sub[i].NumSubMbPart);printf("\n");}

          // ignoring ref_idx_* and *_l1 stuff for now -- I do not support
          // long-term prediction or B-frames anyway ...
          for(mbPartIdx=0; mbPartIdx<4; ++mbPartIdx)
            if(sub[mbPartIdx].sub_mb_type!=B_Direct_8x8 &&
               sub[mbPartIdx].SubMbPredMode!=Pred_L1)
            { // SOF = "scan order factor"
              int SOF=(sub[mbPartIdx].sub_mb_type==P_L0_8x4)?2:1;
              for(subMbPartIdx=0; subMbPartIdx<sub[mbPartIdx].NumSubMbPart; ++subMbPartIdx) {
                int mvdx=get_signed_exp_golomb();
                int mvdy=get_signed_exp_golomb();
				printf ("mvdx = %d       ,          mvdy = %d\n",mvdx,mvdy);
                DeriveMVs(mpi,
                  (*mb_pos_x)+Intra4x4ScanOrder[mbPartIdx*4+subMbPartIdx*SOF][0],
                  (*mb_pos_y)+Intra4x4ScanOrder[mbPartIdx*4+subMbPartIdx*SOF][1],
                  sub[mbPartIdx].SubMbPartWidth,
                  sub[mbPartIdx].SubMbPartHeight,
                  mvdx, mvdy);
              }
            }
        } else {  // mb_pred() ////////////////////////////////////////////////
		  //printf("NORMAL macroblock: mb_pred() : Intra_4x4 or Intra_16x16 or inter\n");
          if(mb->MbPartPredMode[0]==Intra_4x4 ||
             mb->MbPartPredMode[0]==Intra_16x16)
          {  // mb_pred() for intra macroblocks ///////////////////////////////			
            if(mb->MbPartPredMode[0]==Intra_4x4) {
              int luma4x4BlkIdx;
printf("NORMAL macroblock: intra_4x4:\n");
printf("NORMAL macroblock: predIntra4x4PredMode: \n");
              for(luma4x4BlkIdx=0; luma4x4BlkIdx<16; ++luma4x4BlkIdx) {
                int predIntra4x4PredMode=get_predIntra4x4PredMode(mpi,
                      *mb_pos_x+Intra4x4ScanOrder[luma4x4BlkIdx][0],
                      *mb_pos_y+Intra4x4ScanOrder[luma4x4BlkIdx][1]);
                if(input_get_one_bit())  // prev_intra4x4_pred_mode_flag
                  Intra4x4PredMode_withPointer(luma4x4BlkIdx)=predIntra4x4PredMode;
                else {
                  int rem_intra4x4_pred_mode=input_get_bits(3);
                  if(rem_intra4x4_pred_mode<predIntra4x4PredMode)
                    Intra4x4PredMode_withPointer(luma4x4BlkIdx)=rem_intra4x4_pred_mode;
                  else
                    Intra4x4PredMode_withPointer(luma4x4BlkIdx)=rem_intra4x4_pred_mode+1;
                }
printf(" %d",Intra4x4PredMode_withPointer(luma4x4BlkIdx));

              }
printf("\n");

            }else
				printf("NORMAL macroblock: intra_16x16:\n");
		
            *intra_chroma_pred_mode=get_unsigned_exp_golomb();
			printf("intra_chroma_pred_mode = %d\n",*intra_chroma_pred_mode);
			//cin.get();
printf("NORMAL macroblock: Intra_4x4 or Intra_16x16: intra_chroma_pred_mode: %d\n",*intra_chroma_pred_mode);
          } else { // mb_pred() for inter macroblocks /////////////////////////
            int mbPartIdx;
printf("NORMAL macroblock: inter\n");       
			// ignoring ref_idx_* and *_l1 stuff for now -- I do not support
            // long-term prediction or B-frames anyway ...
            int SOF=(mb->mb_type==P_L0_L0_16x8)?8:4;
            for(mbPartIdx=0; mbPartIdx<mb->NumMbPart; ++mbPartIdx)
              if(mb->MbPartPredMode[mbPartIdx]!=Pred_L1) {
                int mvdx=get_signed_exp_golomb();
                int mvdy=get_signed_exp_golomb();			
                DeriveMVs(mpi,
                  (*mb_pos_x)+Intra4x4ScanOrder[mbPartIdx*SOF][0],
                  (*mb_pos_y)+Intra4x4ScanOrder[mbPartIdx*SOF][1],
                  mb->MbPartWidth, mb->MbPartHeight, mvdx, mvdy);
				printf("NORMAL macroblock: inter : DeriveMVs mb_part: %d  mv=(%d,%d)\n",mbPartIdx,mvdx,mvdy);				
 
             }
            }
//cin.get();
        }

        // coded_block_pattern ////////////////////////////////////////////////
        if(mb->MbPartPredMode[0]!=Intra_16x16) {
          int coded_block_pattern=get_unsigned_exp_golomb();
          if(mb->MbPartPredMode[0]==Intra_4x4)
            coded_block_pattern=CodedBlockPatternMapping_Intra4x4[coded_block_pattern];
           else
            coded_block_pattern=CodedBlockPatternMapping_Inter[coded_block_pattern];
//printf("coded_block_pattern=%d\n",coded_block_pattern);
          mb->CodedBlockPatternLuma=coded_block_pattern&15;
          mb->CodedBlockPatternChroma=coded_block_pattern>>4;
//_dump_mb_mode(&mb);
        }

        // Before parsing the residual data, set all coefficients to zero. In
        // the original H.264 documentation, this is done either in
        // residual_block() at the very beginning or by setting values to zero
        // according to the CodedBlockPattern values. So, there's only little
        // overhead if we do it right here.
        //memset(LumaDCLevel,0,sizeof(LumaDCLevel));
        //memset(LumaACLevel,0,sizeof(LumaACLevel));
        //memset(ChromaDCLevel,0,sizeof(ChromaDCLevel));
        //memset(ChromaACLevel,0,sizeof(ChromaACLevel));

		for (int i=0;i<16;i++){
			LumaDCLevel[i]=0;
			for (int j=0;j<16;j++)
				LumaACLevel[i][j]=0;
		}
 
		for (int i=0;i<2;i++)
			for (int j=0;j<4;j++){
				ChromaDCLevel[i][j]=0;
				for (int k=0;k<16;k++)
					ChromaACLevel[i][j][k]=0;
		}

  //int LumaDCLevel[16];      // === Intra16x16DCLevel
  //int LumaACLevel[16][16];  // === Intra16x16ACLevel
  //int ChromaDCLevel[2][4];
  //int 		;

        // residual() /////////////////////////////////////////////////////////
        if(mb->CodedBlockPatternLuma>0 || mb->CodedBlockPatternChroma>0 ||
           mb->MbPartPredMode[0]==Intra_16x16)
        {
          int i8x8,i4x4,iCbCr,QPi;

          mb_qp_delta=get_signed_exp_golomb();
          *QPy=(*QPy+mb_qp_delta+52)%52;
          QPi=*QPy+pps->chroma_qp_index_offset;
          QPi=CustomClip(QPi,0,51);
          if(QPi<30) *QPc=QPi;
                else *QPc=QPcTable[QPi-30];
//printf("mb_qp_delta=%d QPy=%d QPi=%d QPc=%d\n",mb_qp_delta,QPy,QPi,QPc);

          // OK, now let's parse the hell out of the stream ;)        
          if(mb->MbPartPredMode[0]==Intra_16x16)
            residual_block(&LumaDCLevel[0],16,LumaDC_nC_withPointer);
          for(i8x8=0; i8x8<4; ++i8x8)
            for(i4x4=0; i4x4<4; ++i4x4)
              if(mb->CodedBlockPatternLuma&(1<<i8x8)) {
                if(mb->MbPartPredMode[0]==Intra_16x16)
                  LumaAdjust_withPointer residual_block(&LumaACLevel[i8x8*4+i4x4][1],15,LumaAC_nC_withPointer);
                else
                  LumaAdjust_withPointer residual_block(&LumaACLevel[i8x8*4+i4x4][0],16,LumaAC_nC_withPointer);
              };
          for(iCbCr=0; iCbCr<2; iCbCr++)
            if(mb->CodedBlockPatternChroma&3)
              residual_block(&ChromaDCLevel[iCbCr][0],4,ChromaDC_nC);
          for(iCbCr=0; iCbCr<2; iCbCr++)
            for(i4x4=0; i4x4<4; ++i4x4)
              if(mb->CodedBlockPatternChroma&2)
                ChromaAdjust_withPointer residual_block(&ChromaACLevel[iCbCr][i4x4][1],15,ChromaAC_nC_withPointer);
		
	}

}
///////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

//decode_module(sh,sps,pps,mpi,CurrMbAddr,frame_no,&mb,nalu,&mb_pos_x,&mb_pos_y,&QPy,&QPc,LumaACLevel,LumaDCLevel,Intra4x4ScanOrder,ChromaDCLevel,ChromaACLevel);

void decode_module(slice_header *sh,seq_parameter_set *sps, pic_parameter_set *pps,mode_pred_info *mpi,int CurrMbAddr,int frame_no,mb_mode *mb,nal_unit *nalu,int *mb_pos_x,int *mb_pos_y,int *QPy,int *QPc, int LumaACLevel[16][16],int LumaDCLevel[16],int Intra4x4ScanOrder[16][2],int ChromaDCLevel[2][4],int ChromaACLevel[2][4][16]){

  //int CurrMbAddr=sh->first_mb_in_slice*(1+sh->MbaffFrameFlag);
  //int moreDataFlag=1;
  //int prevMbSkipped=0;
  //int MbCount=mpi->MbWidth*mpi->MbHeight;
  //int mb_skip_run;
  int mb_qp_delta;
  //int QPy,QPc;
  int intra_chroma_pred_mode=0;

  
  //mb_mode mb;
  sub_mb_mode sub[4];

  // transform coefficient levels  
  //int LumaDCLevel[16];      // === Intra16x16DCLevel
  //int LumaACLevel[16][16];  // === Intra16x16ACLevel
  //int ChromaDCLevel[2][4];
  //int ChromaACLevel[2][4][16];



	  //printf ("\nCurrMbAddr = %d\n", CurrMbAddr);
      decode_mb_mode(mb,sh->slice_type,get_unsigned_exp_golomb());
      *mb_pos_x=CurrMbAddr%sps->PicWidthInMbs;
      *mb_pos_y=CurrMbAddr/sps->PicWidthInMbs;
      ModePredInfo_MbMode(mpi,*mb_pos_x,*mb_pos_y)=mb->mb_type;
      *mb_pos_x<<=4; *mb_pos_y<<=4;
	  //printf("\n[%d:%d] %d,%d ",frame_no,CurrMbAddr,*mb_pos_x,*mb_pos_y);_dump_mb_mode(mb);

		printf("NORMAL macroblock\n");
//cin.get();
        if(mb->MbPartPredMode[0]!=Intra_4x4 &&
           mb->MbPartPredMode[0]!=Intra_16x16 &&
           mb->NumMbPart==4)
        { // sub_mb_pred() ////////////////////////////////////////////////////
		  //printf("NORMAL macroblock: sub_mb_pred() : NOT Intra_4x4 and Intra_16x16\n");
			
          int mbPartIdx,subMbPartIdx;
          for(mbPartIdx=0; mbPartIdx<4; ++mbPartIdx)
            decode_sub_mb_mode(&sub[mbPartIdx],sh->slice_type,
                               get_unsigned_exp_golomb());
{int i;printf("sub_mb_pred():");for(i=0;i<4;++i)printf(" %s(%d)",_str_sub_mb_type(sub[i].sub_mb_type),sub[i].NumSubMbPart);printf("\n");}

          // ignoring ref_idx_* and *_l1 stuff for now -- I do not support
          // long-term prediction or B-frames anyway ...
          for(mbPartIdx=0; mbPartIdx<4; ++mbPartIdx)
            if(sub[mbPartIdx].sub_mb_type!=B_Direct_8x8 &&
               sub[mbPartIdx].SubMbPredMode!=Pred_L1)
            { // SOF = "scan order factor"
              int SOF=(sub[mbPartIdx].sub_mb_type==P_L0_8x4)?2:1;
              for(subMbPartIdx=0; subMbPartIdx<sub[mbPartIdx].NumSubMbPart; ++subMbPartIdx) {
                int mvdx=get_signed_exp_golomb();
                int mvdy=get_signed_exp_golomb();
                DeriveMVs(mpi,
                  (*mb_pos_x)+Intra4x4ScanOrder[mbPartIdx*4+subMbPartIdx*SOF][0],
                  (*mb_pos_y)+Intra4x4ScanOrder[mbPartIdx*4+subMbPartIdx*SOF][1],
                  sub[mbPartIdx].SubMbPartWidth,
                  sub[mbPartIdx].SubMbPartHeight,
                  mvdx, mvdy);
              }
            }
        } else {  // mb_pred() ////////////////////////////////////////////////
		  //printf("NORMAL macroblock: mb_pred() : Intra_4x4 or Intra_16x16 or inter\n");
          if(mb->MbPartPredMode[0]==Intra_4x4 ||
             mb->MbPartPredMode[0]==Intra_16x16)
          {  // mb_pred() for intra macroblocks ///////////////////////////////			
            if(mb->MbPartPredMode[0]==Intra_4x4) {
              int luma4x4BlkIdx;
printf("NORMAL macroblock: intra_4x4:\n");
printf("NORMAL macroblock: predIntra4x4PredMode: \n");
              for(luma4x4BlkIdx=0; luma4x4BlkIdx<16; ++luma4x4BlkIdx) {
                int predIntra4x4PredMode=get_predIntra4x4PredMode(mpi,
                      *mb_pos_x+Intra4x4ScanOrder[luma4x4BlkIdx][0],
                      *mb_pos_y+Intra4x4ScanOrder[luma4x4BlkIdx][1]);
                if(input_get_one_bit())  // prev_intra4x4_pred_mode_flag
                  Intra4x4PredMode_withPointer(luma4x4BlkIdx)=predIntra4x4PredMode;
                else {
                  int rem_intra4x4_pred_mode=input_get_bits(3);
                  if(rem_intra4x4_pred_mode<predIntra4x4PredMode)
                    Intra4x4PredMode_withPointer(luma4x4BlkIdx)=rem_intra4x4_pred_mode;
                  else
                    Intra4x4PredMode_withPointer(luma4x4BlkIdx)=rem_intra4x4_pred_mode+1;
                }
printf(" %d",Intra4x4PredMode_withPointer(luma4x4BlkIdx));

              }
printf("\n");

            }else
				printf("NORMAL macroblock: intra_16x16:\n");
		
            intra_chroma_pred_mode=get_unsigned_exp_golomb();
printf("NORMAL macroblock: Intra_4x4 or Intra_16x16: intra_chroma_pred_mode: %d\n",intra_chroma_pred_mode);
          } else { // mb_pred() for inter macroblocks /////////////////////////
            int mbPartIdx;
printf("NORMAL macroblock: inter\n");       
			// ignoring ref_idx_* and *_l1 stuff for now -- I do not support
            // long-term prediction or B-frames anyway ...
            int SOF=(mb->mb_type==P_L0_L0_16x8)?8:4;
            for(mbPartIdx=0; mbPartIdx<mb->NumMbPart; ++mbPartIdx)
              if(mb->MbPartPredMode[mbPartIdx]!=Pred_L1) {
                int mvdx=get_signed_exp_golomb();
                int mvdy=get_signed_exp_golomb();			
                DeriveMVs(mpi,
                  (*mb_pos_x)+Intra4x4ScanOrder[mbPartIdx*SOF][0],
                  (*mb_pos_y)+Intra4x4ScanOrder[mbPartIdx*SOF][1],
                  mb->MbPartWidth, mb->MbPartHeight, mvdx, mvdy);
				printf("NORMAL macroblock: inter : DeriveMVs mb_part: %d  mv=(%d,%d)\n",mbPartIdx,mvdx,mvdy);				
 
             }
            }
//cin.get();
        }

        // coded_block_pattern ////////////////////////////////////////////////
        if(mb->MbPartPredMode[0]!=Intra_16x16) {
          int coded_block_pattern=get_unsigned_exp_golomb();
          if(mb->MbPartPredMode[0]==Intra_4x4)
            coded_block_pattern=CodedBlockPatternMapping_Intra4x4[coded_block_pattern];
           else
            coded_block_pattern=CodedBlockPatternMapping_Inter[coded_block_pattern];
//printf("coded_block_pattern=%d\n",coded_block_pattern);
          mb->CodedBlockPatternLuma=coded_block_pattern&15;
          mb->CodedBlockPatternChroma=coded_block_pattern>>4;
//_dump_mb_mode(&mb);
        }

        // Before parsing the residual data, set all coefficients to zero. In
        // the original H.264 documentation, this is done either in
        // residual_block() at the very beginning or by setting values to zero
        // according to the CodedBlockPattern values. So, there's only little
        // overhead if we do it right here.
        //memset(LumaDCLevel,0,sizeof(LumaDCLevel));
        //memset(LumaACLevel,0,sizeof(LumaACLevel));
        //memset(ChromaDCLevel,0,sizeof(ChromaDCLevel));
        //memset(ChromaACLevel,0,sizeof(ChromaACLevel));

		for (int i=0;i<16;i++){
			LumaDCLevel[i]=0;
			for (int j=0;j<16;j++)
				LumaACLevel[i][j]=0;
		}
 
		for (int i=0;i<2;i++)
			for (int j=0;j<4;j++){
				ChromaDCLevel[i][j]=0;
				for (int k=0;k<16;k++)
					ChromaACLevel[i][j][k]=0;
		}

  //int LumaDCLevel[16];      // === Intra16x16DCLevel
  //int LumaACLevel[16][16];  // === Intra16x16ACLevel
  //int ChromaDCLevel[2][4];
  //int 		;

        // residual() /////////////////////////////////////////////////////////
        if(mb->CodedBlockPatternLuma>0 || mb->CodedBlockPatternChroma>0 ||
           mb->MbPartPredMode[0]==Intra_16x16)
        {
          int i8x8,i4x4,iCbCr,QPi;

          mb_qp_delta=get_signed_exp_golomb();
          *QPy=(*QPy+mb_qp_delta+52)%52;
          QPi=*QPy+pps->chroma_qp_index_offset;
          QPi=CustomClip(QPi,0,51);
          if(QPi<30) *QPc=QPi;
                else *QPc=QPcTable[QPi-30];
//printf("mb_qp_delta=%d QPy=%d QPi=%d QPc=%d\n",mb_qp_delta,QPy,QPi,QPc);

          // OK, now let's parse the hell out of the stream ;)        
          if(mb->MbPartPredMode[0]==Intra_16x16)
            residual_block(&LumaDCLevel[0],16,LumaDC_nC_withPointer);
          for(i8x8=0; i8x8<4; ++i8x8)
            for(i4x4=0; i4x4<4; ++i4x4)
              if(mb->CodedBlockPatternLuma&(1<<i8x8)) {
                if(mb->MbPartPredMode[0]==Intra_16x16)
                  LumaAdjust_withPointer residual_block(&LumaACLevel[i8x8*4+i4x4][1],15,LumaAC_nC_withPointer);
                else
                  LumaAdjust_withPointer residual_block(&LumaACLevel[i8x8*4+i4x4][0],16,LumaAC_nC_withPointer);
              };
          for(iCbCr=0; iCbCr<2; iCbCr++)
            if(mb->CodedBlockPatternChroma&3)
              residual_block(&ChromaDCLevel[iCbCr][0],4,ChromaDC_nC);
          for(iCbCr=0; iCbCr<2; iCbCr++)
            for(i4x4=0; i4x4<4; ++i4x4)
              if(mb->CodedBlockPatternChroma&2)
                ChromaAdjust_withPointer residual_block(&ChromaACLevel[iCbCr][i4x4][1],15,ChromaAC_nC_withPointer);
		
	}

}
///////////////////////////////////////////////////////////////////////////////
//noc related
void decode_MicrobBlock(slice_header *sh,seq_parameter_set *sps, pic_parameter_set *pps,mode_pred_info *mpi,int CurrMbAddr,int frame_no,mb_mode *mb,nal_unit *nalu,int *mb_pos_x,int *mb_pos_y,int *QPy,int *QPc, int LumaACLevel[16][16],int LumaDCLevel[16],int Intra4x4ScanOrder[16][2],int ChromaDCLevel[2][4],int ChromaACLevel[2][4][16]){

  		int prevMbSkipped=0;
		int mb_skip_run;//it should be defined as private variable and should be updated each time

		
	   if(sh->slice_type!=I_SLICE && sh->slice_type!=SI_SLICE) {
	      mb_skip_run=get_unsigned_exp_golomb();
	      prevMbSkipped=(mb_skip_run>0);			  
	   }
	   if (mb_skip_run!=0){
			printf("generate event(inter_prediction_module: p_skip) \n");
			printf("mb_skip_run= %d\n",mb_skip_run);
			mb_skip_run--;
	   }
		 // inter_prediction_module(mb.MbPartPredMode[0],this_frame,ref,mpi,mb_pos_x,mb_pos_y,Intra4x4ScanOrder,LumaACLevel,QPy,1, mb_skip_run, &CurrMbAddr, MbCount,sps->PicWidthInMbs,frame_no,ChromaDCLevel,ChromaACLevel,mb.CodedBlockPatternChroma,QPc);
      
	

		
		else{//not p_skip
			printf("event(decode_module)\n");
			///decode_module(sh,sps,pps,mpi,CurrMbAddr,frame_no,&mb,nalu,&mb_pos_x,&mb_pos_y,&QPy,&QPc,LumaACLevel,LumaDCLevel,Intra4x4ScanOrder,ChromaDCLevel,ChromaACLevel);
    	   	///if ((mb.MbPartPredMode[0]==Intra_4x4) or (mb.MbPartPredMode[0]==Intra_16x16))
			///	printf("generate event(intra_prediction_module) \n");
				//intra_prediction_module(mb.MbPartPredMode[0],mb_pos_x,mb_pos_y,Intra4x4ScanOrder,this_frame,mpi,QPy,LumaACLevel,intra_chroma_pred_mode,pps->constrained_intra_pred_flag,mb.Intra16x16PredMode,&LumaDCLevel[0],ChromaDCLevel,ChromaACLevel,mb.CodedBlockPatternChroma,QPc);

			///if  ((mb.MbPartPredMode[0]!=Intra_4x4) and (mb.MbPartPredMode[0]!=Intra_16x16))
				///printf("generate event(inter_prediction_module: normal) \n");
				//inter_prediction_module(mb.MbPartPredMode[0],this_frame,ref,mpi,mb_pos_x,mb_pos_y,Intra4x4ScanOrder,LumaACLevel,QPy,0,0, &CurrMbAddr, MbCount,sps->PicWidthInMbs,frame_no,ChromaDCLevel, ChromaACLevel,mb.CodedBlockPatternChroma,QPc);
		}
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void decode_slice_data_loop(slice_header *sh,
                       seq_parameter_set *sps, pic_parameter_set *pps,
                       nal_unit *nalu,
                       frame *this_frame, frame *ref,
                       mode_pred_info *mpi) {

  int CurrMbAddr=sh->first_mb_in_slice*(1+sh->MbaffFrameFlag);
  int moreDataFlag=1;
  int prevMbSkipped=0;
  int MbCount=mpi->MbWidth*mpi->MbHeight;
  int mb_skip_run;
  int mb_qp_delta;
  int QPy,QPc;
  int intra_chroma_pred_mode=0;

  int mb_pos_x,mb_pos_y;
  mb_mode mb;
  sub_mb_mode sub[4];

  // transform coefficient levels  
  int LumaDCLevel[16];      // === Intra16x16DCLevel
  int LumaACLevel[16][16];  // === Intra16x16ACLevel
  int ChromaDCLevel[2][4];
  int ChromaACLevel[2][4][16];

#if 0
  // clear the frame -- this_frame is only useful for debugging purposes, it may
  // safely be disabled later
  memset(this_frame->L,0,this_frame->Lheight*this_frame->Lpitch);
  memset(this_frame->C[0],128,this_frame->Cheight*this_frame->Cpitch);
  memset(this_frame->C[1],128,this_frame->Cheight*this_frame->Cpitch);
#endif

  // initialize some values
  clear_mode_pred_info(mpi);
  QPy=sh->SliceQPy;
  QPc=QPy;  // only to prevent a warning

   //read nalu MODULE
  moreDataFlag=more_rbsp_data(nalu);
  while( CurrMbAddr<MbCount) {//moreDataFlag &&
  //while (CurrMbAddr<MbCount){	
	printf ("\nCurrMbAddr = %d\n", CurrMbAddr);
	//cin.get();
    // mb_skip_run ////////////////////////////////////////////////////////////
    if(sh->slice_type!=I_SLICE && sh->slice_type!=SI_SLICE) {
      mb_skip_run=get_unsigned_exp_golomb();
      prevMbSkipped=(mb_skip_run>0);
      for(; mb_skip_run; --mb_skip_run, ++CurrMbAddr) {
        if(CurrMbAddr>=MbCount) return;
        mb_pos_x=CurrMbAddr%sps->PicWidthInMbs;
        mb_pos_y=CurrMbAddr/sps->PicWidthInMbs;
        ModePredInfo_MbMode(mpi,mb_pos_x,mb_pos_y)=P_Skip;
        mb_pos_x<<=4; mb_pos_y<<=4;
//printf("\n[%d:%d] %d,%d MB: P_Skip\n",frame_no,CurrMbAddr,mb_pos_x,mb_pos_y);
        Derive_P_Skip_MVs(mpi,mb_pos_x,mb_pos_y);
        MotionCompensateMB(this_frame,ref,mpi,mb_pos_x,mb_pos_y);
//L_pixel(this,mb_pos_x+8,mb_pos_y+8)=255;
      }
      //moreDataFlag=more_rbsp_data(nalu);
    }
    if(CurrMbAddr>=MbCount) return;

    if(1){//moreDataFlag) {  // macroblock_layer
      decode_mb_mode(&mb,sh->slice_type,get_unsigned_exp_golomb());
      mb_pos_x=CurrMbAddr%sps->PicWidthInMbs;
      mb_pos_y=CurrMbAddr/sps->PicWidthInMbs;
      ModePredInfo_MbMode(mpi,mb_pos_x,mb_pos_y)=mb.mb_type;
      mb_pos_x<<=4; mb_pos_y<<=4;
//printf("\n[%d:%d] %d,%d ",frame_no,CurrMbAddr,mb_pos_x,mb_pos_y);_dump_mb_mode(&mb);
      
      if(mb.mb_type==I_PCM) {  // I_PCM macroblock ////////////////////////////
        int x,y,iCbCr;
        unsigned char *pos;        
        input_align_to_next_byte();
        pos=&L_pixel(this_frame,mb_pos_x,mb_pos_y);
        for(y=16; y; --y) {
          for(x=16; x; --x)
            *pos++=input_get_byte();
          pos+=this_frame->Lpitch-16;
        }
        for(iCbCr=0; iCbCr<2; ++iCbCr) {
          pos=&C_pixel(this_frame,iCbCr,mb_pos_x>>1,mb_pos_y>>1);
          for(y=8; y; --y) {
            for(x=8; x; --x)
              *pos++=input_get_byte();
            pos+=this_frame->Cpitch-8;
          }
        }
        // fix mode_pred_info->TotalCoeff data
        for(y=0; y<4; ++y)
          for(x=0; x<4; ++x)
            ModePredInfo_TotalCoeffL(mpi,(mb_pos_x>>2)+x,(mb_pos_y>>2)+y)=16;
        for(y=0; y<2; ++y)
          for(x=0; x<2; ++x) {
            ModePredInfo_TotalCoeffC(mpi,(mb_pos_x>>3)+x,(mb_pos_y>>3)+y,0)=16;
            ModePredInfo_TotalCoeffC(mpi,(mb_pos_x>>3)+x,(mb_pos_y>>3)+y,1)=16;
          }
      } else {  // "normal" macroblock ////////////////////////////////////////

        if(mb.MbPartPredMode[0]!=Intra_4x4 &&
           mb.MbPartPredMode[0]!=Intra_16x16 &&
           mb.NumMbPart==4)
        { // sub_mb_pred() ////////////////////////////////////////////////////
          int mbPartIdx,subMbPartIdx;
          for(mbPartIdx=0; mbPartIdx<4; ++mbPartIdx)
            decode_sub_mb_mode(&sub[mbPartIdx],sh->slice_type,
                               get_unsigned_exp_golomb());
//{int i;printf("sub_mb_pred():");for(i=0;i<4;++i)printf(" %s(%d)",_str_sub_mb_type(sub[i].sub_mb_type),sub[i].NumSubMbPart);printf("\n");}
          // ignoring ref_idx_* and *_l1 stuff for now -- I do not support
          // long-term prediction or B-frames anyway ...
          for(mbPartIdx=0; mbPartIdx<4; ++mbPartIdx)
            if(sub[mbPartIdx].sub_mb_type!=B_Direct_8x8 &&
               sub[mbPartIdx].SubMbPredMode!=Pred_L1)
            { // SOF = "scan order factor"
              int SOF=(sub[mbPartIdx].sub_mb_type==P_L0_8x4)?2:1;
              for(subMbPartIdx=0; subMbPartIdx<sub[mbPartIdx].NumSubMbPart; ++subMbPartIdx) {
                int mvdx=get_signed_exp_golomb();
                int mvdy=get_signed_exp_golomb();
                DeriveMVs(mpi,
                  mb_pos_x+Intra4x4ScanOrder[mbPartIdx*4+subMbPartIdx*SOF][0],
                  mb_pos_y+Intra4x4ScanOrder[mbPartIdx*4+subMbPartIdx*SOF][1],
                  sub[mbPartIdx].SubMbPartWidth,
                  sub[mbPartIdx].SubMbPartHeight,
                  mvdx, mvdy);
              }
            }
        } else {  // mb_pred() ////////////////////////////////////////////////
          if(mb.MbPartPredMode[0]==Intra_4x4 ||
             mb.MbPartPredMode[0]==Intra_16x16)
          {  // mb_pred() for intra macroblocks ///////////////////////////////
            if(mb.MbPartPredMode[0]==Intra_4x4) {
              int luma4x4BlkIdx;
//printf("intra_4x4:");
              for(luma4x4BlkIdx=0; luma4x4BlkIdx<16; ++luma4x4BlkIdx) {
                int predIntra4x4PredMode=get_predIntra4x4PredMode(mpi,
                      mb_pos_x+Intra4x4ScanOrder[luma4x4BlkIdx][0],
                      mb_pos_y+Intra4x4ScanOrder[luma4x4BlkIdx][1]);
                if(input_get_one_bit())  // prev_intra4x4_pred_mode_flag
                  Intra4x4PredMode(luma4x4BlkIdx)=predIntra4x4PredMode;
                else {
                  int rem_intra4x4_pred_mode=input_get_bits(3);
                  if(rem_intra4x4_pred_mode<predIntra4x4PredMode)
                    Intra4x4PredMode(luma4x4BlkIdx)=rem_intra4x4_pred_mode;
                  else
                    Intra4x4PredMode(luma4x4BlkIdx)=rem_intra4x4_pred_mode+1;
                }
//printf(" %d",Intra4x4PredMode(luma4x4BlkIdx));
              }
//printf("\n");
            }
            intra_chroma_pred_mode=get_unsigned_exp_golomb();
//printf("intra_chroma_pred_mode: %d\n",intra_chroma_pred_mode);
          } else { // mb_pred() for inter macroblocks /////////////////////////
            int mbPartIdx;
            // ignoring ref_idx_* and *_l1 stuff for now -- I do not support
            // long-term prediction or B-frames anyway ...
            int SOF=(mb.mb_type==P_L0_L0_16x8)?8:4;
            for(mbPartIdx=0; mbPartIdx<mb.NumMbPart; ++mbPartIdx)
              if(mb.MbPartPredMode[mbPartIdx]!=Pred_L1) {
                int mvdx=get_signed_exp_golomb();
                int mvdy=get_signed_exp_golomb();
                DeriveMVs(mpi,
                  mb_pos_x+Intra4x4ScanOrder[mbPartIdx*SOF][0],
                  mb_pos_y+Intra4x4ScanOrder[mbPartIdx*SOF][1],
                  mb.MbPartWidth, mb.MbPartHeight, mvdx, mvdy);
              }
            }
        }

        // coded_block_pattern ////////////////////////////////////////////////
        if(mb.MbPartPredMode[0]!=Intra_16x16) {
          int coded_block_pattern=get_unsigned_exp_golomb();
          if(mb.MbPartPredMode[0]==Intra_4x4)
            coded_block_pattern=CodedBlockPatternMapping_Intra4x4[coded_block_pattern];
          else
            coded_block_pattern=CodedBlockPatternMapping_Inter[coded_block_pattern];
//printf("coded_block_pattern=%d\n",coded_block_pattern);
          mb.CodedBlockPatternLuma=coded_block_pattern&15;
          mb.CodedBlockPatternChroma=coded_block_pattern>>4;
//_dump_mb_mode(&mb);
        }

        // Before parsing the residual data, set all coefficients to zero. In
        // the original H.264 documentation, this is done either in
        // residual_block() at the very beginning or by setting values to zero
        // according to the CodedBlockPattern values. So, there's only little
        // overhead if we do it right here.
        memset(LumaDCLevel,0,sizeof(LumaDCLevel));
        memset(LumaACLevel,0,sizeof(LumaACLevel));
        memset(ChromaDCLevel,0,sizeof(ChromaDCLevel));
        memset(ChromaACLevel,0,sizeof(ChromaACLevel));

        // residual() /////////////////////////////////////////////////////////
        if(mb.CodedBlockPatternLuma>0 || mb.CodedBlockPatternChroma>0 ||
           mb.MbPartPredMode[0]==Intra_16x16)
        {
          int i8x8,i4x4,iCbCr,QPi;

          mb_qp_delta=get_signed_exp_golomb();
          QPy=(QPy+mb_qp_delta+52)%52;
          QPi=QPy+pps->chroma_qp_index_offset;
          QPi=CustomClip(QPi,0,51);
          if(QPi<30) QPc=QPi;
                else QPc=QPcTable[QPi-30];
//printf("mb_qp_delta=%d QPy=%d QPi=%d QPc=%d\n",mb_qp_delta,QPy,QPi,QPc);

          // OK, now let's parse the hell out of the stream ;)        
          if(mb.MbPartPredMode[0]==Intra_16x16)
            residual_block(&LumaDCLevel[0],16,LumaDC_nC);
          for(i8x8=0; i8x8<4; ++i8x8)
            for(i4x4=0; i4x4<4; ++i4x4)
              if(mb.CodedBlockPatternLuma&(1<<i8x8)) {
                if(mb.MbPartPredMode[0]==Intra_16x16)
                  LumaAdjust residual_block(&LumaACLevel[i8x8*4+i4x4][1],15,LumaAC_nC);
                else
                  LumaAdjust residual_block(&LumaACLevel[i8x8*4+i4x4][0],16,LumaAC_nC);
              };
          for(iCbCr=0; iCbCr<2; iCbCr++)
            if(mb.CodedBlockPatternChroma&3)
              residual_block(&ChromaDCLevel[iCbCr][0],4,ChromaDC_nC);
          for(iCbCr=0; iCbCr<2; iCbCr++)
            for(i4x4=0; i4x4<4; ++i4x4)
              if(mb.CodedBlockPatternChroma&2)
                ChromaAdjust residual_block(&ChromaACLevel[iCbCr][i4x4][1],15,ChromaAC_nC);
/*
{ int i; //printf("L:"); for(i=0; i<16; ++i) //printf(" %d",LumaDCLevel[i]); //printf("\n");
for(i8x8=0; i8x8<16; ++i8x8) { //printf("  [%2d]",i8x8);
for(i=0; i<16; ++i) //printf(" %d",LumaACLevel[i8x8][i]); //printf("\n"); }
printf("Cb:"); for(i=0; i<4; ++i) //printf(" %d",ChromaDCLevel[0][i]); //printf("\n");
for(i8x8=0; i8x8<4; ++i8x8) { //printf("  [%d]",i8x8);
for(i=0; i<16; ++i) //printf(" %d",ChromaACLevel[0][i8x8][i]); //printf("\n"); }
printf("Cr:"); for(i=0; i<4; ++i) //printf(" %d",ChromaDCLevel[1][i]); //printf("\n");
for(i8x8=0; i8x8<4; ++i8x8) { //printf("  [%d]",i8x8);
for(i=0; i<16; ++i) //printf(" %d",ChromaACLevel[1][i8x8][i]); //printf("\n"); } }
*/
        }

        //////////////////////////// RENDERING ////////////////////////////////
        // Now that we have all the informations needed about this macroblock,
        // we can go ahead and really render it.

        if(mb.MbPartPredMode[0]==Intra_4x4) {  ///////////////// Intra_4x4_Pred
          int i;
          for(i=0; i<16; ++i) {
            int x=mb_pos_x+Intra4x4ScanOrder[i][0];
            int y=mb_pos_y+Intra4x4ScanOrder[i][1];
            perf_enter("intra prediction");
            Intra_4x4_Dispatch(this_frame,mpi,x,y,i);
            perf_enter("block entering");
            enter_luma_block(&LumaACLevel[i][0],this_frame,x,y,QPy,0);
          }
          Intra_Chroma_Dispatch(this_frame,mpi,intra_chroma_pred_mode,mb_pos_x>>1,mb_pos_y>>1,pps->constrained_intra_pred_flag);
        } else if(mb.MbPartPredMode[0]==Intra_16x16) {  ////// Intra_16x16_Pred
          int i,j;
          perf_enter("intra prediction");
          Intra_16x16_Dispatch(this_frame,mpi,mb.Intra16x16PredMode,mb_pos_x,mb_pos_y,pps->constrained_intra_pred_flag);
          perf_enter("block entering");
          transform_luma_dc(&LumaDCLevel[0],&LumaACLevel[0][0],QPy);
          for(i=0; i<16; ++i) {
            int x=mb_pos_x+Intra4x4ScanOrder[i][0];
            int y=mb_pos_y+Intra4x4ScanOrder[i][1];
            enter_luma_block(&LumaACLevel[i][0],this_frame,x,y,QPy,1);
          }
          perf_enter("block entering");
          Intra_Chroma_Dispatch(this_frame,mpi,intra_chroma_pred_mode,mb_pos_x>>1,mb_pos_y>>1,pps->constrained_intra_pred_flag);
          // act as if all transform blocks inside this macroblock were
          // predicted using the Intra_4x4_DC prediction mode
          // (without constrained_intra_pred, we'd have to do the same for
          // inter MBs)
          for(i=0; i<4; ++i) for(j=0; j<4; ++j)
            ModePredInfo_Intra4x4PredMode(mpi,(mb_pos_x>>2)+j,(mb_pos_y>>2)+i)=2;
        } else { ///////////////////////////////////////////////// Inter_*_Pred
          int i;
/*
{int x,y;for(y=0;y<4;++y){for(x=0;x<4;++x){i=((mb_pos_y>>2)+y)*mpi->TbPitch+(mb_pos_x>>2)+x;
printf("|%4d,%-4d",mpi->MVx[i],mpi->MVy[i]);}printf("\n");}}
*/
          perf_enter("inter prediction");
          MotionCompensateMB(this_frame,ref,mpi,mb_pos_x,mb_pos_y);
          perf_enter("block entering");
          for(i=0; i<16; ++i) {
            int x=mb_pos_x+Intra4x4ScanOrder[i][0];
            int y=mb_pos_y+Intra4x4ScanOrder[i][1];
            enter_luma_block(&LumaACLevel[i][0],this_frame,x,y,QPy,0);
          }
        } /*else
          //printf("unsupported prediction mode at %d,%d!\n",mb_pos_x,mb_pos_y);*/

        if(mb.CodedBlockPatternChroma) { ////////////////////// Chroma Residual
          int iCbCr,i;
          perf_enter("block entering");
          for(iCbCr=0; iCbCr<2; ++iCbCr) {
            transform_chroma_dc(&ChromaDCLevel[iCbCr][0],QPc);
            for(i=0; i<4; ++i)
              ChromaACLevel[iCbCr][i][0]=ChromaDCLevel[iCbCr][i];
            for(i=0; i<4; ++i)
              enter_chroma_block(&ChromaACLevel[iCbCr][i][0],this_frame,iCbCr,
                (mb_pos_x>>1)+Intra4x4ScanOrder[i][0],
                (mb_pos_y>>1)+Intra4x4ScanOrder[i][1],
              QPc,1);
          }
        }

      }

}//if moredataflag

/*
		decode_module(sh,sps,pps,mpi,CurrMbAddr,frame_no,&mb,nalu,&mb_pos_x,&mb_pos_y,&QPy,&QPc,LumaACLevel,LumaDCLevel,Intra4x4ScanOrder,ChromaDCLevel,ChromaACLevel);
       	if ((mb.MbPartPredMode[0]==Intra_4x4) or (mb.MbPartPredMode[0]==Intra_16x16))
			intra_prediction_module(mb.MbPartPredMode[0],mb_pos_x,mb_pos_y,Intra4x4ScanOrder,this_frame,mpi,QPy,LumaACLevel,intra_chroma_pred_mode,pps->constrained_intra_pred_flag,mb.Intra16x16PredMode,&LumaDCLevel[0],ChromaDCLevel,ChromaACLevel,mb.CodedBlockPatternChroma,QPc);

		if  ((mb.MbPartPredMode[0]!=Intra_4x4) and (mb.MbPartPredMode[0]!=Intra_16x16))
			inter_prediction_module(mb.MbPartPredMode[0],this_frame,ref,mpi,mb_pos_x,mb_pos_y,Intra4x4ScanOrder,LumaACLevel,QPy,0,0, &CurrMbAddr, MbCount,sps->PicWidthInMbs,frame_no,ChromaDCLevel, ChromaACLevel,mb.CodedBlockPatternChroma,QPc);


    } ///////////// end of macroblock_layer() /////////////////////////////////
*/	
	//DISPLAY MODULE
	++CurrMbAddr;


    moreDataFlag=more_rbsp_data(nalu);
    
  } /* while(moreDataFlag && CurrMbAddr<=MbCount) */
}
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
void decode_slice_data(slice_header *sh,
                       seq_parameter_set *sps, pic_parameter_set *pps,
                       nal_unit *nalu,
                       frame *this_frame, frame *ref,
                       mode_pred_info *mpi,int CurrMbAddr) {
//static int first_run_flag=1;
//if (first_run_flag){
  //int CurrMbAddr=sh->first_mb_in_slice*(1+sh->MbaffFrameFlag);
  int moreDataFlag=1;
  int prevMbSkipped=0;
  int MbCount=mpi->MbWidth*mpi->MbHeight;
  static int mb_skip_run=0;
  int mb_qp_delta;
  static int QPy,QPc;
  int intra_chroma_pred_mode=0;
  static int mb_skip_run_read=1;
  int mb_pos_x,mb_pos_y;
  mb_mode mb;
  sub_mb_mode sub[4];

  // transform coefficient levels  
  int LumaDCLevel[16];      // === Intra16x16DCLevel
  int LumaACLevel[16][16];  // === Intra16x16ACLevel
  int ChromaDCLevel[2][4];
  int ChromaACLevel[2][4][16];
if (CurrMbAddr==0){
  QPy=sh->SliceQPy;
  QPc=QPy;  // only to prevent a warning
}
	printf ("\nCurrMbAddr = %d\n", CurrMbAddr);

if (mb_skip_run_read==1){
	   if(sh->slice_type!=I_SLICE && sh->slice_type!=SI_SLICE) {
			if (mb_skip_run==0)		
		      mb_skip_run=get_unsigned_exp_golomb();	        	
			printf("CurrMbAddr= %d ,sh->slice_type=%d ,mb_skip_run= %d\n"	, CurrMbAddr,sh->slice_type,mb_skip_run);
			//cin.get();		
		    if (mb_skip_run!=0){
				mb_skip_run_read=1;
				mb_skip_run--;

				
				inter_prediction_module_MB(mb.MbPartPredMode[0],this_frame,ref,mpi,mb_pos_x,mb_pos_y,LumaACLevel,QPy,1, CurrMbAddr,sps->PicWidthInMbs,ChromaDCLevel,ChromaACLevel,mb.CodedBlockPatternChroma,QPc);

			  	//inter_prediction_module(mb.MbPartPredMode[0],this_frame,ref,mpi,mb_pos_x,mb_pos_y,Intra4x4ScanOrder,LumaACLevel,QPy,1,mb_skip_run , CurrMbAddr, MbCount,sps->PicWidthInMbs,frame_no,ChromaDCLevel,ChromaACLevel,mb.CodedBlockPatternChroma,QPc);//second 1 should be mb_skip_run

				if (mb_skip_run==0)
					mb_skip_run_read=0;
				
				return;
			}
	  }		
}		
		
		decode_module_MB(sh,sps,pps,mpi,CurrMbAddr,&mb,nalu,&mb_pos_x,&mb_pos_y,&QPy,&QPc,LumaACLevel,LumaDCLevel,ChromaDCLevel,ChromaACLevel,&intra_chroma_pred_mode);

		//decode_module(sh,sps,pps,mpi,CurrMbAddr,frame_no,&mb,nalu,&mb_pos_x,&mb_pos_y,&QPy,&QPc,LumaACLevel,LumaDCLevel,Intra4x4ScanOrder,ChromaDCLevel,ChromaACLevel);
       	if ((mb.MbPartPredMode[0]==Intra_4x4) or (mb.MbPartPredMode[0]==Intra_16x16))
			intra_prediction_module(mb.MbPartPredMode[0],mb_pos_x,mb_pos_y,Intra4x4ScanOrder,this_frame,mpi,QPy,LumaACLevel,intra_chroma_pred_mode,pps->constrained_intra_pred_flag,mb.Intra16x16PredMode,&LumaDCLevel[0],ChromaDCLevel,ChromaACLevel,mb.CodedBlockPatternChroma,QPc);

		if  ((mb.MbPartPredMode[0]!=Intra_4x4) and (mb.MbPartPredMode[0]!=Intra_16x16))
			inter_prediction_module(mb.MbPartPredMode[0],this_frame,ref,mpi,mb_pos_x,mb_pos_y,Intra4x4ScanOrder,LumaACLevel,QPy,0,0, &CurrMbAddr, MbCount,sps->PicWidthInMbs,frame_no,ChromaDCLevel, ChromaACLevel,mb.CodedBlockPatternChroma,QPc);

mb_skip_run_read=1;

	//++CurrMbAddr;

}


#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <getopt.h>
#include <SDL/SDL.h>
#include "h264.h"
#include "perf.h"
#include "iostream"
#include "slicehdr.h"
#include "mode_pred.h"
#include "mbmodes.h"
#include "cavlc.h"
#include "slice.h"
#include "modules.h"

using namespace std;

//TODO //HCK
mode_pred_info_MB my_mpi_MB;
int my_mb_pos_x, my_mb_pos_y;
int update_intra_mpi_in_decode_MB_flag=0;
int update_inter_mpi_in_decode_MB_flag=0;
/////////////////////////////////////////////
void obj_get_nalu_PE::get_nalu(bool first_run_flag)
{
	int info;			
	if(first_run_flag){
		info=my_h264_open(_filename,&_sps,&_pps);//get sps, pps
		cout << "filename= " <<_filename<<endl;
		if(!info) {printf("ERROR: no info\n"); return;}
		_width=H264_WIDTH(info);
		_height=H264_HEIGHT(info);
		_nalu=get_nalu_func();
		printf("Width= %d Height=%d\n",_width,_height);
		printf("I got nalu and stored sps and pps\n");			
	}	
	else{
		_nalu=get_nalu_func();
		printf("I got nalu\n");
	}
}

//////////////////////////////////////////////////
void obj_decode_hdr_PE::run_decode(){//for NOC
	slice_header sh;		
	decode_hdr(_nalu,&sh,_sps,_pps);
	_sh=sh;

	printf("run_decode ended successfully\n");
	
}

//////////////////////////////////////////////////
void obj_decode_hdr_PE::decode(int first_run_flag,int picWidth,int picHeight,nal_unit nalu,seq_parameter_set sps,pic_parameter_set pps) {
	slice_header sh;
	if(first_run_flag){
		printf ("picWidth= %d   picHeight= %d\n",picWidth,picHeight);
		_width=picWidth;
		_height=picHeight;
		_sps=sps;
		_pps=pps;			
		decode_hdr(nalu,&sh,_sps,_pps);
		_sh=sh;	
		_MBcount = (picWidth>>4)*(picHeight>>4);
		printf("MBcount = %d\n",_MBcount);
		printf("decod_hdr first run\n");
		//generate event_decode_MB for MBcount = width>>4 * height>>4		
	}else{
		decode_hdr(nalu,&sh,_sps,_pps);
		_sh=sh;			
		printf("decod_hdr\n");				}
}	
////////////////////////////////////////////////////////////////
bool obj_decode_MB_PE::run_decode_MicroBlock(){
bool first_run_flag;	
	_CurrMbAddr++;
	if (_CurrMbAddr<_MbCount){
		if (_CurrMbAddr==0)
			first_run_flag=true;
		else first_run_flag=false;
		decode_MicroBlock(first_run_flag,_sh,
                       _sps,_pps,
                       _nalu,_CurrMbAddr);

		printf("run_decode_MicroBlock ended successfully MB:%d\n",_CurrMbAddr);
		return true;
	}
	else{
		_CurrMbAddr=-1;
		return false;
	}	
}

////////////////////////////////////////////////////////////////
void obj_decode_MB_PE::decode_MicroBlock(int first_run_flag, slice_header sh,
                       seq_parameter_set sps, pic_parameter_set pps,
                       nal_unit nalu, int CurrMbAddr)
{
	_QPy=sh.SliceQPy;
	_QPc=_QPy;  // only to prevent a warning

	_CurrMbAddr=CurrMbAddr;

	if (first_run_flag){
		clear_mode_pred_info(_mpi);
		_mb_skip_read_permission=1;
		//dump_mpi(_mpi);
		//cin.get();
		printf ("decode_MB_PE first run\n");
			
		if (_mb_skip_read_permission){
		   if(_sh.slice_type!=I_SLICE && _sh.slice_type!=SI_SLICE) {
				if (_mb_skip_run==0)		
					_mb_skip_run=get_unsigned_exp_golomb();	        	
				printf("CurrMbAddr= %d ,sh->slice_type=%d ,mb_skip_run= %d\n",_CurrMbAddr,_sh.slice_type,_mb_skip_run);
			    if (_mb_skip_run!=0){
					_mb_skip_run--;
					_trigger_event=TRIGGER_INTER_PRED_PSKIP;
					printf ("generate inter prediction: p-skip\n");
					if (_mb_skip_run==0)
						_mb_skip_read_permission=0;					
					return;
				}// if (mb_skip_run
			}//	if(sh->slice_type	
		}// if mb_skip_read_permission
		
		decode_module_MB(&_sh,&_sps,&_pps,_mpi,_CurrMbAddr,&_mb,&_nalu,&_mb_pos_x,&_mb_pos_y,&_QPy,&_QPc,_LumaACLevel,_LumaDCLevel,_ChromaDCLevel,_ChromaACLevel,&_intra_chroma_pred_mode);//& TODO

       	if ((_mb.MbPartPredMode[0]==Intra_4x4) or (_mb.MbPartPredMode[0]==Intra_16x16)){
			//current_event=INTRA_PRED_NORMAL;
			_trigger_event=TRIGGER_INTRA_PRED;
			printf ("generate intra prediction:\n");
		}
		if  ((_mb.MbPartPredMode[0]!=Intra_4x4) and (_mb.MbPartPredMode[0]!=Intra_16x16)){
			//current_event=INTER_PRED;
			_trigger_event=TRIGGER_INTER_PRED_NORMAL;
			printf ("generate inter prediction:\n");
		}
		_mb_skip_read_permission=1;
	}

	else{ // other runs (not first)

		//HCK TODO
		if (update_intra_mpi_in_decode_MB_flag){
			_mpi->MbMode[(_mb_pos_x/16)+(_mb_pos_y/16)*(_mpi->MbWidth)] = my_mpi_MB.MbMode;
		    for (int i=(my_mb_pos_y/4); i<(my_mb_pos_y/4)+4; i++)
    	    	        for (int j=(my_mb_pos_x/4); j<(my_mb_pos_x/4)+4; j++){
    	     		       _mpi->Intra4x4PredMode[j+i*_mpi->TbWidth]=my_mpi_MB.Intra4x4PredMode[(j-my_mb_pos_x/4)+(i-my_mb_pos_y/4)*4]; 
				}
			update_intra_mpi_in_decode_MB_flag=0;
		}		
		if (update_inter_mpi_in_decode_MB_flag){		
		    for (int i=(my_mb_pos_y/4); i<(my_mb_pos_y/4)+4; i++)
    	    	for (int j=(my_mb_pos_x/4); j<(my_mb_pos_x/4)+4; j++){
 					_mpi->MVx[j+i*_mpi->TbWidth]=my_mpi_MB.MVx[(j-my_mb_pos_x/4)+(i-my_mb_pos_y/4)*4];
					_mpi->MVy[j+i*_mpi->TbWidth]=my_mpi_MB.MVy[(j-my_mb_pos_x/4)+(i-my_mb_pos_y/4)*4];
				}
			update_inter_mpi_in_decode_MB_flag=0;
		}
		///////////////////////////////////////

		if (_mb_skip_read_permission){
		   if(_sh.slice_type!=I_SLICE && _sh.slice_type!=SI_SLICE) {
				if (_mb_skip_run==0)									
					_mb_skip_run=get_unsigned_exp_golomb();	        	
				
			    if (_mb_skip_run!=0){
					//_mb_skip_read_permission=1;
					_mb_skip_run--;
					//current_event=INTER_PRED_PSKIP;
					_trigger_event=TRIGGER_INTER_PRED_PSKIP;
					printf ("generate inter prediction: p-skip\n");
					if (_mb_skip_run==0)
						_mb_skip_read_permission=0;
			
					return;
				}// if (mb_skip_run
			}//	if(sh->slice_type	
		}// if mb_skip_read_permission

		decode_module_MB(&_sh,&_sps,&_pps,_mpi,_CurrMbAddr,&_mb,&_nalu,&_mb_pos_x,&_mb_pos_y,&_QPy,&_QPc,_LumaACLevel,_LumaDCLevel,_ChromaDCLevel,_ChromaACLevel,&_intra_chroma_pred_mode);//& TODO
	
       	if ((_mb.MbPartPredMode[0]==Intra_4x4) or (_mb.MbPartPredMode[0]==Intra_16x16)){
			//current_event=INTRA_PRED;
			_trigger_event=TRIGGER_INTRA_PRED;
			printf ("generate intra prediction:\n");
		}
		if  ((_mb.MbPartPredMode[0]!=Intra_4x4) and (_mb.MbPartPredMode[0]!=Intra_16x16)){
			//current_event=INTER_PRED;
			_trigger_event=TRIGGER_INTER_PRED_NORMAL;
			printf ("generate inter prediction:\n");
		}	
		_mb_skip_read_permission=1;
	}
	/////////////////////////////////////////////////////////////////////////

	printf ("MPI in DECODE_MB:\n");	
	printf ("_mb_pos_y,_mb_pos_x = (%d,%d)\n",_mb_pos_y,_mb_pos_x);
	printf("\n");	
	printf("16x16\n");
	printf ("_mpi->MbMode[%d]: %d\n",(_mb_pos_x/16)+(_mb_pos_y/16)*(_mpi->MbWidth),_mpi->MbMode[(_mb_pos_x/16)+(_mb_pos_y/16)*(_mpi->MbWidth)]);
/*
	printf("\n8x8\n");
	printf("\n");
	for (int i=(_mb_pos_y/8); i<(_mb_pos_y/8)+2; i++){
		printf("\n");
		for (int j=(_mb_pos_x/8); j<(_mb_pos_x/8)+2; j++)	
			printf(" %d,",_mpi->TotalCoeffC[0][j+i*_mpi->CbWidth] );
	}
	printf("\n");
	for (int i=(_mb_pos_y/8); i<(_mb_pos_y/8)+2; i++){
		printf("\n");
		for (int j=(_mb_pos_x/8); j<(_mb_pos_x/8)+2; j++)	
			printf(" %d,",_mpi->TotalCoeffC[1][j+i*_mpi->CbWidth] );
	}

	printf("\n");
	printf("4x4\n");
    for (int i=(_mb_pos_y/4); i<(_mb_pos_y/4)+4; i++){
	printf("\n");	
        for (int j=(_mb_pos_x/4); j<(_mb_pos_x/4)+4; j++) 
            printf ("%d ,",_mpi->TotalCoeffL[j+i*_mpi->TbWidth]);
		
	}
*/

    //for (int i=(_mb_pos_y/4); i<(_mb_pos_y/4)+4; i++){
	//printf("\n");	
    //    for (int j=(_mb_pos_x/4); j<(_mb_pos_x/4)+4; j++)
    //        printf ("%d ,",_mpi->Intra4x4PredMode[j+i*_mpi->TbWidth]);			
	//}

	//printf("\n\nmpi->MVx[][]: decode MB\n");
    for (int i=(_mb_pos_y/4); i<(_mb_pos_y/4)+4; i++){
	//printf("\n");	
        //for (int j=(_mb_pos_x/4); j<(_mb_pos_x/4)+4; j++) 
        //    printf ("%d ,",_mpi->MVx[j+i*_mpi->TbWidth]);
		
	}
	//printf("\n\nmpi->MVy[][]: decode MB\n");
    for (int i=(_mb_pos_y/4); i<(_mb_pos_y/4)+4; i++){
	//printf("\n");	
        //for (int j=(_mb_pos_x/4); j<(_mb_pos_x/4)+4; j++) 
        //    printf ("%d ,",_mpi->MVy[j+i*_mpi->TbWidth]);
		
	}
	


}

////////////////////////////////////////////////////////////////
void obj_intra_pred_PE::run_intra_pred_MB(){

	intra_pred_MB(_mb_pos_x,_mb_pos_y,_mpi,_QPc,_QPy,_MbPartPredMode,_intra_chroma_pred_mode,
	_constrained_intra_pred_flag,_Intra16x16PredMode,_CodedBlockPatternChroma, 
	_LumaACLevel,_LumaDCLevel,_ChromaDCLevel,_ChromaACLevel);

	//add current _MB_frame in private variables
	get_MB_frame_from_frame(_mb_pos_x,_mb_pos_y);

	//printf("\n\nmpi->MVx[][]: intra\n");
    for (int i=(_mb_pos_y/4); i<(_mb_pos_y/4)+4; i++){
	//printf("\n");	
        //for (int j=(_mb_pos_x/4); j<(_mb_pos_x/4)+4; j++) 
        //    printf ("%d ,",_mpi->MVx[j+i*_mpi->TbWidth]);
		
	}
	//printf("\n\nmpi->MVy[][]: intra\n");
    for (int i=(_mb_pos_y/4); i<(_mb_pos_y/4)+4; i++){
	//printf("\n");	
        //for (int j=(_mb_pos_x/4); j<(_mb_pos_x/4)+4; j++) 
        //    printf ("%d ,",_mpi->MVy[j+i*_mpi->TbWidth]);
	}

	//TODO
	//printf("HCK: Update MPI in decode_MB:\n");
	my_mpi_MB.MbMode=_mpi->MbMode[(_mb_pos_x/16)+(_mb_pos_y/16)*(_mpi->MbWidth)];
    for (int i=(_mb_pos_y/4); i<(_mb_pos_y/4)+4; i++)	
        for (int j=(_mb_pos_x/4); j<(_mb_pos_x/4)+4; j++){
          my_mpi_MB.Intra4x4PredMode[(j-_mb_pos_x/4)+(i-_mb_pos_y/4)*4] = _mpi->Intra4x4PredMode[j+i*_mpi->TbWidth];					  
		}	
	update_intra_mpi_in_decode_MB_flag=1;
		
	
	my_mb_pos_x=_mb_pos_x;
	my_mb_pos_y=_mb_pos_y;

	printf ("run_intra_pred DONE\n");
}

//////////////////////////////////////////////////////////////////
MB_frame obj_intra_pred_PE::get_MB_frame_from_frame(int mb_pos_x,int mb_pos_y){	//set _MB_frame, it can return it too		
	for (int i=0;i<16;i++)
		for (int j=0;j<16;j++){
			_MB_frame.L[j*16+i]=_this_frame.L[(mb_pos_y+j)*_this_frame.Lpitch+(mb_pos_x+i)];
			_MB_frame.C[0][(j>>1)*8+(i>>1)]=_this_frame.C[0][((mb_pos_y+j)>>1)*_this_frame.Cpitch+((mb_pos_x+i)>>1)];
			_MB_frame.C[1][(j>>1)*8+(i>>1)]=_this_frame.C[1][((mb_pos_y+j)>>1)*_this_frame.Cpitch+((mb_pos_x+i)>>1)];
		}
	return _MB_frame;
}

///////////////////////////////////////////////////////////////////
void obj_intra_pred_PE::update_frame_with_received_MB_frame(){			
			for (int i=0;i<16;i++)
				for (int j=0;j<16;j++){
					_this_frame.L[(_received_pos_y+j)*_this_frame.Lpitch+(_received_pos_x+i)]=_received_MB_frame.L[j*16+i];
					_this_frame.C[0][((_received_pos_y+j)>>1)*_this_frame.Cpitch+((_received_pos_x+i)>>1)]=_received_MB_frame.C[0][(j>>1)*8+(i>>1)];
					_this_frame.C[1][((_received_pos_y+j)>>1)*_this_frame.Cpitch+((_received_pos_x+i)>>1)]=_received_MB_frame.C[1][(j>>1)*8+(i>>1)];
				}
		}

////////////////////////////////////////////////////////////////
void obj_intra_pred_PE::set_private_data(int mb_pos_x,int mb_pos_y,int QPc,int QPy,int MbPartPredMode,int intra_chroma_pred_mode,int constrained_intra_pred_flag,int Intra16x16PredMode,int CodedBlockPatternChroma,int LumaDCLevel[16],int LumaACLevel[16][16],int ChromaDCLevel[2][4],int ChromaACLevel[2][4][16],mode_pred_info_MB mpi_MB)
{
	_MbPartPredMode=MbPartPredMode;
	_mb_pos_x=mb_pos_x;
	_mb_pos_y=mb_pos_y;
	_QPy=QPy;
	_QPc=QPc;
	_intra_chroma_pred_mode=intra_chroma_pred_mode;
	_constrained_intra_pred_flag=constrained_intra_pred_flag;
	_CodedBlockPatternChroma=CodedBlockPatternChroma;
	_Intra16x16PredMode=Intra16x16PredMode;
	
	//_mpi=mpi;
	for (int i=0;i<16;i++)
		for (int j=0;j<16;j++)
			_LumaACLevel[i][j]=LumaACLevel[i][j];
	for (int i=0;i<16;i++)
		_LumaDCLevel[i]=LumaDCLevel[i];
	for (int i=0;i<2;i++)
		for (int j=0;j<4;j++)
			for (int k=0;k<16;k++)
				_ChromaACLevel[i][j][k]=ChromaACLevel[i][j][k];
	for (int i=0;i<2;i++)
		for (int j=0;j<4;j++)
			_ChromaDCLevel[i][j]=ChromaDCLevel[i][j];
	////////////////////////////////////////////////////
    _mpi->MbHeight=mpi_MB.MbHeight;
    _mpi->MbWidth=mpi_MB.MbWidth;
    _mpi->MbPitch=mpi_MB.MbPitch;

	//printf("mpi_MB.MbMode= %d\n",mpi_MB.MbMode);
	//printf("place= %d\n",(_mb_pos_x/16)+(_mb_pos_y/16)*(_mpi->MbWidth));
    _mpi->MbMode[(_mb_pos_x/16)+(_mb_pos_y/16)*(_mpi->MbWidth)]=mpi_MB.MbMode;
    _mpi->CbWidth=mpi_MB.CbWidth;
    _mpi->CbHeight=mpi_MB.CbHeight;
    _mpi->CbPitch=mpi_MB.CbPitch;
    for (int i=0; i<2; i++)
        for (int j=0; j<2; j++)
        {
            _mpi->TotalCoeffC[0][(j+_mb_pos_x/8)+(i+_mb_pos_y/8)*_mpi->CbWidth]=mpi_MB.TotalCoeffC[0][j+i*2];         
            _mpi->TotalCoeffC[1][(j+_mb_pos_x/8)+(i+_mb_pos_y/8)*_mpi->CbWidth]=mpi_MB.TotalCoeffC[1][j+i*2];             
        }

    _mpi->TbWidth=mpi_MB.TbWidth;
    _mpi->TbHeight=mpi_MB.TbHeight;
    _mpi->TbPitch=mpi_MB.TbPitch;
 	//printf("5: \n");    
    for (int i=0; i<4; i++)
        for (int j=0; j<4; j++)
        {
            _mpi->TotalCoeffL[(j+_mb_pos_x/4)+(i+_mb_pos_y/4)*_mpi->TbWidth]=mpi_MB.TotalCoeffL[j+i*4]; 
            _mpi->Intra4x4PredMode[(j+_mb_pos_x/4)+(i+_mb_pos_y/4)*_mpi->TbWidth]=mpi_MB.Intra4x4PredMode[j+i*4]; 
            _mpi->MVx[(j+_mb_pos_x/4)+(i+_mb_pos_y/4)*_mpi->TbWidth]=mpi_MB.MVx[j+i*4]; 
            _mpi->MVy[(j+_mb_pos_x/4)+(i+_mb_pos_y/4)*_mpi->TbWidth]=mpi_MB.MVy[j+i*4];
        }
	

}
////////////////////////////////////////////////////////////////
void obj_intra_pred_PE::intra_pred_MB(int mb_pos_x,int mb_pos_y,mode_pred_info *mpi, int QPc,int QPy,int MbPartPredMode,int intra_chroma_pred_mode,int constrained_intra_pred_flag,int Intra16x16PredMode,int CodedBlockPatternChroma, int LumaACLevel[16][16],int LumaDCLevel[16],int ChromaDCLevel[2][4],int ChromaACLevel[2][4][16]){

	intra_prediction_module_MB(_MbPartPredMode,_mb_pos_x,_mb_pos_y,&_this_frame,_mpi,_QPy,_LumaACLevel,_intra_chroma_pred_mode,_constrained_intra_pred_flag,_Intra16x16PredMode,_LumaDCLevel,_ChromaDCLevel,_ChromaACLevel,_CodedBlockPatternChroma,_QPc);

}

/////////////////////////////////////////////////////////////////////
/////////////////////////////INTER////////////////////////////////
////////////////////////////////////////////////////////////////////
MB_frame obj_inter_pred_PE::get_MB_frame_from_frame(int mb_pos_x,int mb_pos_y){	//set _MB_frame, it can return it too		
	for (int i=0;i<16;i++)
		for (int j=0;j<16;j++){
			_MB_frame.L[j*16+i]=_this_frame.L[(mb_pos_y+j)*_this_frame.Lpitch+(mb_pos_x+i)];
			_MB_frame.C[0][(j>>1)*8+(i>>1)]=_this_frame.C[0][((mb_pos_y+j)>>1)*_this_frame.Cpitch+((mb_pos_x+i)>>1)];
			_MB_frame.C[1][(j>>1)*8+(i>>1)]=_this_frame.C[1][((mb_pos_y+j)>>1)*_this_frame.Cpitch+((mb_pos_x+i)>>1)];
		}
	return _MB_frame;
}

///////////////////////////////////////////////////////////////////
void obj_inter_pred_PE::update_frame_with_received_MB_frame(){			
			for (int i=0;i<16;i++)
				for (int j=0;j<16;j++){
					_this_frame.L[(_received_pos_y+j)*_this_frame.Lpitch+(_received_pos_x+i)]=_received_MB_frame.L[j*16+i];
					_this_frame.C[0][((_received_pos_y+j)>>1)*_this_frame.Cpitch+((_received_pos_x+i)>>1)]=_received_MB_frame.C[0][(j>>1)*8+(i>>1)];
					_this_frame.C[1][((_received_pos_y+j)>>1)*_this_frame.Cpitch+((_received_pos_x+i)>>1)]=_received_MB_frame.C[1][(j>>1)*8+(i>>1)];
				}
}
			
////////////////////////////////////////////////////////////////
void obj_inter_pred_PE::run_inter_pred_normal_MB(){
	inter_pred_normal_MB(_MbPartPredMode,_mpi,_mb_pos_x,_mb_pos_y,_QPy,_QPc,
						_CodedBlockPatternChroma,_LumaACLevel,_ChromaDCLevel,_ChromaACLevel);
	//add current _MB_frame in private variables
	get_MB_frame_from_frame(_mb_pos_x,_mb_pos_y);

	//printf("\n\nmpi->MVx[][]: inter\n");
    for (int i=(_mb_pos_y/4); i<(_mb_pos_y/4)+4; i++){
	//printf("\n");	
        //for (int j=(_mb_pos_x/4); j<(_mb_pos_x/4)+4; j++) 
        //    printf ("%d ,",_mpi->MVx[j+i*_mpi->TbWidth]);
		
	}
	//printf("\n\nmpi->MVy[][]: inter\n");
    for (int i=(_mb_pos_y/4); i<(_mb_pos_y/4)+4; i++){
	//printf("\n");	
        //for (int j=(_mb_pos_x/4); j<(_mb_pos_x/4)+4; j++) 
        //    printf ("%d ,",_mpi->MVy[j+i*_mpi->TbWidth]);
	}
	//cin.get();
    for (int i=(_mb_pos_y/4); i<(_mb_pos_y/4)+4; i++)	
        for (int j=(_mb_pos_x/4); j<(_mb_pos_x/4)+4; j++){
          my_mpi_MB.MVx[(j-_mb_pos_x/4)+(i-_mb_pos_y/4)*4] = _mpi->MVx[j+i*_mpi->TbWidth];			
          my_mpi_MB.MVy[(j-_mb_pos_x/4)+(i-_mb_pos_y/4)*4] = _mpi->MVy[j+i*_mpi->TbWidth];
		  //my_mpi_MB.TotalCoeffL[(j-my_mb_pos_x/4)+(i-my_mb_pos_y/4)*4] = _mpi->TotalCoeffL[j+i*_mpi->TbWidth];
		}	
	update_inter_mpi_in_decode_MB_flag=1;
		
	
	my_mb_pos_x=_mb_pos_x;
	my_mb_pos_y=_mb_pos_y;

	printf ("run_inter_pred_NORMAL DONE\n");

}
////////////////////////////////////////////////////////////////
void obj_inter_pred_PE::inter_pred_normal_MB(int MbPartPredMode,mode_pred_info *mpi,int mb_pos_x,int mb_pos_y,int QPy,int QPc,int CodedBlockPatternChroma,int LumaACLevel[16][16],int ChromaDCLevel[2][4],int ChromaACLevel[2][4][16]){

	for (int i=0;i<16;i++)
		for (int j=0;j<16;j++)
			_LumaACLevel[i][j]=LumaACLevel[i][j];
	for (int i=0;i<2;i++)
		for (int j=0;j<4;j++)
			for (int k=0;k<16;k++)
				_ChromaACLevel[i][j][k]=ChromaACLevel[i][j][k];
	for (int i=0;i<2;i++)
		for (int j=0;j<4;j++)
			_ChromaDCLevel[i][j]=ChromaDCLevel[i][j];

	inter_prediction_module_MB(_MbPartPredMode,&_this_frame,&_ref,_mpi,_mb_pos_x,_mb_pos_y,_LumaACLevel,_QPy,0,_CurrMbAddr,_sps_PicWidthInMbs,_ChromaDCLevel,_ChromaACLevel,_CodedBlockPatternChroma,_QPc);
}

////////////////////////////////////////////////////////////////
void obj_inter_pred_PE::run_inter_pred_pskip_MB(){

	inter_pred_pSkip_MB(_mpi,_mb_pos_x,_mb_pos_y,_CurrMbAddr,_sps_PicWidthInMbs);
	//add current _MB_frame in private variables
	get_MB_frame_from_frame(_mb_pos_x,_mb_pos_y);


    for (int i=(_mb_pos_y/4); i<(_mb_pos_y/4)+4; i++)	
        for (int j=(_mb_pos_x/4); j<(_mb_pos_x/4)+4; j++){
          my_mpi_MB.MVx[(j-_mb_pos_x/4)+(i-_mb_pos_y/4)*4] = _mpi->MVx[j+i*_mpi->TbWidth];			
          my_mpi_MB.MVy[(j-_mb_pos_x/4)+(i-_mb_pos_y/4)*4] = _mpi->MVy[j+i*_mpi->TbWidth];
		  //my_mpi_MB.TotalCoeffL[(j-my_mb_pos_x/4)+(i-my_mb_pos_y/4)*4] = _mpi->TotalCoeffL[j+i*_mpi->TbWidth];
		}	
	update_inter_mpi_in_decode_MB_flag=1;
		
	
	my_mb_pos_x=_mb_pos_x;
	my_mb_pos_y=_mb_pos_y;
	printf ("run_inter_pred_PSKIP DONE\n");


}
////////////////////////////////////////////////////////////////
void obj_inter_pred_PE::inter_pred_pSkip_MB(mode_pred_info *mpi,int mb_pos_x,int mb_pos_y,int CurrMbAddr,int sps_PicWidthInMbs){
	inter_prediction_module_MB(_MbPartPredMode,&_this_frame,&_ref,_mpi,_mb_pos_x,_mb_pos_y,_LumaACLevel,_QPy,1,_CurrMbAddr,_sps_PicWidthInMbs,_ChromaDCLevel,_ChromaACLevel,_CodedBlockPatternChroma,_QPc);
}
//////////////////////////////////////////////////////////////////////
void obj_inter_pred_PE::set_private_data_for_NORMAL(int mb_pos_x,int mb_pos_y,int QPc,int QPy,int MbPartPredMode,int CodedBlockPatternChroma,int LumaDCLevel[16],int LumaACLevel[16][16],int ChromaDCLevel[2][4],int ChromaACLevel[2][4][16],mode_pred_info_MB mpi_MB)
{
	_MbPartPredMode=MbPartPredMode;
	_mb_pos_x=mb_pos_x;
	_mb_pos_y=mb_pos_y;
	_QPy=QPy;
	_QPc=QPc;
	_CodedBlockPatternChroma=CodedBlockPatternChroma;
	//_mpi=mpi;

	for (int i=0;i<16;i++)
		for (int j=0;j<16;j++)
			_LumaACLevel[i][j]=LumaACLevel[i][j];
	for (int i=0;i<16;i++)
		_LumaDCLevel[i]=LumaDCLevel[i];
	for (int i=0;i<2;i++)
		for (int j=0;j<4;j++)
			for (int k=0;k<16;k++)
				_ChromaACLevel[i][j][k]=ChromaACLevel[i][j][k];
	for (int i=0;i<2;i++)
		for (int j=0;j<4;j++)
			_ChromaDCLevel[i][j]=ChromaDCLevel[i][j];
	//////////////////////////////////////////////////
    _mpi->MbHeight=mpi_MB.MbHeight;
    _mpi->MbWidth=mpi_MB.MbWidth;
    _mpi->MbPitch=mpi_MB.MbPitch;

    _mpi->MbMode[(_mb_pos_x/16)+(_mb_pos_y/16)*(_mpi->MbWidth)]=mpi_MB.MbMode;

    _mpi->CbWidth=mpi_MB.CbWidth;
    _mpi->CbHeight=mpi_MB.CbHeight;
    _mpi->CbPitch=mpi_MB.CbPitch;
 
    for (int i=0; i<2; i++)
        for (int j=0; j<2; j++)
        {
            _mpi->TotalCoeffC[0][(j+_mb_pos_x/8)+(i+_mb_pos_y/8)*_mpi->CbWidth]=mpi_MB.TotalCoeffC[0][j+i*2];           
            _mpi->TotalCoeffC[1][(j+_mb_pos_x/8)+(i+_mb_pos_y/8)*_mpi->CbWidth]=mpi_MB.TotalCoeffC[1][j+i*2];           
        }

    _mpi->TbWidth=mpi_MB.TbWidth;
    _mpi->TbHeight=mpi_MB.TbHeight;
    _mpi->TbPitch=mpi_MB.TbPitch;
     
    for (int i=0; i<4; i++)
        for (int j=0; j<4; j++)
        {
            _mpi->TotalCoeffL[(j+_mb_pos_x/4)+(i+_mb_pos_y/4)*_mpi->TbWidth]=mpi_MB.TotalCoeffL[j+i*4]; 
            _mpi->Intra4x4PredMode[(j+_mb_pos_x/4)+(i+_mb_pos_y/4)*_mpi->TbWidth]=mpi_MB.Intra4x4PredMode[j+i*4]; 
            _mpi->MVx[(j+_mb_pos_x/4)+(i+_mb_pos_y/4)*_mpi->TbWidth]=mpi_MB.MVx[j+i*4]; 
            _mpi->MVy[(j+_mb_pos_x/4)+(i+_mb_pos_y/4)*_mpi->TbWidth]=mpi_MB.MVy[j+i*4];
        }

}
//////////////////////////////////////////////////////////////////////
void obj_inter_pred_PE::set_private_data_for_PSKIP(int mb_pos_x,int mb_pos_y,int sps_PicWidthInMbs, int CurrMbAddr,mode_pred_info_MB mpi_MB)
{

	_CurrMbAddr=CurrMbAddr;  
	_mb_pos_x=(_CurrMbAddr % sps_PicWidthInMbs)*16;
	_mb_pos_y=(_CurrMbAddr / sps_PicWidthInMbs)*16;
	_sps_PicWidthInMbs=sps_PicWidthInMbs;

	
	//_mpi=mpi;


    _mpi->MbHeight=mpi_MB.MbHeight;
    _mpi->MbWidth=mpi_MB.MbWidth;
    _mpi->MbPitch=mpi_MB.MbPitch;

    _mpi->MbMode[(_mb_pos_x/16)+(_mb_pos_y/16)*(_mpi->MbWidth)]=mpi_MB.MbMode;

    _mpi->CbWidth=mpi_MB.CbWidth;
    _mpi->CbHeight=mpi_MB.CbHeight;
    _mpi->CbPitch=mpi_MB.CbPitch;

    for (int i=0; i<2; i++)
        for (int j=0; j<2; j++)
        {
            _mpi->TotalCoeffC[0][(j+_mb_pos_x/8)+(i+_mb_pos_y/8)*_mpi->CbWidth]=mpi_MB.TotalCoeffC[0][j+i*2];           
            _mpi->TotalCoeffC[1][(j+_mb_pos_x/8)+(i+_mb_pos_y/8)*_mpi->CbWidth]=mpi_MB.TotalCoeffC[1][j+i*2];           
        }

    _mpi->TbWidth=mpi_MB.TbWidth;
    _mpi->TbHeight=mpi_MB.TbHeight;
    _mpi->TbPitch=mpi_MB.TbPitch;
     
    for (int i=0; i<4; i++)
        for (int j=0; j<4; j++)
        {
            _mpi->TotalCoeffL[(j+_mb_pos_x/4)+(i+_mb_pos_y/4)*_mpi->TbWidth]=mpi_MB.TotalCoeffL[j+i*4]; 
            _mpi->Intra4x4PredMode[(j+_mb_pos_x/4)+(i+_mb_pos_y/4)*_mpi->TbWidth]=mpi_MB.Intra4x4PredMode[j+i*4]; 
            _mpi->MVx[(j+_mb_pos_x/4)+(i+_mb_pos_y/4)*_mpi->TbWidth]=mpi_MB.MVx[j+i*4]; 
            _mpi->MVy[(j+_mb_pos_x/4)+(i+_mb_pos_y/4)*_mpi->TbWidth]=mpi_MB.MVy[j+i*4];
        }


}



//////////////////////////////////////////////////////////////////////
void obj_frame_buffer_PE::update_frame_with_received_MB_frame(){			
			for (int i=0;i<16;i++)
				for (int j=0;j<16;j++){
					_this_frame.L[(_received_pos_y+j)*_this_frame.Lpitch+(_received_pos_x+i)]=_received_MB_frame.L[j*16+i];
					_this_frame.C[0][((_received_pos_y+j)>>1)*_this_frame.Cpitch+((_received_pos_x+i)>>1)]=_received_MB_frame.C[0][(j>>1)*8+(i>>1)];
					_this_frame.C[1][((_received_pos_y+j)>>1)*_this_frame.Cpitch+((_received_pos_x+i)>>1)]=_received_MB_frame.C[1][(j>>1)*8+(i>>1)];
				}
}
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


static inline int obj_display_PE::clamp_and_scale(int i) {
#ifdef USE_X86_ASM
  asm(" \
    sarl $20,  %%eax; \
    cdq;              \
    notl %%edx;       \
    andl %%edx,%%eax; \
    subl $256, %%eax; \
    cdq;              \
    notl %%edx;       \
    orl  %%edx,%%eax; \
    andl $255, %%eax; \
  ":"=a"(i):"a"(i):"%edx");
#else
  i>>=20;
  if(i<0)i=0; if(i>255)i=255;
#endif
  return i;
}


void obj_display_PE::showframe(frame *f) {
  int i,x,y,Y,Cb=0,Cr=0,R,G,B;
  unsigned char *pY,*pCb,*pCr;
  unsigned char *line;
  SDL_Surface *screen=_main_screen;
  int zoom=1;


#if (BitsPerPixel==32)
#define DEST_TYPE unsigned int
  DEST_TYPE cache;
#define BytesPerPixel 4
#endif

#if (BitsPerPixel==24)
#define DEST_TYPE unsigned char
#define BytesPerPixel 3
#endif

#if (BitsPerPixel==15) || (BitsPerPixel==16)
#define DEST_TYPE unsigned short
  DEST_TYPE cache;
#define BytesPerPixel 2
#endif

  DEST_TYPE *dest;
  perf_enter("rendering");
  
  SDL_LockSurface(screen);
  line=screen->pixels;
  for(y=0; y<f->Lheight; ++y) {
    pY=&L_pixel(f,0,y);
    pCb=&Cb_pixel(f,0,y>>1);
    pCr=&Cr_pixel(f,0,y>>1);
    dest=(DEST_TYPE *)line;
    for(x=0; x<f->Lwidth; ++x) {
      Y=*pY++;
      if(!(x&1)) {
        Cb=*pCb++;
        Cr=*pCr++;
      }

      { int Y1,Pb,Pr;
        Y1= (Y-16) *1225732;
        Pb=(Cb-128)*1170;
        Pr=(Cr-128)*1170;
        R=clamp_and_scale(Y1        +1436*Pr);
        G=clamp_and_scale(Y1- 352*Pb -731*Pr);
        B=clamp_and_scale(Y1+1815*Pb);
      }

#if (BitsPerPixel==32)
      cache=(R<<16)|(G<<8)|B;
      for(i=zoom; i; --i)
        *dest++=cache;
#endif

#if (BitsPerPixel==24)
      for(i=zoom; i; --i) {
        *dest++=B;
        *dest++=G;
        *dest++=R;
      }
#endif

#if (BitsPerPixel==16)
      cache=((R>>3)<<11)|((G>>2)<<5)|(B>>3);
      for(i=zoom; i; --i)
        *dest++=cache;
#endif

#if (BitsPerPixel==15)
      cache=((R>>3)<<10)|((G>>3)<<5)|(B>>3);
      for(i=zoom; i; --i)
        *dest++=cache;
#endif

    }
    dest=(DEST_TYPE *)line;
    line+=screen->pitch;
    for(i=zoom; i>1; --i) {
      memcpy(line,dest,f->Lwidth*zoom*BytesPerPixel);
      line+=screen->pitch;      
    }
  }
  SDL_UnlockSurface(screen);
  SDL_UpdateRect(screen,0,0,0,0);
}
///////////////////////////////////////////////////////////
void obj_display_PE::update_frame_with_received_MB_frame(){			
			//reset_frame(_this_frame);
			for (int i=0;i<16;i++)
				for (int j=0;j<16;j++){
					_this_frame.L[(_received_pos_y+j)*_this_frame.Lpitch+(_received_pos_x+i)]=_received_MB_frame.L[j*16+i];
					_this_frame.C[0][((_received_pos_y+j)>>1)*_this_frame.Cpitch+((_received_pos_x+i)>>1)]=_received_MB_frame.C[0][(j>>1)*8+(i>>1)];
					_this_frame.C[1][((_received_pos_y+j)>>1)*_this_frame.Cpitch+((_received_pos_x+i)>>1)]=_received_MB_frame.C[1][(j>>1)*8+(i>>1)];
				}
}

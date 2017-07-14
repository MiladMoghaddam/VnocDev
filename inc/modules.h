#ifndef __MODULES_H__
#define __MODULES_H__

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
#include "NOC_global.h"
#include "main.h"

#define USE_X86_ASM
using namespace std;

//TODO
enum TRIGGER_EVENT {TRIGGER_INTER_PRED_NORMAL,TRIGGER_INTER_PRED_PSKIP,TRIGGER_INTRA_PRED};
//enum event {READ_NALU_INIT,READ_NALU,DECODE_HDR_INIT,DECODE_HDR,DECODE_MB,DECODE_MB_INIT,INTER_PRED_NORMAL,INTER_PRED_PSKIP,INTRA_PRED};
//extern enum event current_event;

class obj_get_nalu_PE {
	private:
		char *_filename;
		int _width, _height;
		nal_unit _nalu;//generate
		seq_parameter_set _sps;//generate(first_run)
        pic_parameter_set _pps;//generate(first_run)
		

	public:
		obj_get_nalu_PE(){
			#define WIDTH 352
			#define HEIGHT 288

			_filename="../streams/Plane_352x288baseline.264";
			res_file_name="../results/test.txt";
			//_filename="../streams/foreman_cif_baseline.264";//352 288
			//_filename="./nemo.264";//192x144		
			//_filename="../Girl.264"; //176x144
			//res_file_name="res_nemo.txt";
			//_filename="../vc1_Girl704x576baseline.264"; //704x576
			//res_file_name="../results/Girl_704x576_BadPlacement_new.txt";
			//_filename="../streams/Freeway_704x576baseline.264"; //704x576
			//res_file_name="../results/Freeway_704x576baseline.txt";
			//_filename="../streams/BigBuckBunny_CIF_24fps_baseline.264"; //704x576
			//res_file_name="../results/DVFS/BigBuckBunny/base.txt";
			//_filename="../streams/Plane_704x576baseline.264"; //704x576
			//res_file_name="../results/bad_placement/Plane_704x576.txt";
			//_filename="../streams/4-foreman_cif_baseline.264";
 			//_filename="../streams/Plane_176x144baseline.264";
			//_filename="../streams/stefan_cif_baseline.264";
			//res_file_name="../results/DVFS/test.txt";
			//_filename="../streams/Shore_352x288baseline.264"; //352x288
			//res_file_name="../results/Shore_352x288baseline.txt";
			//_filename="../streams/Plane_1280x720baseline.264"; //352x288
			//res_file_name="../results/Plane_1280x720baseline.txt";
		}
		void get_nalu(bool first_run_flag);
		nal_unit return_nalu(){return _nalu;}
		seq_parameter_set return_sps(){return _sps;}
		pic_parameter_set return_pps(){return _pps;}
		int return_width(){return _width;}
		int return_height(){return _height;}
		
};

//////////////////////////////////////////////////
class obj_decode_hdr_PE {
	private:
		nal_unit _nalu;//receive
		slice_header _sh;//generate
        seq_parameter_set _sps;//receive
        pic_parameter_set _pps;//receive
		int _MBcount;//used to make our decod_MB events
		int _width, _height;
	public:
		obj_decode_hdr_PE(){}
		void run_decode();//for NOC
		void decode(int first_run_flag,int picWidth,int picHeight,nal_unit nalu,seq_parameter_set sps,pic_parameter_set pps);
		slice_header return_sh(){return _sh;}
		seq_parameter_set return_sps(){return _sps;}
		pic_parameter_set return_pps(){return _pps;}
		int return_width(){return _width;}
		int return_height(){return _height;}
		nal_unit return_nalu(){return _nalu;}
		int return_MBcount(){return _MBcount;}
		void set_pic_width_height(int picWidth,int picHeight) {_width=picWidth;_height=picHeight;}
		void set_pps_sps(pic_parameter_set pps,seq_parameter_set sps){_pps=pps;_sps=sps;}
		void set_nalu(nal_unit nalu){_nalu=nalu;}
};

////////////////////////////////////////////////////////////////
class obj_decode_MB_PE {
	public:

	private:
		nal_unit _nalu;//should be received just once //also with internal buffers
		slice_header _sh;
		seq_parameter_set _sps;	
		pic_parameter_set _pps;
		mode_pred_info *_mpi;
		int _CurrMbAddr;
		int _MbCount;
		int _frame_no;
		mb_mode _mb;//generate
		int _mb_pos_x;
		int _mb_pos_y;
		int _QPy;
		int _QPc;
		int _LumaACLevel[16][16];
		int _LumaDCLevel[16];
		int _ChromaDCLevel[2][4];
		int _ChromaACLevel[2][4][16];
		int _height;
		int _width;
		int _mb_skip_run;
		int _mb_skip_read_permission;
		int _intra_chroma_pred_mode;
		TRIGGER_EVENT _trigger_event;//it would be filled at the end of the decode_MB to trigger next PRED
	public:		
		obj_decode_MB_PE(){
			_mb_skip_run=0;//first run // should be thought about TODO
			_mb_skip_read_permission=1;	
			_width=WIDTH;//192//TODO
			_height=HEIGHT;//144//TODO
			_mpi=alloc_mode_pred_info(_width, _height);
			_CurrMbAddr=-1;
			_MbCount=(_width>>4)*(_height>>4);
		}

		TRIGGER_EVENT return_trigger_event(){return _trigger_event;}
		bool run_decode_MicroBlock();

		void decode_MicroBlock(int first_run_flag, slice_header sh,
                       seq_parameter_set sps, pic_parameter_set pps,
                       nal_unit nalu, int CurrMbAddr);

		void return_LumaACLevel(int LumaACLevel[16][16]){
			for (int i=0;i<16;i++)
				for (int j=0;j<16;j++)
					LumaACLevel[i][j]=_LumaACLevel[i][j];
		}
		void return_LumaDCLevel(int LumaDCLevel[16]){
			for (int i=0;i<16;i++)
				LumaDCLevel[i]=_LumaDCLevel[i];
		}
		void return_ChromaACLevel(int ChromaACLevel[2][4][16]){
			for (int i=0;i<2;i++)
				for (int j=0;j<4;j++)
					for (int k=0;k<16;k++)
						ChromaACLevel[i][j][k]=_ChromaACLevel[i][j][k];
		}
		void return_ChromaDCLevel(int ChromaDCLevel[2][4]){
			for (int i=0;i<2;i++)
				for (int j=0;j<4;j++)
					ChromaDCLevel[i][j]=_ChromaDCLevel[i][j];
		}
		int return_intra_chroma_pred_mode(){return _intra_chroma_pred_mode;}
		pic_parameter_set return_pps(){return _pps;}//TODO
		seq_parameter_set return_sps(){return _sps;}//TODO
		mb_mode return_mb(){return _mb;}//TODO
		int return_mb_pos_x(){return _mb_pos_x;}
		int return_mb_pos_y(){return _mb_pos_y;}
		mode_pred_info* return_mpi(){return _mpi;}
		int return_QPy(){return _QPy;}
		int return_QPc(){return _QPc;}
		int return_CurrMbAddr(){return _CurrMbAddr;}
		int return_mb_MbPartPredMod_0(){return _mb.MbPartPredMode[0];}
		int return_mb_Intra16x16PredMode(){return _mb.Intra16x16PredMode;}
		int return_mb_CodedBlockPatternChroma(){return _mb.CodedBlockPatternChroma;}
		int return_pps_constrained_intra_pred_flag(){return _pps.constrained_intra_pred_flag;}
		int return_sps_PicWidthInMbs(){return _sps.PicWidthInMbs;}
		void set_sh(slice_header sh) {_sh=sh;}
		void set_pps_sps(pic_parameter_set pps,seq_parameter_set sps){_pps=pps;_sps=sps;}
		void set_nalu(nal_unit nalu){_nalu=nalu;}

		
};
class obj_intra_pred_PE{
	private:
		int _MbPartPredMode;
		int _mb_pos_x;
		int _mb_pos_y;
		int _QPy;
		int _QPc;
		int _Intra16x16PredMode;
		int _intra_chroma_pred_mode;
		int _constrained_intra_pred_flag;
		int _CodedBlockPatternChroma;
		int _LumaACLevel[16][16];
		int _LumaDCLevel[16];
		int _ChromaDCLevel[2][4];
		int _ChromaACLevel[2][4][16];
		mode_pred_info *_mpi;
		MB_frame _MB_frame;
		frame _this_frame;//output
		int _received_pos_x;
		int _received_pos_y;
		MB_frame _received_MB_frame;
		
	public:
		obj_intra_pred_PE(){
   			_this_frame=my_alloc_frame(WIDTH,HEIGHT);
			_mpi= alloc_mode_pred_info(WIDTH, HEIGHT);		
			clear_mode_pred_info(_mpi);
		}
		void run_intra_pred_MB();//for NOC
		void intra_pred_MB(int mb_pos_x,int mb_pos_y,mode_pred_info *mpi, int QPc,int QPy,int MbPartPredMode,int intra_chroma_pred_mode,int constrained_intra_pred_flag,int Intra16x16PredMode,int CodedBlockPatternChroma, int LumaACLevel[16][16],int LumaDCLevel[16],int ChromaDCLevel[2][4],int ChromaACLevel[2][4][16]);		

		frame return_frame(){return _this_frame;}

		MB_frame get_MB_frame_from_frame(int mb_pos_x,int mb_pos_y);//used in run_intra_pred_MB to set _MB_frame
		void update_frame_with_received_MB_frame();

		MB_frame return_current_MB_frame(){return _MB_frame;}
		int return_current_mb_pos_x(){return _mb_pos_x;}
		int return_current_mb_pos_y(){return _mb_pos_y;}

		MB_frame return_received_MB_frame(){return _received_MB_frame;}
		int return_received_mb_pos_x(){return _received_pos_x;}
		int return_received_mb_pos_y(){return _received_pos_y;}

		set_received_MB_frame_data(int x,int y,MB_frame MB_f){_received_pos_x=x;_received_pos_y=y;_received_MB_frame=MB_f;}
		void set_private_data(int mb_pos_x,int mb_pos_y,int QPc,int QPy,int MbPartPredMod,int intra_chroma_pred_mode,int constrained_intra_pred_flag,int Intra16x16PredMode,int CodedBlockPatternChroma,int LumaDCLevel[16],int LumaACLevel[16][16],int ChromaDCLevel[2][4],int ChromaACLevel[2][4][16],mode_pred_info_MB mpi_MB);

};
			
class obj_inter_pred_PE{
	private:
		int _MbPartPredMode;
		int _mb_pos_x;
		int _mb_pos_y;
		int _QPy;
		int _QPc;
		int _CodedBlockPatternChroma;
		int _LumaDCLevel[16];
		int _LumaACLevel[16][16];
		int _ChromaDCLevel[2][4];
		int _ChromaACLevel[2][4][16];
		int _CurrMbAddr;
		int _sps_PicWidthInMbs;
		mode_pred_info *_mpi;
		frame _this_frame;//output
		frame _ref;//output
		int _received_pos_x;
		int _received_pos_y;
		MB_frame _received_MB_frame;
		MB_frame _MB_frame;
	
	public:
		obj_inter_pred_PE(){
   			_this_frame=my_alloc_frame(WIDTH,HEIGHT);
			_ref=my_alloc_frame(WIDTH,HEIGHT);
			_mpi= alloc_mode_pred_info(WIDTH, HEIGHT);		
			clear_mode_pred_info(_mpi);

		}
		void run_inter_pred_normal_MB();
		void inter_pred_normal_MB(int MbPartPredMode,mode_pred_info *mpi,int mb_pos_x,int mb_pos_y,int QPy,int QPc,int CodedBlockPatternChroma,int LumaACLevel[16][16],int ChromaDCLevel[2][4],int ChromaACLevel[2][4][16]);

		void run_inter_pred_pskip_MB();		
		void inter_pred_pSkip_MB(mode_pred_info *mpi,int mb_pos_x,int mb_pos_y,int CurrMbAddr,int sps_PicWidthInMbs);

		frame return_frame(){return _this_frame;}
		frame return_ref(){return _ref;}

		MB_frame return_current_MB_frame(){return _MB_frame;}
		int return_current_mb_pos_x(){return _mb_pos_x;}
		int return_current_mb_pos_y(){return _mb_pos_y;}

		MB_frame return_received_MB_frame(){return _received_MB_frame;}
		int return_received_mb_pos_x(){return _received_pos_x;}
		int return_received_mb_pos_y(){return _received_pos_y;}

		set_received_MB_frame_data(int x,int y,MB_frame MB_f){_received_pos_x=x;_received_pos_y=y;_received_MB_frame=MB_f;}
		MB_frame get_MB_frame_from_frame(int mb_pos_x,int mb_pos_y);//to set _MB_frame
		void update_frame_with_received_MB_frame();

		void update_ref(){
			frame temp=my_alloc_frame(WIDTH,HEIGHT);
			copy_frame(temp,_this_frame);copy_frame(_this_frame,_ref);copy_frame(_ref,temp);
		} 


 		void set_private_data_for_NORMAL(int mb_pos_x,int mb_pos_y,int QPc,int QPy,int MbPartPredMod,int CodedBlockPatternChroma,int LumaDCLevel[16],int LumaACLevel[16][16],int ChromaDCLevel[2][4],int ChromaACLevel[2][4][16],mode_pred_info_MB mpi);
		void set_private_data_for_PSKIP(int mb_pos_x,int mb_pos_y,int sps_PicWidthInMbs, int CurrMbAddr,mode_pred_info_MB mpi);
      		    
		void reset_frame(frame f){
			for (int i=0;i<WIDTH*HEIGHT;i++)
				f.L[i]=0;
			for (int i=0;i<WIDTH*HEIGHT/4;i++){
				f.C[0][i]=0;f.C[1][i]=0;
			}
		}
		void copy_frame(frame f2,frame f1){//f2=f1;
			for (int i=0;i<WIDTH*HEIGHT;i++)
				f2.L[i]=f1.L[i];
			for (int i=0;i<WIDTH*HEIGHT/4;i++){
				f2.C[0][i]=f1.C[0][i];f2.C[1][i]=f1.C[1][i];
			}
		}

};
/////////////////////////////////////////////////////////////////////////////
class obj_frame_buffer_PE{
	private:
		int _mb_pos_x;
		int _mb_pos_y;
		MB_frame _MB_frame;
		int _received_pos_x;
		int _received_pos_y;
		MB_frame _received_MB_frame;
		frame _this_frame;//output
		frame _last_frame;
	public:
		obj_frame_buffer_PE(){
   			_this_frame=my_alloc_frame(WIDTH,HEIGHT);
			_last_frame=my_alloc_frame(WIDTH,HEIGHT);		
		}
		frame return_frame(){return _this_frame;}

		MB_frame return_current_MB_frame(){return _MB_frame;}
		int return_current_mb_pos_x(){return _mb_pos_x;}
		int return_current_mb_pos_y(){return _mb_pos_y;}

		MB_frame return_received_MB_frame(){return _received_MB_frame;}
		int return_received_mb_pos_x(){return _received_pos_x;}
		int return_received_mb_pos_y(){return _received_pos_y;}

		set_received_MB_frame_data(int x,int y,MB_frame MB_f){_received_pos_x=x;_received_pos_y=y;_received_MB_frame=MB_f;}
		void update_frame_with_received_MB_frame();
		void update_buffer(){copy_frame(_last_frame,_this_frame);reset_frame(_this_frame);} //just copied the new frame into _last_frame;
		//void update_buffer(){frame temp;copy_frame(temp,_this_frame);copy_frame(_this_frame,_last_frame);copy_frame(_last_frame,temp);} //just copied the new frame into _last_frame;
		void reset_frame(frame f){
			for (int i=0;i<WIDTH*HEIGHT;i++)
				f.L[i]=0;
			for (int i=0;i<WIDTH*HEIGHT/4;i++){
				f.C[0][i]=0;f.C[1][i]=0;
			}
		}
		void copy_frame(frame f2,frame f1){//f2=f1;
			for (int i=0;i<WIDTH*HEIGHT;i++)
				f2.L[i]=f1.L[i];
			for (int i=0;i<WIDTH*HEIGHT/4;i++){
				f2.C[0][i]=f1.C[0][i];f2.C[1][i]=f1.C[1][i];
			}
		}


};		

/////////////////////////////////////////////////////////////////////////////
class obj_display_PE{
	private:
		SDL_Surface * _main_screen;		
		int _received_pos_x;
		int _received_pos_y;
		MB_frame _received_MB_frame;
		frame _this_frame;//output
		frame _ref;
	public:
		#define BitsPerPixel 16
	public:
		obj_display_PE(){	
   			_this_frame=my_alloc_frame(WIDTH,HEIGHT);
   			_ref=my_alloc_frame(WIDTH,HEIGHT);
			_main_screen=SDL_SetVideoMode(WIDTH,HEIGHT,BitsPerPixel,SDL_HWSURFACE);        
		}
		frame return_frame(){return _this_frame;}
		MB_frame return_received_MB_frame(){return _received_MB_frame;}
		int return_received_mb_pos_x(){return _received_pos_x;}
		int return_received_mb_pos_y(){return _received_pos_y;}

		set_received_MB_frame_data(int x,int y,MB_frame MB_f){_received_pos_x=x;_received_pos_y=y;_received_MB_frame=MB_f;}

		void update_frame_with_received_MB_frame();
		static inline int clamp_and_scale(int i);		
		void showframe(frame *f);		
		void reset_frame(frame f){
			for (int i=0;i<WIDTH*HEIGHT;i++)
				f.L[i]=0;
			for (int i=0;i<WIDTH*HEIGHT/4;i++){
				f.C[0][i]=0;f.C[1][i]=0;
			}
		}
		void copy_frame(frame f2,frame f1){//f2=f1;
			for (int i=0;i<WIDTH*HEIGHT;i++)
				f2.L[i]=f1.L[i];
			for (int i=0;i<WIDTH*HEIGHT/4;i++){
				f2.C[0][i]=f1.C[0][i];f2.C[1][i]=f1.C[1][i];
			}
		}
		void reset_this_frame(){
			for (int i=0;i<WIDTH*HEIGHT;i++)
				_this_frame.L[i]=0;
			for (int i=0;i<WIDTH*HEIGHT/4;i++){
				_this_frame.C[0][i]=0;_this_frame.C[1][i]=0;
			}
		}
		void update_ref(){
			frame temp=my_alloc_frame(WIDTH,HEIGHT);
			copy_frame(temp,_this_frame);copy_frame(_this_frame,_ref);copy_frame(_ref,temp);
		} 

};	

#endif /*__MODULES_H__*/

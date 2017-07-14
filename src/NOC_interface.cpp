#include <iostream>
#include <stdio.h>
#include <vector>
#include "Display.h"
//#include "NOC_global.h"
#include "NOC_interface.h"
#include <SDL/SDL.h>
#include <time.h>
#include "vnoc.h"
#include "vnoc_event.h"
#include "main.h"
#include <limits.h>
using namespace std;
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
static inline int clamp_and_scale(int i) {
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


void showframe(SDL_Surface *screen, frame *f, int zoom) {
  int i,x,y,Y,Cb=0,Cr=0,R,G,B;
  unsigned char *pY,*pCb,*pCr;
  unsigned char *line;

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



/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
//template <class T>
void NOC_interface_send::add_flit_data(DATA_ELEMENT data)
{
    _packet.push_back(data);
    _flit_count++;
            
    if (_flit_count % _packet_size==0 && _flit_count!=0)
    {               
        _packet_array.push_back(_packet);
        _packet_num++; 
        //cout <<"packet_num= "<<_packet_num<<"  packet.size()= "<< _packet.size()<<endl;                
        _packet.clear();                
    }
    
}

void NOC_interface_send::packetize(string event_module_name)
{
    _flit_count=0;
    _packet.clear();
    _packet_array.clear();
    _packet_num=0;
    _zero_overloaded_num = 0;    
 
    if (event_module_name=="GET_NALU_TO_DECODE_HDR_FIRST_RUN")
    {
        seq_parameter_set sps;
        pic_parameter_set pps;
        nal_unit nalu;
        int picWidth,picHeight;
        nalu = _vnoc->_nalu_PE.return_nalu();    
        sps = _vnoc->_nalu_PE.return_sps();
        pps=_vnoc->_nalu_PE.return_pps();
        picWidth = _vnoc->_nalu_PE.return_width();
        picHeight = _vnoc->_nalu_PE.return_height();
        
        // printf ("packetize:GET_NALU_TO_DECODE_HDR_FIRST_RUN_FIRST_RUN\n");
        _packet.clear(); 
        _packet_array.clear();
        for (int i=0;i<_packet_size;i++)
            _packet.push_back(GET_NALU_TO_DECODE_HDR_FIRST_RUN_PACKET_FLIT_SIGN);                
        _packet_array.push_back(_packet);
        _packet.clear();
        
        //main data
        add_flit_data((DATA_ELEMENT)picWidth);//take care of casting: I'm sending int in to a DATA_ELEMENT type
        add_flit_data((DATA_ELEMENT)picHeight);
        add_pps_flit_data(pps);
        add_sps_flit_data(sps);
        add_nalu_flit_data(nalu);
    }//if "GET_NALU_FIRST_RUN"    

    else if (event_module_name=="DECODE_HDR_TO_DECODE_MB_FIRST_RUN")
    {
        slice_header sh; 
        seq_parameter_set sps;
        pic_parameter_set pps;
        nal_unit nalu;

        sh=_vnoc->_decode_hdr_PE.return_sh();
        pps=_vnoc->_decode_hdr_PE.return_pps();
        sps=_vnoc->_decode_hdr_PE.return_sps();
        nalu=_vnoc->_decode_hdr_PE.return_nalu();


        // printf ("packetize:DECODE_HDR_TO_DECODE_MB_FIRST_RUN\n");
        _packet.clear(); 
        _packet_array.clear();
        for (int i=0;i<_packet_size;i++)
            _packet.push_back(DECODE_HDR_TO_DECODE_MB_FIRST_RUN_PACKET_FLIT_SIGN);                
        _packet_array.push_back(_packet);
        _packet.clear();

        //main data
        add_sh_flit_data(sh);
        add_pps_flit_data(pps);
        add_sps_flit_data(sps);
        add_nalu_flit_data(nalu); 
        
    }//if "DECODE_HDR"
        
    else if (event_module_name=="DECODE_MB_TO_INTRA_PRED")
    {
         
        int mb_pos_x,mb_pos_y;
        int QPc,QPy;
        int intra_chroma_pred_mode;
        int CurrMbAddr;
        int LumaDCLevel[16];      
        int LumaACLevel[16][16];  
        int ChromaDCLevel[2][4]; 
        int ChromaACLevel[2][4][16];
        int mb_MbPartPredMod_0;
        int mb_Intra16x16PredMode;
        int mb_CodedBlockPatternChroma;
        int pps_constrained_intra_pred_flag;
        mode_pred_info *mpi;

        mb_pos_x=_vnoc->_decode_MB_PE.return_mb_pos_x();
        mb_pos_y=_vnoc->_decode_MB_PE.return_mb_pos_y();
        QPc=_vnoc->_decode_MB_PE.return_QPc();
        QPy=_vnoc->_decode_MB_PE.return_QPy();
        mb_MbPartPredMod_0=_vnoc->_decode_MB_PE.return_mb_MbPartPredMod_0();
        intra_chroma_pred_mode=_vnoc->_decode_MB_PE.return_intra_chroma_pred_mode();
        pps_constrained_intra_pred_flag=_vnoc->_decode_MB_PE.return_pps_constrained_intra_pred_flag();
        mb_Intra16x16PredMode=_vnoc->_decode_MB_PE.return_mb_Intra16x16PredMode();
        mb_CodedBlockPatternChroma=_vnoc->_decode_MB_PE.return_mb_CodedBlockPatternChroma();        
        _vnoc->_decode_MB_PE.return_LumaACLevel(LumaACLevel);
        _vnoc->_decode_MB_PE.return_LumaDCLevel(LumaDCLevel);
        _vnoc->_decode_MB_PE.return_ChromaACLevel(ChromaACLevel);
        _vnoc->_decode_MB_PE.return_ChromaDCLevel(ChromaDCLevel);
        mpi=_vnoc->_decode_MB_PE.return_mpi();        


        // printf ("packetize:DECODE_MB_TO_INTRA_PRED\n");
        _packet.clear(); 
        _packet_array.clear();
        for (int i=0;i<_packet_size;i++)
            _packet.push_back(DECODE_MB_TO_INTRA_PRED_PACKET_FLIT_SIGN);                
        _packet_array.push_back(_packet);
        _packet.clear();

        //main data
        add_flit_data(mb_pos_x);
        add_flit_data(mb_pos_y);
        add_flit_data(QPc);
        add_flit_data(QPy);
        add_flit_data(mb_MbPartPredMod_0);
        add_flit_data(intra_chroma_pred_mode);
        add_flit_data(pps_constrained_intra_pred_flag);
        add_flit_data(mb_Intra16x16PredMode);
        add_flit_data(mb_CodedBlockPatternChroma);
        add_luma_chroma_DC_AC_flit_data(LumaDCLevel,LumaACLevel,ChromaDCLevel,ChromaACLevel);        
        add_mpi_MB_flit_data(mpi,mb_pos_x,mb_pos_y);
        
        
    }//if "DECODE_MB_TO_INTRA_PRED"

    else if (event_module_name=="DECODE_MB_TO_INTER_PRED_NORMAL")
    {
         
        int mb_pos_x,mb_pos_y;
        int QPc,QPy;
        int intra_chroma_pred_mode;
        int CurrMbAddr;
        int LumaDCLevel[16];      
        int LumaACLevel[16][16];  
        int ChromaDCLevel[2][4]; 
        int ChromaACLevel[2][4][16];
        int mb_MbPartPredMod_0;
        int mb_Intra16x16PredMode;
        int mb_CodedBlockPatternChroma;
        int pps_constrained_intra_pred_flag;
        mode_pred_info *mpi;

        mb_pos_x=_vnoc->_decode_MB_PE.return_mb_pos_x();
        mb_pos_y=_vnoc->_decode_MB_PE.return_mb_pos_y();
        QPc=_vnoc->_decode_MB_PE.return_QPc();
        QPy=_vnoc->_decode_MB_PE.return_QPy();
        mb_MbPartPredMod_0=_vnoc->_decode_MB_PE.return_mb_MbPartPredMod_0();
        intra_chroma_pred_mode=_vnoc->_decode_MB_PE.return_intra_chroma_pred_mode();
        pps_constrained_intra_pred_flag=_vnoc->_decode_MB_PE.return_pps_constrained_intra_pred_flag();
        mb_Intra16x16PredMode=_vnoc->_decode_MB_PE.return_mb_Intra16x16PredMode();
        mb_CodedBlockPatternChroma=_vnoc->_decode_MB_PE.return_mb_CodedBlockPatternChroma();        
        _vnoc->_decode_MB_PE.return_LumaACLevel(LumaACLevel);
        _vnoc->_decode_MB_PE.return_LumaDCLevel(LumaDCLevel);
        _vnoc->_decode_MB_PE.return_ChromaACLevel(ChromaACLevel);
        _vnoc->_decode_MB_PE.return_ChromaDCLevel(ChromaDCLevel);
        mpi=_vnoc->_decode_MB_PE.return_mpi();        

        // printf ("packetize:DECODE_MB_TO_INTER_PRED_NORMAL\n");
        _packet.clear(); 
        _packet_array.clear();
        for (int i=0;i<_packet_size;i++)
            _packet.push_back(DECODE_MB_TO_INTER_PRED_NORMAL_PACKET_FLIT_SIGN);                
        _packet_array.push_back(_packet);
        _packet.clear();

        //main data
        add_flit_data(mb_pos_x);
        add_flit_data(mb_pos_y);
        add_flit_data(QPc);
        add_flit_data(QPy);
        add_flit_data(mb_MbPartPredMod_0);
        add_flit_data(mb_CodedBlockPatternChroma);
        add_luma_chroma_DC_AC_flit_data(LumaDCLevel,LumaACLevel,ChromaDCLevel,ChromaACLevel);        
        add_mpi_MB_flit_data(mpi,mb_pos_x,mb_pos_y);
        
        
    }//if "DECODE_MB_TO_INTER_PRED_NORMAL"

    else if (event_module_name=="DECODE_MB_TO_INTER_PRED_PSKIP")
    {
         
        int mb_pos_x,mb_pos_y;
        int QPc,QPy;
        int intra_chroma_pred_mode;
        int CurrMbAddr;
        int LumaDCLevel[16];      
        int LumaACLevel[16][16];  
        int ChromaDCLevel[2][4]; 
        int ChromaACLevel[2][4][16];
        int mb_MbPartPredMod_0;
        int mb_Intra16x16PredMode;
        int mb_CodedBlockPatternChroma;
        int pps_constrained_intra_pred_flag;
        mode_pred_info *mpi;
        int sps_PicWidthInMbs;
        

        mb_pos_x=_vnoc->_decode_MB_PE.return_mb_pos_x();
        mb_pos_y=_vnoc->_decode_MB_PE.return_mb_pos_y();
        QPc=_vnoc->_decode_MB_PE.return_QPc();
        QPy=_vnoc->_decode_MB_PE.return_QPy();
        mb_MbPartPredMod_0=_vnoc->_decode_MB_PE.return_mb_MbPartPredMod_0();
        intra_chroma_pred_mode=_vnoc->_decode_MB_PE.return_intra_chroma_pred_mode();
        pps_constrained_intra_pred_flag=_vnoc->_decode_MB_PE.return_pps_constrained_intra_pred_flag();
        mb_Intra16x16PredMode=_vnoc->_decode_MB_PE.return_mb_Intra16x16PredMode();
        mb_CodedBlockPatternChroma=_vnoc->_decode_MB_PE.return_mb_CodedBlockPatternChroma();        
        _vnoc->_decode_MB_PE.return_LumaACLevel(LumaACLevel);
        _vnoc->_decode_MB_PE.return_LumaDCLevel(LumaDCLevel);
        _vnoc->_decode_MB_PE.return_ChromaACLevel(ChromaACLevel);
        _vnoc->_decode_MB_PE.return_ChromaDCLevel(ChromaDCLevel);
        mpi=_vnoc->_decode_MB_PE.return_mpi();        
        sps_PicWidthInMbs=_vnoc->_decode_MB_PE.return_sps_PicWidthInMbs();
        CurrMbAddr=_vnoc->_decode_MB_PE.return_CurrMbAddr();        
    

        // printf ("packetize:DECODE_MB_TO_INTER_PRED_PSKIP\n");
        _packet.clear(); 
        _packet_array.clear();
        for (int i=0;i<_packet_size;i++)
            _packet.push_back(DECODE_MB_TO_INTER_PRED_PSKIP_PACKET_FLIT_SIGN);                
        _packet_array.push_back(_packet);
        _packet.clear();

        //main data
        add_flit_data(mb_pos_x);
        add_flit_data(mb_pos_y);
        add_flit_data(sps_PicWidthInMbs);        
        add_flit_data(CurrMbAddr);        
        add_mpi_MB_flit_data(mpi,mb_pos_x,mb_pos_y);
        // printf ("packetize:DECODE_MB_TO_INTER_PRED_PSKIP DONE\n");
        
    }//if "DECODE_MB_TO_INTER_PRED_PSKIP"



    else if (event_module_name=="DECODE_HDR_TO_DECODE_MB")
    {
        slice_header sh;        
        sh=_vnoc->_decode_hdr_PE.return_sh();
        // printf ("packetize:DECODE_HDR_TO_DECODE_MB\n");
        _packet.clear(); 
        _packet_array.clear();
        for (int i=0;i<_packet_size;i++)
            _packet.push_back(DECODE_HDR_TO_DECODE_MB_PACKET_FLIT_SIGN);                
        _packet_array.push_back(_packet);
        _packet.clear();

        //main data
        add_sh_flit_data(sh);
    }//if "DECODE_HDR"

    else if (event_module_name=="INTER_PRED_MB_FRAME_TO_INTRA_PRED")
    {
        int mb_pos_x;
        int mb_pos_y;
        MB_frame MB_f;        

        mb_pos_x=_vnoc->_inter_pred_PE.return_current_mb_pos_x();
        mb_pos_y=_vnoc->_inter_pred_PE.return_current_mb_pos_y();
        MB_f = _vnoc->_inter_pred_PE.return_current_MB_frame();
        // printf ("packetize:INTER_PRED_MB_TO_INTRA_PRED\n");
        _packet.clear(); 
        _packet_array.clear();
        for (int i=0;i<_packet_size;i++)
            _packet.push_back(INTER_PRED_MB_FRAME_TO_INTRA_PRED_PACKET_FLIT_SIGN);                
        _packet_array.push_back(_packet);
        _packet.clear();
        //main data
        
        add_flit_data(mb_pos_x);
        add_flit_data(mb_pos_y);        
        add_MB_frame_flit_data(MB_f);
    }//if "INTER_PRED_MB_FRAME_TO_INTRA_PRED"

    else if (event_module_name=="INTRA_PRED_MB_FRAME_TO_FRAME_BUFFER")
    {
        int mb_pos_x;
        int mb_pos_y;
        MB_frame MB_f;        

        mb_pos_x=_vnoc->_intra_pred_PE.return_received_mb_pos_x();
        mb_pos_y=_vnoc->_intra_pred_PE.return_received_mb_pos_y();
        MB_f = _vnoc->_intra_pred_PE.return_received_MB_frame();
        // printf ("packetize:INTRA_PRED_MB_FRAME_TO_FRAME_BUFFER\n");

        _packet.clear(); 
        _packet_array.clear();
        for (int i=0;i<_packet_size;i++)
            _packet.push_back(INTRA_PRED_MB_FRAME_TO_FRAME_BUFFER_PACKET_FLIT_SIGN);        
        
        _packet_array.push_back(_packet);
        _packet.clear();
        //main data
        
        add_flit_data(mb_pos_x);
        add_flit_data(mb_pos_y);        
        add_MB_frame_flit_data(MB_f);
    }//if "INTRA_PRED_MB_FRAME_TO_FRAME_BUFFER



    else if (event_module_name=="INTRA_PRED_MB_FRAME_TO_INTER_PRED")
    {
        int mb_pos_x;
        int mb_pos_y;
        MB_frame MB_f;        

        mb_pos_x=_vnoc->_intra_pred_PE.return_current_mb_pos_x();
        mb_pos_y=_vnoc->_intra_pred_PE.return_current_mb_pos_y();
        MB_f = _vnoc->_intra_pred_PE.return_current_MB_frame();
        // printf ("packetize:INTRA_PRED_MB_TO_INTER_PRED\n");
        _packet.clear(); 
        _packet_array.clear();
        for (int i=0;i<_packet_size;i++)
            _packet.push_back(INTRA_PRED_MB_FRAME_TO_INTER_PRED_PACKET_FLIT_SIGN);                
        _packet_array.push_back(_packet);
        _packet.clear();
        //main data
        
        add_flit_data(mb_pos_x);
        add_flit_data(mb_pos_y);        
        add_MB_frame_flit_data(MB_f);
    }//if "INTRA_PRED_MB_FRAME_TO_INTER_PRED"

    else if (event_module_name=="INTER_PRED_MB_FRAME_TO_FRAME_BUFFER")
    {
        int mb_pos_x;
        int mb_pos_y;
        MB_frame MB_f;        

        mb_pos_x=_vnoc->_inter_pred_PE.return_received_mb_pos_x();
        mb_pos_y=_vnoc->_inter_pred_PE.return_received_mb_pos_y();
        MB_f = _vnoc->_inter_pred_PE.return_received_MB_frame();
        // printf ("packetize:INTER_PRED_MB_FRAME_TO_FRAME_BUFFER\n");

        _packet.clear(); 
        _packet_array.clear();
        for (int i=0;i<_packet_size;i++)
            _packet.push_back(INTER_PRED_MB_FRAME_TO_FRAME_BUFFER_PACKET_FLIT_SIGN);        
        
        _packet_array.push_back(_packet);
        _packet.clear();
        //main data
        
        add_flit_data(mb_pos_x);
        add_flit_data(mb_pos_y);        
        add_MB_frame_flit_data(MB_f);
    }//if "INTER_PRED_MB_FRAME_TO_FRAME_BUFFER"

    else if (event_module_name=="FRAME_BUFFER_MB_FRAME_TO_DISPLAY")
    {
        int mb_pos_x;
        int mb_pos_y;
        MB_frame MB_f;        

        mb_pos_x=_vnoc->_frame_buffer_PE.return_received_mb_pos_x();
        mb_pos_y=_vnoc->_frame_buffer_PE.return_received_mb_pos_y();
        MB_f = _vnoc->_frame_buffer_PE.return_received_MB_frame();
        // printf ("packetize:FRAME_BUFFER_MB_FRAME_TO_DISPLAY\n");

        _packet.clear(); 
        _packet_array.clear();
        for (int i=0;i<_packet_size;i++)
            _packet.push_back(FRAME_BUFFER_MB_FRAME_TO_DISPLAY_PACKET_FLIT_SIGN);        
        
        _packet_array.push_back(_packet);
        _packet.clear();
        //main data
        
        add_flit_data(mb_pos_x);
        add_flit_data(mb_pos_y);        
        add_MB_frame_flit_data(MB_f);
    }//if "FRAME_BUFFER_MB_FRAME_TO_DISPLAY"

    else if (event_module_name=="TO_CONTINUE_DECODE_MB")
    {
        // printf ("packetize:TO_CONTINUE_DECODE_MB\n");
        _packet.clear(); 
        _packet_array.clear();
        for (int i=0;i<_packet_size;i++)
            _packet.push_back(TO_CONTINUE_DECODE_MB_PACKET_FLIT_SIGN);        
        
        _packet_array.push_back(_packet);
        _packet.clear();
        //main data        
        // printf ("packetize:TO_CONTINUE_DECODE_MB DONE\n");
    }//if "TO_CONTINUE_DECODE_MB"

    else if (event_module_name=="UPDATE_REF_FRAME_IN_INTER_PRED")
    {
        // printf ("packetize:UPDATE_REF_FRAME_IN_INTER_PRED\n");
        _packet.clear(); 
        _packet_array.clear();
        for (int i=0;i<_packet_size;i++)
            _packet.push_back(UPDATE_REF_FRAME_IN_INTER_PRED_PACKET_FLIT_SIGN);        
        
        _packet_array.push_back(_packet);
        _packet.clear();
        //main data
    }//if "UPDATE_REF_FRAME_IN_INTER_PRED"

    else if (event_module_name=="UPDATE_FRAME_IN_FRAME_BUFFER")
    {
        // printf ("packetize:UPDATE_FRAME_IN_FRAME_BUFFER\n");
        _packet.clear(); 
        _packet_array.clear();
        for (int i=0;i<_packet_size;i++)
            _packet.push_back(UPDATE_FRAME_IN_FRAME_BUFFER_PACKET_FLIT_SIGN);        
        
        _packet_array.push_back(_packet);
        _packet.clear();
        //main data
    }//if "UPDATE_FRAME_IN_FRAME_BUFFER"

    else if (event_module_name=="TO_GET_NALU")
    {
        // printf ("packetize:TO_GET_NALU\n");
        _packet.clear(); 
        _packet_array.clear();
        for (int i=0;i<_packet_size;i++)
            _packet.push_back(TO_GET_NALU_PACKET_FLIT_SIGN);        
        
        _packet_array.push_back(_packet);
        _packet.clear();
        //main data
    }//if "TO_GET_NALU"




    else
    {
        // printf ("ERROR: WRONG event_module_name in NOC_interface_send::packetize\n");
        cout << "event_module_name= "<<event_module_name<<endl;        
        exit(1);
    }
    



    ////////////////////////////////////////////////////////////////////////
   if (_packet.size()!=_packet_size && _packet.size()!= 0) 
   {
        for (long i=_packet.size(); i<_packet_size;i++)
        {                
            _packet.push_back(0);            
            _zero_overloaded_num++;
        }
        _packet_array.push_back(_packet);
        _packet_num++;        
        cout << "send to display- packet_num = "<<_packet_num<<endl;        
   }
   _packet.clear();      
   //end of package
   for (int i=0;i<_packet_size;i++)
        _packet.push_back(LAST_PACKET_FLIT_SIGN);

   _packet_array.push_back(_packet);
   _packet_num++;
   _packet.clear();
}
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
//////////////////////////////////////////////////////
void NOC_interface_receive::depacketize(string event_module_name)
{
    
    _depack_packet_num=0;
    _depack_packet_array_num=0;
    
    // printf ("depacketize\n");
    if (event_module_name=="GET_NALU_TO_DECODE_HDR_FIRST_RUN")// here I can use _event_received_from as well
    {
        seq_parameter_set sps;
        pic_parameter_set pps;
        nal_unit nalu;
        int picWidth,picHeight;

        //picWidth=get_flit_data();
        //picHeight=get_flit_data();
        picWidth=get_flit_data(); //take care of casting. I'm copying a DATA_ELEMENT into an int
        picHeight=get_flit_data();
        pps=get_pps_flit_data();
        sps=get_sps_flit_data();
        nalu=get_nalu_flit_data(); 

        _vnoc->_decode_hdr_PE.set_pic_width_height(picWidth,picHeight);
        _vnoc->_decode_hdr_PE.set_pps_sps(pps,sps);            
        _vnoc->_decode_hdr_PE.set_nalu(nalu);            
        //cin.get();
            
    }
    else if (event_module_name=="DECODE_HDR_TO_DECODE_MB_FIRST_RUN")
    {
        slice_header sh;
        seq_parameter_set sps;
        pic_parameter_set pps;
        nal_unit nalu;

        sh=get_sh_flit_data();
        pps=get_pps_flit_data();
        sps=get_sps_flit_data();
        nalu=get_nalu_flit_data(); 
        _vnoc->_decode_MB_PE.set_sh(sh);
        _vnoc->_decode_MB_PE.set_pps_sps(pps,sps);            
        _vnoc->_decode_MB_PE.set_nalu(nalu);            
        //cin.get();

        
    }

    else if (event_module_name=="DECODE_MB_TO_INTRA_PRED")//not ready
    {        
        int mb_pos_x,mb_pos_y;
        int QPc,QPy;
        int intra_chroma_pred_mode;
        int CurrMbAddr;
        int LumaDCLevel[16];      
        int LumaACLevel[16][16];  
        int ChromaDCLevel[2][4]; 
        int ChromaACLevel[2][4][16];
        int mb_MbPartPredMod_0;
        int mb_Intra16x16PredMode;
        int mb_CodedBlockPatternChroma;
        int pps_constrained_intra_pred_flag;
        mode_pred_info_MB mpi_MB;

        //main data
        mb_pos_x=get_flit_data();
        mb_pos_y=get_flit_data();
        QPc=get_flit_data();
        QPy=get_flit_data();
        mb_MbPartPredMod_0=get_flit_data();
        intra_chroma_pred_mode=get_flit_data();
        pps_constrained_intra_pred_flag=get_flit_data();
        mb_Intra16x16PredMode=get_flit_data();
        mb_CodedBlockPatternChroma=get_flit_data();
        get_luma_chroma_DC_AC_flit_data(LumaDCLevel,LumaACLevel,ChromaDCLevel,ChromaACLevel);        
        mpi_MB=get_mpi_MB_flit_data();
        //set data to _intra_pred_PE 
        _vnoc->_intra_pred_PE.set_private_data(mb_pos_x,mb_pos_y,QPc,QPy,mb_MbPartPredMod_0,intra_chroma_pred_mode,pps_constrained_intra_pred_flag,mb_Intra16x16PredMode,mb_CodedBlockPatternChroma,LumaDCLevel,LumaACLevel,ChromaDCLevel,ChromaACLevel,mpi_MB);        

    }//if "DECODE_MB_TO_INTRA_PRED"

    else if (event_module_name=="DECODE_MB_TO_INTER_PRED_NORMAL")//not ready
    {        
        int mb_pos_x,mb_pos_y;
        int QPc,QPy;
        int intra_chroma_pred_mode;
        int CurrMbAddr;
        int LumaDCLevel[16];      
        int LumaACLevel[16][16];  
        int ChromaDCLevel[2][4]; 
        int ChromaACLevel[2][4][16];
        int mb_MbPartPredMod_0;
        int mb_Intra16x16PredMode;
        int mb_CodedBlockPatternChroma;
        int pps_constrained_intra_pred_flag;
        mode_pred_info_MB mpi_MB;

        //main data
        mb_pos_x=get_flit_data();
        mb_pos_y=get_flit_data();
        QPc=get_flit_data();
        QPy=get_flit_data();
        mb_MbPartPredMod_0=get_flit_data();
        mb_CodedBlockPatternChroma=get_flit_data();
        get_luma_chroma_DC_AC_flit_data(LumaDCLevel,LumaACLevel,ChromaDCLevel,ChromaACLevel);        
        mpi_MB=get_mpi_MB_flit_data();

        //set data to _inter_pred_PE 
        _vnoc->_inter_pred_PE.set_private_data_for_NORMAL(mb_pos_x,mb_pos_y,QPc,QPy,mb_MbPartPredMod_0,mb_CodedBlockPatternChroma,LumaDCLevel,LumaACLevel,ChromaDCLevel,ChromaACLevel,mpi_MB);        

    }//if "DECODE_MB_TO_INTER_PRED_NORMAL"
    else if (event_module_name=="DECODE_MB_TO_INTER_PRED_PSKIP")//not ready
    {        
        int mb_pos_x,mb_pos_y;
        int QPc,QPy;
        int intra_chroma_pred_mode;
        int CurrMbAddr;
        int LumaDCLevel[16];      
        int LumaACLevel[16][16];  
        int ChromaDCLevel[2][4]; 
        int ChromaACLevel[2][4][16];
        int mb_MbPartPredMod_0;
        int mb_Intra16x16PredMode;
        int mb_CodedBlockPatternChroma;
        int pps_constrained_intra_pred_flag;
        mode_pred_info_MB mpi_MB;
        int sps_PicWidthInMbs;

        
        //main data
        mb_pos_x=get_flit_data();
        mb_pos_y=get_flit_data();
        sps_PicWidthInMbs=get_flit_data();
        CurrMbAddr=get_flit_data();     
        mpi_MB=get_mpi_MB_flit_data();
        //set data to _inter_pred_PE 
        _vnoc->_inter_pred_PE.set_private_data_for_PSKIP(mb_pos_x,mb_pos_y,sps_PicWidthInMbs,CurrMbAddr,mpi_MB);        

    }//if "DECODE_MB_TO_INTER_PRED_PSKIP"

    else if (event_module_name=="INTRA_PRED_MB_FRAME_TO_INTER_PRED")
    {
        int mb_pos_x;
        int mb_pos_y;
        MB_frame MB_f;
        
        //main data
        mb_pos_x=get_flit_data();
        mb_pos_y=get_flit_data();        
        MB_f=get_MB_frame_flit_data();
        _vnoc->_inter_pred_PE.set_received_MB_frame_data(mb_pos_x,mb_pos_y,MB_f);
        
    }//if "INTRA_PRED_MB_FRAME_TO_INTER_PRED"

    else if (event_module_name=="INTER_PRED_MB_FRAME_TO_FRAME_BUFFER")
    {
        int mb_pos_x;
        int mb_pos_y;
        MB_frame MB_f;
        
        //main data
        mb_pos_x=get_flit_data();
        mb_pos_y=get_flit_data();        
        MB_f=get_MB_frame_flit_data();
        _vnoc->_frame_buffer_PE.set_received_MB_frame_data(mb_pos_x,mb_pos_y,MB_f);
        
    }//if "INTER_PRED_MB_FRAME_TO_FRAME_BUFFER"

    else if (event_module_name=="INTER_PRED_MB_FRAME_TO_INTRA_PRED")
    {
        int mb_pos_x;
        int mb_pos_y;
        MB_frame MB_f;
        
        //main data
        mb_pos_x=get_flit_data();
        mb_pos_y=get_flit_data();        
        MB_f=get_MB_frame_flit_data();
        _vnoc->_intra_pred_PE.set_received_MB_frame_data(mb_pos_x,mb_pos_y,MB_f);
        
    }//if "INTER_PRED_MB_FRAME_TO_INTRA_PRED"

    else if (event_module_name=="INTRA_PRED_MB_FRAME_TO_FRAME_BUFFER")
    {
        int mb_pos_x;
        int mb_pos_y;
        MB_frame MB_f;
        
        //main data
        mb_pos_x=get_flit_data();
        mb_pos_y=get_flit_data();        
        MB_f=get_MB_frame_flit_data();
        _vnoc->_frame_buffer_PE.set_received_MB_frame_data(mb_pos_x,mb_pos_y,MB_f);
        
    }//if "INTRA_PRED_MB_FRAME_TO_FRAME_BUFFER"

    else if (event_module_name=="FRAME_BUFFER_MB_FRAME_TO_DISPLAY")
    {
        int mb_pos_x;
        int mb_pos_y;
        MB_frame MB_f;
        
        //main data
        mb_pos_x=get_flit_data();
        mb_pos_y=get_flit_data();        
        MB_f=get_MB_frame_flit_data();
        _vnoc->_display_PE.set_received_MB_frame_data(mb_pos_x,mb_pos_y,MB_f);
        
    }//if "FRAME_BUFFER_MB_FRAME_TO_DISPLAY"




    else if (event_module_name=="DECODE_HDR_TO_DECODE_MB")//not ready
    {
        slice_header sh;
        sh=get_sh_flit_data();
    }

    else
    {
        // printf ("ERROR: WRONG event_module_name in NOC_interface_receive::depacketize\n");        
        cout << "event_module_name= "<<event_module_name<<endl;    
        exit(1);
    }


}


///////////////////////////////////////////////////////////////

/////////////////////////////////////////
void NOC_interface_send::display_packet(long k)
{
        cout << "NOC_interface_send::display_packet("<<k<<")" << endl;    
        cout << "_packet_size= " <<_packet_size<<endl;
        cout << "_packet_num= " <<_packet_num<<endl;
        cout << "_zero_overloaded_num= "<<_zero_overloaded_num<<endl;
        for (long i=0;i<_packet_size;i++){
            cout<<" packet= "<<_packet_array[k][i]<<endl;
        }
        cout<<endl;
}

void NOC_interface_send::packet_array_display()
{
    cout <<"NOC_interface_send::packet_array_display()"<<endl;    
    for (int i=0;i<_packet_num;i++){            
        cout <<"packet["<<i<<"]:"<<endl;            
                
        for(int j=0;j<_packet_array[i].size();j++)
            cout <<_packet_array[i][j]<<"   ";
        cout <<endl;        
    }
    
}


////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
//////////////////////   RECIEVE   /////////////////////////
////////////////////////////////////////////////////////////
//template <class T>

DATA_ELEMENT NOC_interface_receive::get_flit_data()
{
    
    DATA_ELEMENT data=_packet_array[_depack_packet_array_num][_depack_packet_num];
    _depack_packet_num++;
    if (_depack_packet_num==_packet_size)
    {            
        _depack_packet_num=0;        
        _depack_packet_array_num++;
    } 
    return data;    
}



        



////////////////////////////////////////////////////////
bool NOC_interface_receive::is_it_start_packet(DATA packet,int packet_size)
{
    bool decision[17];
    for (int i=0;i<17;i++)
        decision[i]=true;
    
    for (int i=0;i<packet_size;i++) 
    {            
        if (packet[i]!=TO_GET_NALU_FIRST_RUN_PACKET_FLIT_SIGN)
            decision[0]=false;
        if (packet[i]!=TO_GET_NALU_PACKET_FLIT_SIGN)    
            decision[1]=false;
        if (packet[i]!=GET_NALU_TO_DECODE_HDR_FIRST_RUN_PACKET_FLIT_SIGN)
            decision[2]=false;
        if (packet[i]!=GET_NALU_TO_DECODE_HDR_PACKET_FLIT_SIGN)
            decision[3]=false;
        if (packet[i]!=DECODE_HDR_TO_DECODE_MB_FIRST_RUN_PACKET_FLIT_SIGN)
            decision[4]=false;
        if (packet[i]!=DECODE_HDR_TO_DECODE_MB_PACKET_FLIT_SIGN)
            decision[5]=false;
        if (packet[i]!=DECODE_MB_TO_INTRA_PRED_PACKET_FLIT_SIGN)
            decision[6]=false;
        if (packet[i]!=DECODE_MB_TO_INTER_PRED_NORMAL_PACKET_FLIT_SIGN)
            decision[7]=false;
        if (packet[i]!=DECODE_MB_TO_INTER_PRED_PSKIP_PACKET_FLIT_SIGN)
            decision[8]=false;
        if (packet[i]!=INTRA_PRED_MB_FRAME_TO_INTER_PRED_PACKET_FLIT_SIGN)
            decision[9]=false;
        if (packet[i]!=INTER_PRED_MB_FRAME_TO_INTRA_PRED_PACKET_FLIT_SIGN)
            decision[10]=false;
        if (packet[i]!=INTRA_PRED_MB_FRAME_TO_FRAME_BUFFER_PACKET_FLIT_SIGN)
            decision[11]=false;
        if (packet[i]!=INTER_PRED_MB_FRAME_TO_FRAME_BUFFER_PACKET_FLIT_SIGN)
            decision[12]=false;
        if (packet[i]!=FRAME_BUFFER_MB_FRAME_TO_DISPLAY_PACKET_FLIT_SIGN)
            decision[13]=false;
        if (packet[i]!=TO_CONTINUE_DECODE_MB_PACKET_FLIT_SIGN)
            decision[14]=false;
        if (packet[i]!=UPDATE_REF_FRAME_IN_INTER_PRED_PACKET_FLIT_SIGN)
            decision[15]=false;
        if (packet[i]!=UPDATE_FRAME_IN_FRAME_BUFFER_PACKET_FLIT_SIGN)
            decision[16]=false;


    }
    
    for (int i=0;i<17;i++)
        if (decision[i]==true) 
            {
                //printf("it is start packet\n");                
                //cin.get();
                
            return true;
            }
    
    return false;

}

bool NOC_interface_receive::is_it_end_packet(DATA packet,int packet_size)
{
    for (int i=0;i<packet_size;i++)
        if (packet[i]!=LAST_PACKET_FLIT_SIGN)
               return false;
   return true;
}

void NOC_interface_receive::display_packet(long k)
{
        cout << "_packet_size= " <<_packet_size<<endl;
        cout << "_packet_num= " <<_packet_num<<endl;
        cout << "_zero_overloaded_num= "<<_zero_overloaded_num<<endl;
        for (long i=0;i<_packet_size;i++){
            cout<<" packet= "<<_packet_array[k][i]<<endl;
        }
        cout<<endl;
}

void NOC_interface_receive::set_event_mode(DATA packet,int packet_size)
{
    switch (packet[0]){
        case TO_GET_NALU_FIRST_RUN_PACKET_FLIT_SIGN://defined in NOC_global.h
            _event_mode_received=TO_GET_NALU_FIRST_RUN;
            break;
        
        case TO_GET_NALU_PACKET_FLIT_SIGN:
            _event_mode_received=TO_GET_NALU;
            break;
        
        case GET_NALU_TO_DECODE_HDR_FIRST_RUN_PACKET_FLIT_SIGN:
            _event_mode_received=GET_NALU_TO_DECODE_HDR_FIRST_RUN;
            break;

        case GET_NALU_TO_DECODE_HDR_PACKET_FLIT_SIGN:
            _event_mode_received=GET_NALU_TO_DECODE_HDR;
            break;

        case DECODE_HDR_TO_DECODE_MB_FIRST_RUN_PACKET_FLIT_SIGN:
            _event_mode_received=DECODE_HDR_TO_DECODE_MB_FIRST_RUN;
            break;

        case DECODE_HDR_TO_DECODE_MB_PACKET_FLIT_SIGN:
            _event_mode_received=DECODE_HDR_TO_DECODE_MB;
            break;

        case DECODE_MB_TO_INTRA_PRED_PACKET_FLIT_SIGN:
            _event_mode_received=DECODE_MB_TO_INTRA_PRED;
            break;

        case DECODE_MB_TO_INTER_PRED_NORMAL_PACKET_FLIT_SIGN:
            _event_mode_received=DECODE_MB_TO_INTER_PRED_NORMAL;
            break;

        case DECODE_MB_TO_INTER_PRED_PSKIP_PACKET_FLIT_SIGN:
            _event_mode_received=DECODE_MB_TO_INTER_PRED_PSKIP;
            break;

        case INTRA_PRED_MB_FRAME_TO_INTER_PRED_PACKET_FLIT_SIGN:
            _event_mode_received=INTRA_PRED_MB_FRAME_TO_INTER_PRED;
            break;

        case INTER_PRED_MB_FRAME_TO_INTRA_PRED_PACKET_FLIT_SIGN:
            _event_mode_received=INTER_PRED_MB_FRAME_TO_INTRA_PRED;
            break;

        case INTRA_PRED_MB_FRAME_TO_FRAME_BUFFER_PACKET_FLIT_SIGN:
            _event_mode_received=INTRA_PRED_MB_FRAME_TO_FRAME_BUFFER;
            break;

        case INTER_PRED_MB_FRAME_TO_FRAME_BUFFER_PACKET_FLIT_SIGN:
            _event_mode_received=INTER_PRED_MB_FRAME_TO_FRAME_BUFFER;
            break;

        case FRAME_BUFFER_MB_FRAME_TO_DISPLAY_PACKET_FLIT_SIGN:
            _event_mode_received=FRAME_BUFFER_MB_FRAME_TO_DISPLAY;
            break;

        case TO_CONTINUE_DECODE_MB_PACKET_FLIT_SIGN:
            _event_mode_received=TO_CONTINUE_DECODE_MB;
            break;

        case UPDATE_REF_FRAME_IN_INTER_PRED_PACKET_FLIT_SIGN:
            _event_mode_received=UPDATE_REF_FRAME_IN_INTER_PRED;
            break;
        
        case UPDATE_FRAME_IN_FRAME_BUFFER_PACKET_FLIT_SIGN:
            _event_mode_received=UPDATE_FRAME_IN_FRAME_BUFFER;
            break;
                


        default:    
            // printf("DUMMY in set_event_mode \n");
            exit(1);
            
    }
}

            


void NOC_interface_receive::get_packet_from_NOC(double time,long long unsigned int data)
{
    DATA my_packet;
    SDL_Surface *screen;
    //printf("received_packet= %d\n",data);
    //cin.get();
    
    _packet.push_back(data);
    if (_packet.size()==_packet_size)
    {            
        if(is_it_start_packet(_packet,_packet_size))
        {
            // printf("start_packet received\n");            
            set_event_mode(_packet,_packet_size); 
           _packets_ready=false;
           _packet_array.clear();            
           _packet.clear();
           _packet_num=0; 
        }

        else if(is_it_end_packet(_packet,_packet_size))
        {                
            // printf("end_packet received\n");            
           _packet.clear();
           _packets_ready=true; 
        }
        else
        {
            //_packets_ready=false;
           _packet_array.push_back(_packet);
           _packet_num++;                
           _packet.clear();                
        }
   }    
    
    
    if (_packets_ready==true)
    {                    
            double delay = IP_CORE_DELAY;
            cout << "received packet time: "<<float(clock())<<endl;
            cout <<"packets_ready = true"<<endl;
            my_estimated_time = clock()-my_start_time;
            // printf ("start_time: %g, end_time= %g\n", my_start_time,clock());            
            // printf ("my_estimated time for receiving %d packets is:\n%f %f %e\n",_packet_num,double(my_estimated_time)/CLOCKS_PER_SEC,double(my_estimated_time*1000000)/CLOCKS_PER_SEC,my_estimated_time/CLOCKS_PER_SEC);            
            // printf ("_noc_ifc_receive[]._ID = %d\n",_ID);            
            // printf ("_event_mode_received_from= %d\n",_event_mode_received);
            
            
            
            //cin.get();            
            //if _event_mode = ....
            // build it in this way ...
            //if (_event_mode == GET_NALU_FIRST_RUN)
            //    build_from_GET_NALU_FIRST_RUN(); //set decode_hdr data
            //if (_event_mode == GET_NALU)
            //    build_from_GET_NALU();
            //if (_event_mode == DECODE_HDR_FIRST_RUN)
            //    build_from_DECODE_HDR_FIRST_RUN();
            //if (_event_mode == DECODE_HDR)
            //    build_from_DECODE_HDR();
            //nalu_PE->get_nalu(0);
            /*

            |--> GET_NALU -> DECODE_HDR -> DECODE_MB -> INTRA_PRED -> DISPLAY -----------> 
            |                                       `-> INTER_PRED `                      |
            |-----------------------------------------------------------------------------|

            */                                                               
            if (_event_mode_received == GET_NALU_TO_DECODE_HDR_FIRST_RUN)      
            {                    


                //NOW we are in DECODE_HDR_FIRST_RUN 
                // printf ("_event_mode_received == GET_NALU_TO_DECODE_HDR_FIRST_RUN\n");                
                // printf ("_noc_ifc_receive[]._ID = %d\n",_ID);     
                // printf ("_noc_ifc_send[]._ID = %d\n",DECODE_HDR_ID);     
 
                _vnoc->_noc_ifc_receive[DECODE_HDR_ID].depacketize("GET_NALU_TO_DECODE_HDR_FIRST_RUN");
                // printf ("depacketizing is finished\n");
                _vnoc->_decode_hdr_PE.run_decode();   
                _vnoc->_noc_ifc_send[DECODE_HDR_ID].packetize("DECODE_HDR_TO_DECODE_MB_FIRST_RUN");

                
                //_vnoc->_noc_ifc_send[DECODE_HDR_ID].packet_array_display();
                //cin.get();
                

                cout <<"time= "<<time<<endl;
                cout <<"event_current_sim_time= "<<_vnoc->_event_queue->current_sim_time()<<endl;            

                double delay = IP_CORE_DELAY;
                
                my_packet =_vnoc->_noc_ifc_send[DECODE_HDR_ID].get_packet(_vnoc->get_packet_injected_num("DECODE_HDR"));
                _vnoc->_event_queue->add_event( EVENT(EVENT::DECODE_HDR_TO_DECODE_MB_FIRST_RUN,_vnoc->_event_queue->current_sim_time()+ delay, DISPLAY, my_packet));
            }

            if (_event_mode_received == DECODE_HDR_TO_DECODE_MB_FIRST_RUN)      
            {
                //NOW we are in DECODE_MB_FIRST_RUN
                // printf ("_event_mode_received == DECODE_HDR_TO_DECODE_MB_FIRST_RUN\n");                         
                // printf ("_noc_ifc_receive[]._ID = %d\n",_ID);     
                // printf ("_noc_ifc_send[]._ID = %d\n",DECODE_MB_ID);            

                _vnoc->_noc_ifc_receive[DECODE_MB_ID].depacketize("DECODE_HDR_TO_DECODE_MB_FIRST_RUN");
                // printf ("depacketizing is finished\n");
                
                if (_vnoc->_decode_MB_PE.run_decode_MicroBlock())
                {                        
                
                    if (_vnoc->_decode_MB_PE.return_trigger_event() ==TRIGGER_INTRA_PRED)
                    {                        
                        // printf ("TRIGGER EVENT = INTRA_PRED\n");
                        _vnoc->_noc_ifc_send[DECODE_MB_ID].packetize("DECODE_MB_TO_INTRA_PRED");
                        my_packet =_vnoc->_noc_ifc_send[DECODE_MB_ID].get_packet(_vnoc->get_packet_injected_num("DECODE_MB"));
                        _vnoc->_event_queue->add_event( EVENT(EVENT::DECODE_MB_TO_INTRA_PRED,_vnoc->_event_queue->current_sim_time()+ delay, DISPLAY, my_packet));                                        
                    }
                
                    if (_vnoc->_decode_MB_PE.return_trigger_event() == TRIGGER_INTER_PRED_NORMAL)
                    {                        
                        // printf ("TRIGGER EVENT = TRIGGER_INTER_PRED_NORMAL\n");
                        _vnoc->_noc_ifc_send[DECODE_MB_ID].packetize("DECODE_MB_TO_INTER_PRED_NORMAL");
                        my_packet =_vnoc->_noc_ifc_send[DECODE_MB_ID].get_packet(_vnoc->get_packet_injected_num("DECODE_MB"));
                        _vnoc->_event_queue->add_event( EVENT(EVENT::DECODE_MB_TO_INTER_PRED_NORMAL,_vnoc->_event_queue->current_sim_time()+ delay, DISPLAY, my_packet));                                    
                    }

                    if (_vnoc->_decode_MB_PE.return_trigger_event() == TRIGGER_INTER_PRED_PSKIP)
                    {                        
                        // printf ("TRIGGER EVENT = TRIGGER_INTER_PRED_PSKIP\n");
                        _vnoc->_noc_ifc_send[DECODE_MB_ID].packetize("DECODE_MB_TO_INTER_PRED_PSKIP");
                        my_packet =_vnoc->_noc_ifc_send[DECODE_MB_ID].get_packet(_vnoc->get_packet_injected_num("DECODE_MB"));
                        _vnoc->_event_queue->add_event( EVENT(EVENT::DECODE_MB_TO_INTER_PRED_PSKIP,_vnoc->_event_queue->current_sim_time()+ delay, DISPLAY, my_packet));                                        
                    }             
                }
                else
                {
                    // printf("_decode_MB_PE.run_decode_MicroBlock() = false, CurrMbAddress == MbCount\n"); 
                    //add get_nalu event;                   
                }

            } //if "DECODE_HDR_TO_DECODE_MB_FIRST_RUN"
//INTRA_PRED            
            if (_event_mode_received == DECODE_MB_TO_INTRA_PRED)      
            {                    

                //screen=SDL_SetVideoMode(192,144,BitsPerPixel,SDL_HWSURFACE);

                MB_frame mb_f;                
                //NOW we are in INTRA_PRED 
                // printf ("_event_mode_received == DECODE_MB_TO_INTRA_PRED\n");                
                // printf ("_noc_ifc_receive[]._ID = %d\n",_ID);     
                // printf ("_noc_ifc_send[]._ID = %d\n",INTRA_PRED_ID);     
                
                
                _vnoc->_noc_ifc_receive[INTRA_PRED_ID].depacketize("DECODE_MB_TO_INTRA_PRED");
                // printf ("depacketizing is finished\n");
                _vnoc->_intra_pred_PE.run_intra_pred_MB();//produces MB_frame

                //showframe(screen,&(_vnoc->_intra_pred_PE.return_frame()),1);
                //cin.get();
            
                _vnoc->_noc_ifc_send[INTRA_PRED_ID].packetize("INTRA_PRED_MB_FRAME_TO_INTER_PRED");
                //_vnoc->_noc_ifc_send[INTRA_PRED_ID].packet_array_display();                

                cout <<"time= "<<time<<endl;
                cout <<"event_current_sim_time= "<<_vnoc->_event_queue->current_sim_time()<<endl;            

                double delay = IP_CORE_DELAY;

              
                my_packet =_vnoc->_noc_ifc_send[INTRA_PRED_ID].get_packet(_vnoc->get_packet_injected_num("INTRA_PRED"));
                _vnoc->_event_queue->add_event( EVENT(EVENT::INTRA_PRED_MB_FRAME_TO_INTER_PRED,_vnoc->_event_queue->current_sim_time()+ delay, DISPLAY, my_packet));

            }

//INTER_PRED_NORMAL
            if (_event_mode_received == DECODE_MB_TO_INTER_PRED_NORMAL)      
            {                    

                //screen=SDL_SetVideoMode(192,144,BitsPerPixel,SDL_HWSURFACE);

                MB_frame mb_f;                
                //NOW we are in INTER_PRED 
                // printf ("_event_mode_received == DECODE_MB_TO_INTER_PRED_NORMAL\n");                
                // printf ("_noc_ifc_receive[]._ID = %d\n",_ID);     
                // printf ("_noc_ifc_send[]._ID = %d\n",INTER_PRED_ID);     
 
                _vnoc->_noc_ifc_receive[INTER_PRED_ID].depacketize("DECODE_MB_TO_INTER_PRED_NORMAL");
                // printf ("depacketizing is finished\n");
                _vnoc->_inter_pred_PE.run_inter_pred_normal_MB();//produces MB_frame
                // printf("INTER_PRED_NORMAL\n");
                //cin.get();
 
                //showframe(screen,&(_vnoc->_inter_pred_PE.return_frame()),1);
 
                //cin.get();
            
                _vnoc->_noc_ifc_send[INTER_PRED_ID].packetize("INTER_PRED_MB_FRAME_TO_INTRA_PRED");
                //_vnoc->_noc_ifc_send[INTER_PRED_ID].packet_array_display();                

                cout <<"time= "<<time<<endl;
                cout <<"event_current_sim_time= "<<_vnoc->_event_queue->current_sim_time()<<endl;            

                double delay = IP_CORE_DELAY;

              
                my_packet =_vnoc->_noc_ifc_send[INTER_PRED_ID].get_packet(_vnoc->get_packet_injected_num("INTER_PRED"));
                _vnoc->_event_queue->add_event( EVENT(EVENT::INTER_PRED_MB_FRAME_TO_INTRA_PRED,_vnoc->_event_queue->current_sim_time()+ delay, DISPLAY, my_packet));

            }
//INTER_PRED_PSKIP
            if (_event_mode_received == DECODE_MB_TO_INTER_PRED_PSKIP)      
            {                    

                //screen=SDL_SetVideoMode(192,144,BitsPerPixel,SDL_HWSURFACE);

                MB_frame mb_f;                
                //NOW we are in INTER_PRED 
                // printf ("_event_mode_received == DECODE_MB_TO_INTER_PRED_PSKIP\n");                
                // printf ("_noc_ifc_receive[]._ID = %d\n",_ID);     
                // printf ("_noc_ifc_send[]._ID = %d\n",INTER_PRED_ID);     
 
                _vnoc->_noc_ifc_receive[INTER_PRED_ID].depacketize("DECODE_MB_TO_INTER_PRED_PSKIP");
                // printf ("depacketizing is finished\n");
                _vnoc->_inter_pred_PE.run_inter_pred_pskip_MB();//produces MB_frame
                // printf("INTER_PRED_PSKIP\n");
                //cin.get();
                
                //showframe(screen,&(_vnoc->_inter_pred_PE.return_frame()),1);
 
                //cin.get();
            
                _vnoc->_noc_ifc_send[INTER_PRED_ID].packetize("INTER_PRED_MB_FRAME_TO_INTRA_PRED");
                //_vnoc->_noc_ifc_send[INTER_PRED_ID].packet_array_display();                

                cout <<"time= "<<time<<endl;
                cout <<"event_current_sim_time= "<<_vnoc->_event_queue->current_sim_time()<<endl;            

                double delay = IP_CORE_DELAY;

              
                my_packet =_vnoc->_noc_ifc_send[INTER_PRED_ID].get_packet(_vnoc->get_packet_injected_num("INTER_PRED"));
                _vnoc->_event_queue->add_event( EVENT(EVENT::INTER_PRED_MB_FRAME_TO_INTRA_PRED,_vnoc->_event_queue->current_sim_time()+ delay, DISPLAY, my_packet));

            }


            if (_event_mode_received == INTER_PRED_MB_FRAME_TO_INTRA_PRED )      
            {                    

                //screen=SDL_SetVideoMode(192,144,BitsPerPixel,SDL_HWSURFACE);                               
                //NOW we are in INTRA_PRED 
                // printf ("_event_mode_received == INTER_PRED_MB_FRAME_TO_INTRA_PRED \n");                
                // printf ("_noc_ifc_receive[]._ID = %d\n",_ID);     
                // printf ("_noc_ifc_send[]._ID = %d\n",INTRA_PRED_ID);     
                
                
                _vnoc->_noc_ifc_receive[INTRA_PRED_ID].depacketize("INTER_PRED_MB_FRAME_TO_INTRA_PRED");//sets _received_frame and it's x and y in private variables
                // printf ("depacketizing is finished\n");
                //showframe(screen,&(_vnoc->_intra_pred_PE.return_frame()),1);
                //cin.get();
                
                _vnoc->_intra_pred_PE.update_frame_with_received_MB_frame();

                //showframe(screen,&(_vnoc->_intra_pred_PE.return_frame()),1);                                
                // printf("new frame from intra pred\n");
                
                //cin.get();
                _vnoc->_noc_ifc_send[INTRA_PRED_ID].packetize("INTRA_PRED_MB_FRAME_TO_FRAME_BUFFER");

                //_vnoc->_noc_ifc_send[INTRA_PRED_ID].packet_array_display();
                //cin.get();

                cout <<"time= "<<time<<endl;
                cout <<"event_current_sim_time= "<<_vnoc->_event_queue->current_sim_time()<<endl;

                double delay = IP_CORE_DELAY;                
                my_packet =_vnoc->_noc_ifc_send[INTRA_PRED_ID].get_packet(_vnoc->get_packet_injected_num("INTRA_PRED"));
                _vnoc->_event_queue->add_event( EVENT(EVENT::INTRA_PRED_MB_FRAME_TO_FRAME_BUFFER,_vnoc->_event_queue->current_sim_time()+ delay, DISPLAY, my_packet));               
            }
//FRAME BUFFER
            if (_event_mode_received == INTRA_PRED_MB_FRAME_TO_FRAME_BUFFER  )      
            {                    

                //screen=SDL_SetVideoMode(192,144,BitsPerPixel,SDL_HWSURFACE);                               
                //NOW we are in  FRAME_BUFFER
                // printf ("_event_mode_received == INTRA_PRED_MB_FRAME_TO_FRAME_BUFFER \n");                
                // printf ("_noc_ifc_receive[]._ID = %d\n",_ID);     
                // printf ("_noc_ifc_send[]._ID = %d\n", FRAME_BUFFER_ID);     
                //cin.get();
                
                _vnoc->_noc_ifc_receive[FRAME_BUFFER_ID].depacketize("INTRA_PRED_MB_FRAME_TO_FRAME_BUFFER");//sets _received_frame and it's x and y in private variables
                // printf ("depacketizing is finished\n");
                //showframe(screen,&(_vnoc->_frame_buffer_PE.return_frame()),1);
                //cin.get();
                //cin.get();
                
                _vnoc->_frame_buffer_PE.update_frame_with_received_MB_frame();

                //showframe(screen,&(_vnoc->_intra_pred_PE.return_frame()),1);                                
                // printf("new frame from frame_buffer\n");
                
                //cin.get();
                _vnoc->_noc_ifc_send[FRAME_BUFFER_ID].packetize("FRAME_BUFFER_MB_FRAME_TO_DISPLAY");

                //_vnoc->_noc_ifc_send[FRAME_BUFFER_ID].packet_array_display();
                //cin.get();

                cout <<"time= "<<time<<endl;
                cout <<"event_current_sim_time= "<<_vnoc->_event_queue->current_sim_time()<<endl;

                double delay = IP_CORE_DELAY;                
                my_packet =_vnoc->_noc_ifc_send[FRAME_BUFFER_ID].get_packet(_vnoc->get_packet_injected_num("FRAME_BUFFER"));
                _vnoc->_event_queue->add_event( EVENT(EVENT::FRAME_BUFFER_MB_FRAME_TO_DISPLAY,_vnoc->_event_queue->current_sim_time()+ delay, DISPLAY, my_packet));
            }






            if (_event_mode_received == INTRA_PRED_MB_FRAME_TO_INTER_PRED )      
            {                    

                //screen=SDL_SetVideoMode(192,144,BitsPerPixel,SDL_HWSURFACE);                               
                //NOW we are in INTER_PRED 
                // printf ("_event_mode_received == INTRA_PRED_MB_FRAME_TO_INTER_PRED \n");                
                // printf ("_noc_ifc_receive[]._ID = %d\n",_ID);     
                // printf ("_noc_ifc_send[]._ID = %d\n",INTER_PRED_ID);     
                
                
                _vnoc->_noc_ifc_receive[INTER_PRED_ID].depacketize("INTRA_PRED_MB_FRAME_TO_INTER_PRED");//sets _received_frame and it's x and y in private variables
                // printf ("depacketizing is finished\n");
                //showframe(screen,&(_vnoc->_inter_pred_PE.return_frame()),1);
                //cin.get();
                
                _vnoc->_inter_pred_PE.update_frame_with_received_MB_frame();

                //showframe(screen,&(_vnoc->_inter_pred_PE.return_frame()),1);                                
                // printf("new frame from inter pred\n");
                
                //cin.get();
                _vnoc->_noc_ifc_send[INTER_PRED_ID].packetize("INTER_PRED_MB_FRAME_TO_FRAME_BUFFER");

                //_vnoc->_noc_ifc_send[INTER_PRED_ID].packet_array_display();
                //cin.get();

                cout <<"time= "<<time<<endl;
                cout <<"event_current_sim_time= "<<_vnoc->_event_queue->current_sim_time()<<endl;

                double delay = IP_CORE_DELAY;                
                my_packet =_vnoc->_noc_ifc_send[INTER_PRED_ID].get_packet(_vnoc->get_packet_injected_num("INTER_PRED"));
                _vnoc->_event_queue->add_event( EVENT(EVENT::INTER_PRED_MB_FRAME_TO_FRAME_BUFFER,_vnoc->_event_queue->current_sim_time()+ delay, DISPLAY, my_packet));               
            }

//FRAME_BUFFER from inter            
            if (_event_mode_received == INTER_PRED_MB_FRAME_TO_FRAME_BUFFER  )      
            {                    

                //screen=SDL_SetVideoMode(192,144,BitsPerPixel,SDL_HWSURFACE);                               
                //NOW we are in  FRAME_BUFFER
                // printf ("_event_mode_received == INTER_PRED_MB_FRAME_TO_FRAME_BUFFER \n");                
                // printf ("_noc_ifc_receive[]._ID = %d\n",_ID);     
                // printf ("_noc_ifc_send[]._ID = %d\n", FRAME_BUFFER_ID);     
                //cin.get();
                
                _vnoc->_noc_ifc_receive[FRAME_BUFFER_ID].depacketize("INTER_PRED_MB_FRAME_TO_FRAME_BUFFER");//sets _received_frame and it's x and y in private variables
                // printf ("depacketizing is finished\n");
                //showframe(screen,&(_vnoc->_frame_buffer_PE.return_frame()),1);
                //cin.get();
                //cin.get();
                
                _vnoc->_frame_buffer_PE.update_frame_with_received_MB_frame();

                //showframe(screen,&(_vnoc->_inter_pred_PE.return_frame()),1);                                
                // printf("new frame from frame_buffer\n");
                
                //cin.get();
                _vnoc->_noc_ifc_send[FRAME_BUFFER_ID].packetize("FRAME_BUFFER_MB_FRAME_TO_DISPLAY");

                //_vnoc->_noc_ifc_send[FRAME_BUFFER_ID].packet_array_display();
                //cin.get();

                cout <<"time= "<<time<<endl;
                cout <<"event_current_sim_time= "<<_vnoc->_event_queue->current_sim_time()<<endl;

                double delay = IP_CORE_DELAY;                
                my_packet =_vnoc->_noc_ifc_send[FRAME_BUFFER_ID].get_packet(_vnoc->get_packet_injected_num("FRAME_BUFFER"));
                _vnoc->_event_queue->add_event( EVENT(EVENT::FRAME_BUFFER_MB_FRAME_TO_DISPLAY,_vnoc->_event_queue->current_sim_time()+ delay, DISPLAY, my_packet));
            }
//DISPLAY
            if (_event_mode_received == FRAME_BUFFER_MB_FRAME_TO_DISPLAY )      
            {
                screen=SDL_SetVideoMode(WIDTH,HEIGHT,BitsPerPixel,SDL_HWSURFACE);                               
                //NOW we are in  FRAME_BUFFER
                // printf ("_event_mode_received == FRAME_BUFFER_MB_FRAME_TO_DISPLAY \n");                
                // printf ("_noc_ifc_receive[]._ID = %d\n",_ID);     
                // printf ("_noc_ifc_send[]._ID = %d\n", DISPLAY_ID);     
                //cin.get();
                
                _vnoc->_noc_ifc_receive[DISPLAY_ID].depacketize("FRAME_BUFFER_MB_FRAME_TO_DISPLAY");//sets _received_frame and it's x and y in private variables
                // printf ("depacketizing is finished\n");
                _vnoc->_display_PE.update_frame_with_received_MB_frame();
                showframe(screen,&(_vnoc->_display_PE.return_frame()),1);
                //cin.get();
                //cin.get();                

                //showframe(screen,&(_vnoc->_inter_pred_PE.return_frame()),1);                                
                // printf("new frame from display\n");
                
                //cin.get();

                cout <<"time= "<<time<<endl;
                cout <<"event_current_sim_time= "<<_vnoc->_event_queue->current_sim_time()<<endl;
                
                _vnoc->_noc_ifc_send[DISPLAY_ID].packetize("TO_CONTINUE_DECODE_MB");//send new MB number that should be decoded
                my_packet =_vnoc->_noc_ifc_send[DISPLAY_ID].get_packet(_vnoc->get_packet_injected_num("FRAME_BUFFER"));
                _vnoc->_event_queue->add_event( EVENT(EVENT::TO_CONTINUE_DECODE_MB,_vnoc->_event_queue->current_sim_time()+ delay, DISPLAY, my_packet));                
                
                //_vnoc->_noc_ifc_send[DISPLAY_ID].packet_array_display();
                //cin.get();

                
            }


            if (_event_mode_received == TO_CONTINUE_DECODE_MB)      
            {
                //NOW we are in DECODE_MB
                // printf ("_event_mode_received == TO_CONTINUE_DECODE_MB\n");                         
                // printf ("_noc_ifc_receive[]._ID = %d\n",_ID);     
                // printf ("_noc_ifc_send[]._ID = %d\n",DECODE_MB_ID);            

                //no depacketizing
                
                if (_vnoc->_decode_MB_PE.run_decode_MicroBlock())
                {                        
                //based on the trigger_event by decode_MB we have to
                //packetize for INTRA_PRED and call INTRA_PRED event
                //or
                //packetize for INTER_PRED and call INTER_PRED event
                
                    if (_vnoc->_decode_MB_PE.return_trigger_event() ==TRIGGER_INTRA_PRED)
                    {                        
                        // printf ("TRIGGER EVENT = INTRA_PRED\n");
                        _vnoc->_noc_ifc_send[DECODE_MB_ID].packetize("DECODE_MB_TO_INTRA_PRED");
                        my_packet =_vnoc->_noc_ifc_send[DECODE_MB_ID].get_packet(_vnoc->get_packet_injected_num("DECODE_MB"));
                        _vnoc->_event_queue->add_event( EVENT(EVENT::DECODE_MB_TO_INTRA_PRED,_vnoc->_event_queue->current_sim_time()+ delay, DISPLAY, my_packet));                                        
                    }
                
                    if (_vnoc->_decode_MB_PE.return_trigger_event() == TRIGGER_INTER_PRED_NORMAL)
                    {                        
                        // printf ("TRIGGER EVENT = TRIGGER_INTER_PRED_NORMAL\n");
                        _vnoc->_noc_ifc_send[DECODE_MB_ID].packetize("DECODE_MB_TO_INTER_PRED_NORMAL");
                        my_packet =_vnoc->_noc_ifc_send[DECODE_MB_ID].get_packet(_vnoc->get_packet_injected_num("DECODE_MB"));
                        _vnoc->_event_queue->add_event( EVENT(EVENT::DECODE_MB_TO_INTER_PRED_NORMAL,_vnoc->_event_queue->current_sim_time()+ delay, DISPLAY, my_packet));                                    
                    }

                    if (_vnoc->_decode_MB_PE.return_trigger_event() == TRIGGER_INTER_PRED_PSKIP)
                    {                        
                        // printf ("TRIGGER EVENT = TRIGGER_INTER_PRED_PSKIP\n");
                        _vnoc->_noc_ifc_send[DECODE_MB_ID].packetize("DECODE_MB_TO_INTER_PRED_PSKIP");
                        my_packet =_vnoc->_noc_ifc_send[DECODE_MB_ID].get_packet(_vnoc->get_packet_injected_num("DECODE_MB"));
                        _vnoc->_event_queue->add_event( EVENT(EVENT::DECODE_MB_TO_INTER_PRED_PSKIP,_vnoc->_event_queue->current_sim_time()+ delay, DISPLAY, my_packet));                                        
                    }             
                }
                else
                {                    
                    // printf("_decode_MB_PE.run_decode_MicroBlock() = false, CurrMbAddress == MbCount\n");
                    //add get_nalu event;                   
                    //update ref
                    _vnoc->_noc_ifc_send[DECODE_MB_ID].packetize("UPDATE_REF_FRAME_IN_INTER_PRED");
                    my_packet =_vnoc->_noc_ifc_send[DECODE_MB_ID].get_packet(_vnoc->get_packet_injected_num("DECODE_MB"));
                    _vnoc->_event_queue->add_event( EVENT(EVENT::UPDATE_REF_FRAME_IN_INTER_PRED,_vnoc->_event_queue->current_sim_time()+ delay, DISPLAY, my_packet));                                        
                    _vnoc->update_and_print_simulation_results();
                    //cin.get();
 
                }

            } //if "TO_CONTINUE_DECODE_MB"

//UPDATE_REF_FRAME_IN_INTER_PRED
            if (_event_mode_received == UPDATE_REF_FRAME_IN_INTER_PRED)      
            {
                                               
                //NOW we are in  INTER_PRED
                // printf ("_event_mode_received ==  \n");                
                // printf ("_noc_ifc_receive[]._ID = %d\n",_ID);     
                // printf ("_noc_ifc_send[]._ID = %d\n", INTER_PRED_ID);                
                cout <<"time= "<<time<<endl;
                cout <<"event_current_sim_time= "<<_vnoc->_event_queue->current_sim_time()<<endl;
                //update ref
                _vnoc->_inter_pred_PE.update_ref();

                //temporary: reset frame_buffer_in display
                _vnoc->_display_PE.reset_this_frame();
                
                //showframe(screen,&(_vnoc->_inter_pred_PE.return_frame()),1);
                
                _vnoc->_noc_ifc_send[INTER_PRED_ID].packetize("UPDATE_FRAME_IN_FRAME_BUFFER");
                my_packet =_vnoc->_noc_ifc_send[INTER_PRED_ID].get_packet(_vnoc->get_packet_injected_num("INTER_PRED"));
                _vnoc->_event_queue->add_event( EVENT(EVENT::UPDATE_FRAME_IN_FRAME_BUFFER,_vnoc->_event_queue->current_sim_time()+ delay, DISPLAY, my_packet));                
                
                //_vnoc->_noc_ifc_send[DISPLAY_ID].packet_array_display();
                //cin.get();
            }


            if (_event_mode_received == UPDATE_FRAME_IN_FRAME_BUFFER)      
            {
                                               
                //NOW we are in  FRAME_BUFFER
                // printf ("_event_mode_received ==  \n");                
                // printf ("_noc_ifc_receive[]._ID = %d\n",_ID);     
                // printf ("_noc_ifc_send[]._ID = %d\n", FRAME_BUFFER_ID);                
                cout <<"time= "<<time<<endl;
                cout <<"event_current_sim_time= "<<_vnoc->_event_queue->current_sim_time()<<endl;
                //update buffer
                _vnoc->_frame_buffer_PE.update_buffer();

                _vnoc->_noc_ifc_send[FRAME_BUFFER_ID].packetize("TO_GET_NALU");
                my_packet =_vnoc->_noc_ifc_send[FRAME_BUFFER_ID].get_packet(_vnoc->get_packet_injected_num("FRAME_BUFFER"));
                _vnoc->_event_queue->add_event( EVENT(EVENT::TO_GET_NALU,_vnoc->_event_queue->current_sim_time()+ delay, DISPLAY, my_packet));                
                
                //_vnoc->_noc_ifc_send[DISPLAY_ID].packet_array_display();
                //cin.get();
            }


            if (_event_mode_received == TO_GET_NALU )      
            {
                                               
                //NOW we are in  GET_NALU
                // printf ("_event_mode_received ==  \n");                
                // printf ("_noc_ifc_receive[]._ID = %d\n",_ID);     
                // printf ("_noc_ifc_send[]._ID = %d\n", GET_NALU_ID);                
                cout <<"time= "<<time<<endl;
                cout <<"event_current_sim_time= "<<_vnoc->_event_queue->current_sim_time()<<endl;
                
                //.............................
                _vnoc->frame_number_plus_plus();                
                _vnoc->_nalu_PE.get_nalu(false);
                _vnoc->_noc_ifc_send[GET_NALU_ID].packetize("GET_NALU_TO_DECODE_HDR_FIRST_RUN");
                my_packet =_vnoc->_noc_ifc_send[GET_NALU_ID].get_packet(_vnoc->get_packet_injected_num("GET_NALU"));
                _vnoc->_event_queue->add_event( EVENT(EVENT::GET_NALU_TO_DECODE_HDR_FIRST_RUN,_vnoc->_event_queue->current_sim_time()+ delay, DISPLAY, my_packet));                

            }



            _packets_ready=false;            
    }        
        
}


void NOC_interface_receive::packet_array_display()
{
    // printf ("packet_array_display(), _packet_num = %d\n" , _packet_num);
    
    for (int i=0;i<_packet_num;i++){            
        cout <<"packet["<<i<<"]:"<<endl;            
                
        for(int j=0;j<_packet_array[i].size();j++)
            cout <<_packet_array[i][j]<<"   ";
        cout <<endl;        
    }
    
}

void NOC_interface_receive::build_pic()
{ 
    
    _pic.size_x=_packet_array[0][0];    
    _pic.size_y=_packet_array[0][1];  
    
    cout <<"_pic.size_x= "<<_pic.size_x<<endl;    
    cout <<"_pic.size_y= "<<_pic.size_y<<endl;

    cout << "1: build_pic()"<<endl;

    _pic.imgUV = (imgpel ***)malloc(2 * sizeof(imgpel **));
    _pic.imgUV[0] = (imgpel **)malloc(_pic.size_y/2 * sizeof(imgpel *));
    _pic.imgUV[1] = (imgpel **)malloc(_pic.size_y/2 * sizeof(imgpel *));
    _pic.imgY = (imgpel **)malloc(_pic.size_y * sizeof(imgpel *));

    for (int y=0;y<_pic.size_y;y++){                
        _pic.imgY[y] = (imgpel *)malloc(_pic.size_x * sizeof(imgpel));            
        _pic.imgY[y] = (imgpel *)malloc(_pic.size_x * sizeof(imgpel));
        if (y%2==0){
            _pic.imgUV[0][y/2] = (imgpel *)malloc(_pic.size_x/2 * sizeof(imgpel));
            _pic.imgUV[1][y/2] = (imgpel *)malloc(_pic.size_x/2 * sizeof(imgpel));                       
        }
    }
    cout << "2: build_pic()"<<endl;

    int packet_num=0;  
    long pixels=_pic.size_x*_pic.size_y;    
    long offset=2;
    int X=-1;
    int Y=0;
    
    for (long i=offset;i<offset+pixels;i++)
    {     

        if (i%_packet_size==0 && i!=0)
            packet_num++;            
        
        X++;
        if (X==_pic.size_x){
            X=0;
            Y++;
        } 
        
        _pic.imgY[Y][X]=_packet_array[packet_num][i%_packet_size];

    
    }

    X=-1;
    Y=0;    
    offset = pixels+2;        
    for (i=offset;i<offset+pixels/4;i++)
    {            
        if (i%_packet_size==0 && i!=0)
            packet_num++;

        X++;    
        if (X==_pic.size_x/2){
            X=0;
            Y++;
        }        
        _pic.imgUV[0][Y][X]=_packet_array[packet_num][i%_packet_size];
    }
    

    X=-1;
    Y=0;
    offset = pixels+2+pixels/4;        
    for (i=offset;i<offset+pixels/4;i++)
    {            
        if (i%_packet_size==0 && i!=0 )//&& packet_num<5
            packet_num++;

        X++;    
        if (X==_pic.size_x/2){
            X=0;
            Y++;
        }        
        //cout <<"i="<<i<<" packet_num= "<<packet_num<<endl;        
        _pic.imgUV[1][Y][X]=_packet_array[packet_num][i%_packet_size];
                    
    }
}

    
//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
void NOC_interface_send::add_sps_flit_data(seq_parameter_set sps)
{
    add_flit_data(sps.profile_idc);
    add_flit_data(sps.constraint_set0_flag);
    add_flit_data(sps.constraint_set1_flag);        
    add_flit_data(sps.constraint_set2_flag);
    add_flit_data(sps.reserved_zero_5bits);
    add_flit_data(sps.level_idc);        
    add_flit_data(sps.seq_parameter_set_id);
    add_flit_data(sps.log2_max_frame_num);
    add_flit_data(sps.MaxFrameNum);        
    add_flit_data(sps.pic_order_cnt_type);
    add_flit_data(sps.log2_max_pic_order_cnt_lsb);
    add_flit_data(sps.MaxPicOrderCntLsb);        
    add_flit_data(sps.delta_pic_order_always_zero_flag);
    add_flit_data(sps.offset_for_non_ref_pic);
    add_flit_data(sps.offset_for_top_to_bottom_field);        
    add_flit_data(sps.num_ref_frames_in_pic_order_cnt_cycle);
    for (int i=0;i<256;i++)
        add_flit_data(sps.offset_for_ref_frame[i]);
    add_flit_data(sps.num_ref_frames);        
    add_flit_data(sps.gaps_in_frame_num_value_allowed_flag);
    add_flit_data(sps.PicWidthInMbs);
    add_flit_data(sps.PicWidthInSamples);        
    add_flit_data(sps.PicHeightInMapUnits);
    add_flit_data(sps.PicSizeInMapUnits);
    add_flit_data(sps.FrameHeightInMbs);
    add_flit_data(sps.FrameHeightInSamples);
    add_flit_data(sps.frame_mbs_only_flag);
    add_flit_data(sps.mb_adaptive_frame_field_flag);
    add_flit_data(sps.direct_8x8_inference_flag);
    add_flit_data(sps.frame_cropping_flag);
    add_flit_data(sps.frame_crop_left_offset);
    add_flit_data(sps.frame_crop_right_offset);
    add_flit_data(sps.frame_crop_top_offset);
    add_flit_data(sps.frame_crop_bottom_offset);
    add_flit_data(sps.vui_parameters_present_flag);
}

void NOC_interface_send::add_pps_flit_data(pic_parameter_set pps)
{
    add_flit_data(pps.pic_parameter_set_id);
    add_flit_data(pps.seq_parameter_set_id);
    add_flit_data(pps.entropy_coding_mode_flag);
    add_flit_data(pps.pic_order_present_flag);
    add_flit_data(pps.num_slice_groups);
    add_flit_data(pps.slice_group_map_type);
    for (int i=0;i<8;i++)
        add_flit_data(pps.run_length[i]);
    for (int i=0;i<8;i++)
        add_flit_data(pps.top_left[i]);
    for (int i=0;i<8;i++)
        add_flit_data(pps.bottom_right[i]);
    add_flit_data(pps.slice_group_change_direction_flag);
    add_flit_data(pps.SliceGroupChangeRate);
    add_flit_data(pps.pic_size_in_map_units);
    for (int i=0;i<8192;i++)
        add_flit_data(pps.slice_group_id[i]);
    add_flit_data(pps.num_ref_idx_l0_active);
    add_flit_data(pps.num_ref_idx_l1_active);
    add_flit_data(pps.weighted_pred_flag);
    add_flit_data(pps.weighted_bipred_idc);
    add_flit_data(pps.pic_init_qp);
    add_flit_data(pps.pic_init_qs);
    add_flit_data(pps.chroma_qp_index_offset);
    add_flit_data(pps.deblocking_filter_control_present_flag);
    add_flit_data(pps.constrained_intra_pred_flag);
    add_flit_data(pps.redundant_pic_cnt_present_flag);
}
////////////////////////////////////////////////////////////
pic_parameter_set NOC_interface_receive::get_pps_flit_data()
{
    pic_parameter_set pps;    
    pps.pic_parameter_set_id=get_flit_data();
    pps.seq_parameter_set_id=get_flit_data();
    pps.entropy_coding_mode_flag=get_flit_data();
    pps.pic_order_present_flag=get_flit_data();
    pps.num_slice_groups=get_flit_data();
    pps.slice_group_map_type=get_flit_data();
    for (int i=0;i<8;i++)
        pps.run_length[i]=get_flit_data();
    for (int i=0;i<8;i++)
        pps.top_left[i]=get_flit_data();
    for (int i=0;i<8;i++)
        pps.bottom_right[i]=get_flit_data();
    pps.slice_group_change_direction_flag=get_flit_data();
    pps.SliceGroupChangeRate=get_flit_data();
    pps.pic_size_in_map_units=get_flit_data();
    for (int i=0;i<8192;i++)
        pps.slice_group_id[i]=get_flit_data();
    pps.num_ref_idx_l0_active=get_flit_data();
    pps.num_ref_idx_l1_active=get_flit_data();
    pps.weighted_pred_flag=get_flit_data();
    pps.weighted_bipred_idc=get_flit_data();
    pps.pic_init_qp=get_flit_data();
    pps.pic_init_qs=get_flit_data();
    pps.chroma_qp_index_offset=get_flit_data();
    pps.deblocking_filter_control_present_flag=get_flit_data();
    pps.constrained_intra_pred_flag=get_flit_data();
    pps.redundant_pic_cnt_present_flag=get_flit_data();
    return pps;    
}
/////////////////////////////////////////////////////////////
seq_parameter_set NOC_interface_receive::get_sps_flit_data()
{
    seq_parameter_set sps;   
    sps.profile_idc=get_flit_data();
    sps.constraint_set0_flag=get_flit_data();
    sps.constraint_set1_flag=get_flit_data();        
    sps.constraint_set2_flag=get_flit_data();
    sps.reserved_zero_5bits=get_flit_data();
    sps.level_idc=get_flit_data();        
    sps.seq_parameter_set_id=get_flit_data();
    sps.log2_max_frame_num=get_flit_data();
    sps.MaxFrameNum=get_flit_data();        
    sps.pic_order_cnt_type=get_flit_data();
    sps.log2_max_pic_order_cnt_lsb=get_flit_data();
    sps.MaxPicOrderCntLsb=get_flit_data();        
    sps.delta_pic_order_always_zero_flag=get_flit_data();
    sps.offset_for_non_ref_pic=get_flit_data();
    sps.offset_for_top_to_bottom_field=get_flit_data();        
    sps.num_ref_frames_in_pic_order_cnt_cycle=get_flit_data();
    for (int i=0;i<256;i++)
        sps.offset_for_ref_frame[i]=get_flit_data();
    sps.num_ref_frames=get_flit_data();        
    sps.gaps_in_frame_num_value_allowed_flag=get_flit_data();
    sps.PicWidthInMbs=get_flit_data();
    sps.PicWidthInSamples=get_flit_data();        
    sps.PicHeightInMapUnits=get_flit_data();
    sps.PicSizeInMapUnits=get_flit_data();
    sps.FrameHeightInMbs=get_flit_data();
    sps.FrameHeightInSamples=get_flit_data();
    sps.frame_mbs_only_flag=get_flit_data();
    sps.mb_adaptive_frame_field_flag=get_flit_data();
    sps.direct_8x8_inference_flag=get_flit_data();
    sps.frame_cropping_flag=get_flit_data();
    sps.frame_crop_left_offset=get_flit_data();
    sps.frame_crop_right_offset=get_flit_data();
    sps.frame_crop_top_offset=get_flit_data();
    sps.frame_crop_bottom_offset=get_flit_data();
    sps.vui_parameters_present_flag=get_flit_data();
    return sps;    
}
///////////////////////////////////////////////////////////////
void NOC_interface_send::add_nalu_flit_data(nal_unit nalu)
{
    add_flit_data(nalu.NumBytesInNALunit);
    add_flit_data(nalu.forbidden_zero_bit);
    add_flit_data(nalu.nal_ref_idc);    
    add_flit_data(nalu.nal_unit_type);
    add_flit_data(nalu.last_rbsp_byte);
}

///////////////////////////////////////////////////////////////
nal_unit NOC_interface_receive::get_nalu_flit_data()
{
    nal_unit nalu;    
    DATA_ELEMENT data;    
    nalu.NumBytesInNALunit=get_flit_data();
    nalu.forbidden_zero_bit=get_flit_data();
    nalu.nal_ref_idc=get_flit_data();    
    nalu.nal_unit_type=get_flit_data();    
    nalu.last_rbsp_byte=get_flit_data();
    return nalu;       
}
///////////////////////////////////////////////////////////////
void NOC_interface_send::add_sh_flit_data(slice_header sh)
{
    add_flit_data(sh.first_mb_in_slice);
    add_flit_data(sh.slice_type);
    add_flit_data(sh.pic_parameter_set_id);
    add_flit_data(sh.frame_num);
    add_flit_data(sh.field_pic_flag);
    add_flit_data(sh.MbaffFrameFlag);
    add_flit_data(sh.PicHeightInMbs);
    add_flit_data(sh.PicHeightInSamples);
    add_flit_data(sh.PicSizeInMbs);
    add_flit_data(sh.bottom_field_flag);
    add_flit_data(sh.idr_pic_id);
    add_flit_data(sh.pic_order_cnt_lsb);
    add_flit_data(sh.delta_pic_order_cnt_bottom);
    for (int i=0; i<2;i++)
        add_flit_data(sh.delta_pic_order_cnt[i]);
    add_flit_data(sh.redundant_pic_cnt);
    add_flit_data(sh.direct_spatial_mv_pred_flag);
    add_flit_data(sh.num_ref_idx_active_override_flag);
    add_flit_data(sh.num_ref_idx_l0_active);
    add_flit_data(sh.num_ref_idx_l1_active);
    add_flit_data(sh.ref_pic_list_reordering_flag_l0);
    add_flit_data(sh.ref_pic_list_reordering_flag_l1);
    add_flit_data(sh.no_output_of_prior_pics_flag);
    add_flit_data(sh.long_term_reference_flag);
    add_flit_data(sh.adaptive_ref_pic_marking_mode_flag);
    add_flit_data(sh.cabac_init_idc);
    add_flit_data(sh.slice_qp_delta);
    add_flit_data(sh.SliceQPy);
    add_flit_data(sh.sp_for_switch_flag);
    add_flit_data(sh.slice_qs_delta);
    add_flit_data(sh.disable_deblocking_filter_idc);
    add_flit_data(sh.slice_alpha_c0_offset_div2);
    add_flit_data(sh.slice_beta_offset_div2);
    add_flit_data(sh.slice_group_change_cycle);    
}

///////////////////////////////////////////////////////////////
slice_header NOC_interface_receive::get_sh_flit_data()
{
    slice_header sh;
    
    sh.first_mb_in_slice=get_flit_data();
    sh.slice_type=get_flit_data();
    sh.pic_parameter_set_id=get_flit_data();
    sh.frame_num=get_flit_data();
    sh.field_pic_flag=get_flit_data();
    sh.MbaffFrameFlag=get_flit_data();
    sh.PicHeightInMbs=get_flit_data();
    sh.PicHeightInSamples=get_flit_data();
    sh.PicSizeInMbs=get_flit_data();
    sh.bottom_field_flag=get_flit_data();
    sh.idr_pic_id=get_flit_data();
    sh.pic_order_cnt_lsb=get_flit_data();
    sh.delta_pic_order_cnt_bottom=get_flit_data();
    for (int i=0; i<2;i++)
        sh.delta_pic_order_cnt[i]=get_flit_data();
    sh.redundant_pic_cnt=get_flit_data();
    sh.direct_spatial_mv_pred_flag=get_flit_data();
    sh.num_ref_idx_active_override_flag=get_flit_data();
    sh.num_ref_idx_l0_active=get_flit_data();
    sh.num_ref_idx_l1_active=get_flit_data();
    sh.ref_pic_list_reordering_flag_l0=get_flit_data();
    sh.ref_pic_list_reordering_flag_l1=get_flit_data();
    sh.no_output_of_prior_pics_flag=get_flit_data();
    sh.long_term_reference_flag=get_flit_data();
    sh.adaptive_ref_pic_marking_mode_flag=get_flit_data();
    sh.cabac_init_idc=get_flit_data();
    sh.slice_qp_delta=get_flit_data();
    sh.SliceQPy=get_flit_data();
    sh.sp_for_switch_flag=get_flit_data();
    sh.slice_qs_delta=get_flit_data();
    sh.disable_deblocking_filter_idc=get_flit_data();
    sh.slice_alpha_c0_offset_div2=get_flit_data();
    sh.slice_beta_offset_div2=get_flit_data();
    sh.slice_group_change_cycle=get_flit_data();    

    return sh;    
}

//////////////////////////////////////////////////
void NOC_interface_send::add_luma_chroma_DC_AC_flit_data(int LumaDCLevel[16], int LumaACLevel[16][16], int ChromaDCLevel[2][4], int ChromaACLevel[2][4][16])
{
    for(int i=0;i<16;i++)
        add_flit_data(LumaDCLevel[i]);
    for (int i=0;i<16;i++)
        for (int j=0;j<16;j++)
            add_flit_data(LumaACLevel[i][j]);       
    for (int i=0;i<2;i++)
        for (int j=0;j<4;j++)
            add_flit_data(ChromaDCLevel[i][j]);
    for (int i=0;i<2;i++)
        for (int j=0;j<4;j++)
            for (int k=0;k<16;k++)
                add_flit_data(ChromaACLevel[i][j][k]);
}

////////////////////////////////////////////////////

void NOC_interface_receive::get_luma_chroma_DC_AC_flit_data(int LumaDCLevel[16], int LumaACLevel[16][16], int ChromaDCLevel[2][4], int ChromaACLevel[2][4][16])
{
    for(int i=0;i<16;i++)
        LumaDCLevel[i]=get_flit_data();
    for (int i=0;i<16;i++)
        for (int j=0;j<16;j++)
            LumaACLevel[i][j]=get_flit_data();       
    for (int i=0;i<2;i++)
        for (int j=0;j<4;j++)
            ChromaDCLevel[i][j]=get_flit_data();
    for (int i=0;i<2;i++)
        for (int j=0;j<4;j++)
            for (int k=0;k<16;k++)
               ChromaACLevel[i][j][k]=get_flit_data();
}

//////////////////////////////////////////////////
void NOC_interface_send::add_mpi_flit_data(mode_pred_info *mpi)
{
    add_flit_data(mpi->MbHeight);
    add_flit_data(mpi->MbWidth);
    add_flit_data(mpi->MbPitch);
    for (int i=0; i<mpi->MbHeight; i++)
        for (int j=0; j<mpi->MbWidth; j++)
            add_flit_data(mpi->MbMode[j+i*mpi->MbWidth]);
    add_flit_data(mpi->CbWidth);
    add_flit_data(mpi->CbHeight);
    add_flit_data(mpi->CbPitch);
    for (int i=0; i<mpi->CbHeight; i++)
        for (int j=0; j<mpi->CbWidth; j++)
        {                
            add_flit_data(mpi->TotalCoeffC[0][j+i*mpi->CbWidth]);
            add_flit_data(mpi->TotalCoeffC[1][j+i*mpi->CbWidth]);
        }    
    add_flit_data(mpi->TbWidth);
    add_flit_data(mpi->TbHeight);
    add_flit_data(mpi->TbPitch);
    for (int i=0; i<mpi->TbHeight; i++)
        for (int j=0; j<mpi->TbWidth; j++) 
        {                
            add_flit_data(mpi->TotalCoeffL[j+i*mpi->TbWidth]);
            add_flit_data(mpi->Intra4x4PredMode[j+i*mpi->TbWidth]);
            add_flit_data(mpi->MVx[j+i*mpi->TbWidth]);
            add_flit_data(mpi->MVy[j+i*mpi->TbWidth]);
        }            
}
/////////////////////////////////////////////////////////////////
void NOC_interface_send::add_mpi_MB_flit_data(mode_pred_info *mpi,int x,int y)
{
    add_flit_data(mpi->MbHeight);
    add_flit_data(mpi->MbWidth);
    add_flit_data(mpi->MbPitch);

    add_flit_data(mpi->MbMode[(x/16)+(y/16)*(mpi->MbWidth/16)]);

    add_flit_data(mpi->CbWidth);
    add_flit_data(mpi->CbHeight);
    add_flit_data(mpi->CbPitch);

    for (int i=(y/8); i<(y/8)+2; i++)        
       for (int j=(x/8); j<(x/8)+2; j++)
       {                
            add_flit_data(mpi->TotalCoeffC[0][j+i*mpi->CbWidth]);   
            add_flit_data(mpi->TotalCoeffC[1][j+i*mpi->CbWidth]);
       }    

    add_flit_data(mpi->TbWidth);
    add_flit_data(mpi->TbHeight);
    add_flit_data(mpi->TbPitch);

    for (int i=(y/4); i<(y/4)+4; i++)
        for (int j=(x/4); j<(x/4)+4; j++) 
       {                
            add_flit_data(mpi->TotalCoeffL[j+i*mpi->TbWidth]);
            add_flit_data(mpi->Intra4x4PredMode[j+i*mpi->TbWidth]);
            add_flit_data(mpi->MVx[j+i*mpi->TbWidth]);
            add_flit_data(mpi->MVy[j+i*mpi->TbWidth]);
        }            
}

/////////////////////////////////////////////////////////////////
void NOC_interface_send::add_MB_frame_flit_data(MB_frame MB_f)
{
    for (int i=0;i<256;i++)
        add_flit_data(MB_f.L[i]);
    for (int i=0;i<2;i++)    
        for (int j=0;j<64;j++)
            add_flit_data(MB_f.C[i][j]);
}
////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
MB_frame NOC_interface_receive::get_MB_frame_flit_data()
{
    MB_frame MB_f;    
    for (int i=0;i<256;i++)
       MB_f.L[i]=get_flit_data();
    for (int i=0;i<2;i++)    
        for (int j=0;j<64;j++)
           MB_f.C[i][j]=get_flit_data();
    return MB_f;    
}

////////////////////////////////////////////////////////////////
mode_pred_info *NOC_interface_receive::get_mpi_flit_data()
{
    mode_pred_info *mpi;
    int MbHeight,MbWidth,MbPitch;
    
    MbHeight=get_flit_data();
    MbWidth=get_flit_data();
    MbPitch=get_flit_data();    

    mpi=alloc_mode_pred_info(MbWidth<<4,MbHeight<<4);    

    mpi->MbHeight=MbHeight;
    mpi->MbWidth=MbWidth;
    mpi->MbPitch=MbPitch;    

    for (int i=0; i<mpi->MbHeight; i++)
        for (int j=0; j<mpi->MbWidth; j++)
            mpi->MbMode[j+i*mpi->MbWidth]=get_flit_data();
    mpi->CbWidth=get_flit_data();
    mpi->CbHeight=get_flit_data();
    mpi->CbPitch=get_flit_data();
    for (int i=0; i<mpi->CbHeight; i++)
        for (int j=0; j<mpi->CbWidth; j++)
        {                
            mpi->TotalCoeffC[0][j+i*mpi->CbWidth]=get_flit_data();
            mpi->TotalCoeffC[1][j+i*mpi->CbWidth]=get_flit_data();
        }    
    mpi->TbWidth=get_flit_data();
    mpi->TbHeight=get_flit_data();
    mpi->TbPitch=get_flit_data();
    for (int i=0; i<mpi->TbHeight; i++)
        for (int j=0; j<mpi->TbWidth; j++) 
        {                
            mpi->TotalCoeffL[j+i*mpi->TbWidth]=get_flit_data();
            mpi->Intra4x4PredMode[j+i*mpi->TbWidth]=(int)get_flit_data();
            mpi->MVx[j+i*mpi->TbWidth]=get_flit_data();
            mpi->MVy[j+i*mpi->TbWidth]=get_flit_data();
        }            
    return mpi;
    
}

/////////////////////////////////////////////////////////////////
mode_pred_info_MB NOC_interface_receive::get_mpi_MB_flit_data()
{
    mode_pred_info_MB mpi_MB;
    int MbHeight,MbWidth,MbPitch;
    
    MbHeight=get_flit_data();
    MbWidth=get_flit_data();
    MbPitch=get_flit_data();  

    mpi_MB.MbHeight=MbHeight;
    mpi_MB.MbWidth=MbWidth;
    mpi_MB.MbPitch=MbPitch;

    mpi_MB.MbMode=get_flit_data();

    mpi_MB.CbWidth=get_flit_data();
    mpi_MB.CbHeight=get_flit_data();
    mpi_MB.CbPitch=get_flit_data();
 
    for (int i=0; i<2; i++)
        for (int j=0; j<2; j++)
        {
            mpi_MB.TotalCoeffC[0][j+i*2]=get_flit_data();
            //printf("received mpi_MB.TotalCoeffC[0][%d] = %d\n",j+i*2,mpi_MB.TotalCoeffC[0][j+i*2]);            
            mpi_MB.TotalCoeffC[1][j+i*2]=get_flit_data(); 
            //printf("received mpi_MB.TotalCoeffC[1][%d] = %d\n",j+i*2,mpi_MB.TotalCoeffC[1][j+i*2]);            
          
        }

    mpi_MB.TbWidth=get_flit_data();
    mpi_MB.TbHeight=get_flit_data();
    mpi_MB.TbPitch=get_flit_data();
     
    for (int i=0; i<4; i++)
        for (int j=0; j<4; j++)
        {
            mpi_MB.TotalCoeffL[j+i*4]=get_flit_data(); 
            mpi_MB.Intra4x4PredMode[j+i*4]=get_flit_data(); 
            mpi_MB.MVx[j+i*4]=get_flit_data(); 
            mpi_MB.MVy[j+i*4]=get_flit_data();
        }
    
    return mpi_MB;
    
}



//////////////////////////////////////////////////
//////////////////////////////////////////////////
/////////////////////////////////////////////////    
void NOC_interface_send::send_to_display(StorablePicture pic,IP_CORE_NAME src,long packet_size,long start_packet_sign)
{
    int flit_count=0;
    DATA packet;
    vector<DATA> packet_array;

    _packet_num=0;
    _src=src;
    _dest=DISPLAY;
    _packet_size=packet_size;
    _zero_overloaded_num = 0;    
    _pic=pic;
    //_packet_array.clear();
    
    //sign for packet from event ...
    // printf ("send_to_display\nstart_packet_sign = %d\n",start_packet_sign);
    //cin.get();
    
    for (int i=0;i<packet_size;i++)
        packet.push_back(start_packet_sign);

        
    _packet_array.push_back(packet);
    _packet_num++;
    packet.clear(); 
   ////////////////////////////////////////////////////////////

    //main packet
    packet.push_back(_pic.size_x);
    flit_count++;
    packet.push_back(_pic.size_y);
    flit_count++;  
 
    // cout <<"pic.size"<<pic.A.size()<<endl;
    // imgY
    //cout <<"1: send_to_display"<<endl;
    //cout <<"_pic.size_y: "<<_pic.size_y<<"   _pic.size_x: "<<_pic.size_x<<endl;
    //cout <<"_pic.imgY[0][0]: "<<_pic.imgY[0][0]<<endl;
        
    for (long y=0;y<_pic.size_y;y++)
    {            
        for (long x=0;x<_pic.size_x;x++)
        {            
            //cout <<"("<<y<<","<<x<<")";        
            packet.push_back(_pic.imgY[y][x]);
            flit_count++;
            
            if (flit_count % packet_size==0 && flit_count!=0)
            {               
                _packet_array.push_back(packet);
                _packet_num++; 
                //cout <<"packet_num= "<<_packet_num<<"  packet.size()= "<< packet.size()<<endl;
                
                packet.clear();                
            }

        }
    }
    
    //cout <<"2: send_to_display"<<endl;
        
    // imgUV[0]
    for (long y=0;y<_pic.size_y/2;y++)
    {            
        for (long x=0;x<_pic.size_x/2;x++)
        {            
            
            //cout <<"i= "<<i<<endl;        
            packet.push_back(_pic.imgUV[0][y][x]);
            flit_count++;
            
            if (flit_count % packet_size==0 && flit_count!=0)
            {               
                _packet_array.push_back(packet);
                _packet_num++; 
                //cout <<"packet_num= "<<_packet_num<<"  packet.size()= "<< packet.size()<<endl;
                
                packet.clear();                
            }
        }
    }

    //cout <<"3: send_to_display"<<endl;

    // imgUV[1]
    for (long y=0;y<_pic.size_y/2;y++)
    {            
        for (long x=0;x<_pic.size_x/2;x++)
        {            
            
            //cout <<"i= "<<i<<endl;        
            packet.push_back(_pic.imgUV[1][y][x]);
            flit_count++;
            
            if (flit_count % packet_size==0 && flit_count!=0)
            {               
                _packet_array.push_back(packet);
                _packet_num++; 
                //cout <<"packet_num= "<<_packet_num<<"  packet.size()= "<< packet.size()<<endl;
                
                packet.clear();                
            }

             
        
        }
    }

    if (packet.size()!=_packet_size && packet.size()!= 0) 
    {            
        for (long i=packet.size(); i<_packet_size;i++)
        {                
            packet.push_back(0);            
            _zero_overloaded_num++;
        }
        _packet_array.push_back(packet);
        _packet_num++;        
        //cout << "send to display- packet_num = "<<_packet_num<<endl;
        
    }    

    packet.clear();      
    //end of package
    for (int i=0;i<packet_size;i++)
        packet.push_back(LAST_PACKET_FLIT_SIGN);

    _packet_array.push_back(packet);
   _packet_num++;
    packet.clear();

    //packet_array_display()
}
////////////////////////////////////////
void NOC_interface_send::send_to_display1(IP_CORE_NAME src,long packet_size)
{
    int flit_count=0;
    DATA packet;
    vector<DATA> packet_array;

    _packet_num=0;
    _src=src;
    _dest=DISPLAY;
    _packet_size=packet_size;
    _zero_overloaded_num = 0;    
    //_pic=pic;
    _packet_array.clear();
    packet.clear();
    
    //sign for packet from event ...
    for (int i=0;i<packet_size;i++)
        packet.push_back(GET_NALU_TO_DECODE_HDR_FIRST_RUN_PACKET_FLIT_SIGN);

    _packet_array.push_back(packet);
    _packet_num++;
    packet.clear(); 
   ////////////////////////////////////////////////////////////

    //main packet
    packet.push_back(_pic.size_x);
    flit_count++;
    packet.push_back(_pic.size_y);
    flit_count++;  
 
    // cout <<"pic.size"<<pic.A.size()<<endl;
    // imgY
    //cout <<"1: send_to_display"<<endl;
    //cout <<"_pic.size_y: "<<_pic.size_y<<"   _pic.size_x: "<<_pic.size_x<<endl;
    //cout <<"_pic.imgY[0][0]: "<<_pic.imgY[0][0]<<endl;
        
    for (long y=0;y<_pic.size_y;y++)
    {            
        for (long x=0;x<_pic.size_x;x++)
        {            
            
            //cout <<"("<<y<<","<<x<<")";        
            packet.push_back(_pic.imgY[y][x]+1000);
            flit_count++;
            
            if (flit_count % packet_size==0 && flit_count!=0)
            {               
                _packet_array.push_back(packet);
                _packet_num++; 
                //cout <<"packet_num= "<<_packet_num<<"  packet.size()= "<< packet.size()<<endl;
                
                packet.clear();                
            }

        }
    }
    
    //cout <<"2: send_to_display"<<endl;
        
    // imgUV[0]
    for (long y=0;y<_pic.size_y/2;y++)
    {            
        for (long x=0;x<_pic.size_x/2;x++)
        {            
            
            //cout <<"i= "<<i<<endl;        
            packet.push_back(_pic.imgUV[0][y][x]+1000);
            flit_count++;
            
            if (flit_count % packet_size==0 && flit_count!=0)
            {               
                _packet_array.push_back(packet);
                _packet_num++; 
                //cout <<"packet_num= "<<_packet_num<<"  packet.size()= "<< packet.size()<<endl;
                
                packet.clear();                
            }
        }
    }

    //cout <<"3: send_to_display"<<endl;

    // imgUV[1]
    for (long y=0;y<_pic.size_y/2;y++)
    {            
        for (long x=0;x<_pic.size_x/2;x++)
        {            
            
            //cout <<"i= "<<i<<endl;        
            packet.push_back(_pic.imgUV[1][y][x]+1000);
            flit_count++;
            
            if (flit_count % packet_size==0 && flit_count!=0)
            {               
                _packet_array.push_back(packet);
                _packet_num++; 
                //cout <<"packet_num= "<<_packet_num<<"  packet.size()= "<< packet.size()<<endl;
                
                packet.clear();                
            }

             
        
        }
    }

    if (packet.size()!=_packet_size && packet.size()!= 0) 
    {            
        for (long i=packet.size(); i<_packet_size;i++)
        {                
            packet.push_back(0);            
            _zero_overloaded_num++;
        }
        _packet_array.push_back(packet);
        _packet_num++;        
        //cout << "send to display1- packet_num = "<<_packet_num<<endl;
        
    }    

    packet.clear();      
    //end of package
    for (int i=0;i<packet_size;i++)
        packet.push_back(LAST_PACKET_FLIT_SIGN);

    _packet_array.push_back(packet);
   _packet_num++;
    packet.clear();

}
    

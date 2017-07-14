#ifndef NOC_interface
#define NOC_interface


#include "NOC_global.h"
//#include "vnoc_event.h"
#include "vnoc_topology.h"//enum IP_CORE_NAME {DECODER, DISPLAY};
#include <vector>
#include <iostream>
#include "modules.h"
//#include "NOC_interface.h"
#include "main.h"
#include <SDL/SDL.h>
/////////////////////////////////
/*
#include <getopt.h>
#include <SDL/SDL.h>
#include "h264.h"
#include "perf.h"
#include "slicehdr.h"
#include "mode_pred.h"
#include "mbmodes.h"
#include "cavlc.h"
#include "slice.h"
*/

#define USE_X86_ASM

#define BitsPerPixel 16
/////////////////////////////////



using namespace std;                             //


class VNOC;       
//////////////////////////////////////////////////////////////////////////////////////////////////////////
static inline int clamp_and_scale(int i);

void showframe(SDL_Surface *screen, frame *f, int zoom);


/////////////////////////////////////////////////////////////////////////////////////////////////////////
class NOC_interface_send 
{
    private:
		VNOC *_vnoc;
		int _ID;
        IP_CORE_NAME _src;
        IP_CORE_NAME _dest;
        long _packet_size;
        vector<DATA> _packet_array;
        long _zero_overloaded_num;
        long _packet_num;
        StorablePicture _pic;
        bool _packets_ready;//receive
        double _start_time;        
        DATA _packet;//temprorary use
        long _flit_count;

     public:    
		//NOC_interface_send(){}
        NOC_interface_send(int ID,int packet_size,VNOC *owner){
			_vnoc=owner;
			_ID=ID;	
			_packet_size=packet_size;	
		}

        //NOC_interface_send(long packet_num,long packet_size){// receive
                                                         //packet_num should be eliminated
        //    _packets_ready=false;
        //    _packet_num=packet_num;
        //    _packet_size=packet_size;    
        //    _packet_array.resize(packet_num);
        //    _start_time=-1;
            
        //}        

        //NOC_interface(const NOC_interface &n) : _packet_num(n.get_packet_num()), _packet_size(n.get_packet_size()) {}
        
        ~NOC_interface_send(){}        
        void send_to_display(StorablePicture pic,IP_CORE_NAME,long packet_size,long start_packet_sign);	
        void send_to_display1(IP_CORE_NAME,long packet_size);
	    void display_packet(long k);        
        DATA get_packet(long k){ return _packet_array[k];}
        void get_packet_from_DECODER(double time,long long unsigned int data);
        void pic_display();
        long get_packet_num(){return _packet_num; }
        long get_packet_size(){return _packet_size; }
        void packet_array_display();
        void build_pic();
		void packetize(string event_module_name);
		//template <class T>
        void add_flit_data(DATA_ELEMENT data);
		void add_sps_flit_data(seq_parameter_set sps);
		void add_pps_flit_data(pic_parameter_set pps);
		void add_nalu_flit_data(nal_unit nalu);
		void add_sh_flit_data(slice_header sh);
		void add_luma_chroma_DC_AC_flit_data(int LumaDCLevel[16], int LumaACLevel[16][16], int ChromaDCLevel[2][4], int ChromaACLevel[2][4][16]);
		void add_mpi_flit_data(mode_pred_info* mpi);
		void add_mpi_MB_flit_data(mode_pred_info* mpi,int x,int y);
		void add_MB_frame_flit_data(MB_frame MB_f);
};

///////////////////////////////////////////////////////
///////////////////////////////////////////////////////
///////////////////////////////////////////////////////

class NOC_interface_receive 
{ 
	
	public:
	   //enum EVENT_MODE { //used for set_event_mode(DATA packet,int packet_size) func, based on the packet sign	        
		//	GET_NALU_FIRST_RUN, GET_NALU, DECODE_HDR_FIRST_RUN, DECODE_HDR,
			// DECODE_MB_FIRST_RUN, DECODE_MB, INTRA_PRED, INTER_PRED_NORMAL, INTER_PRED_PSKIP};
		enum EVENT_MODE { TO_GET_NALU_FIRST_RUN, TO_GET_NALU, GET_NALU_TO_DECODE_HDR_FIRST_RUN, GET_NALU_TO_DECODE_HDR, DECODE_HDR_TO_DECODE_MB_FIRST_RUN, DECODE_HDR_TO_DECODE_MB, DECODE_MB_TO_INTRA_PRED, DECODE_MB_TO_INTER_PRED_NORMAL, DECODE_MB_TO_INTER_PRED_PSKIP, INTRA_PRED_MB_FRAME_TO_INTER_PRED, INTER_PRED_MB_FRAME_TO_INTRA_PRED, INTRA_PRED_MB_FRAME_TO_FRAME_BUFFER, INTER_PRED_MB_FRAME_TO_FRAME_BUFFER, FRAME_BUFFER_MB_FRAME_TO_DISPLAY,TO_CONTINUE_DECODE_MB, UPDATE_REF_FRAME_IN_INTER_PRED, UPDATE_FRAME_IN_FRAME_BUFFER};

    private:
		int _ID;
		VNOC *_vnoc;
        IP_CORE_NAME _src;
        IP_CORE_NAME _dest;
        long _packet_size;
		DATA _packet;
        vector<DATA> _packet_array;
        long _packet_num;
		long _depack_packet_num;
		long _depack_packet_array_num;	
        long _zero_overloaded_num;
        StorablePicture _pic;
        bool _packets_ready;//receive
        double _start_time;        
        EVENT_MODE _event_mode_received;
        //H264_EVENT_TYPE _event_mode_received_from;
		
     public:    
        NOC_interface_receive(){}
        NOC_interface_receive(VNOC *owner_vnoc){
				_vnoc=owner_vnoc;
		}
        NOC_interface_receive(long packet_num,long packet_size,VNOC *owner_vnoc,int ID){// receive
                                                         //packet_num should be eliminated
			_ID=ID;
            _vnoc=owner_vnoc;
			_packets_ready=false;
            _packet_num=0;
            _packet_size=packet_size;    
            _packet_array.resize(packet_num);
            _start_time=-1;
			_packet.clear();
            
        }        
        //NOC_interface(const NOC_interface &n) : _packet_num(n.get_packet_num()), _packet_size(n.get_packet_size()) {}
        
        ~NOC_interface_receive(){}        
        void send_to_display(StorablePicture pic,IP_CORE_NAME,long packet_size,long start_packet_sign);	
        void display_packet(long k);        
        DATA get_packet(long k){ return _packet_array[k];}
        void get_packet_from_NOC(double time,long long unsigned int data);
        void pic_display();
        long get_packet_num(){return _packet_num; }
        long get_packet_size(){return _packet_size; }
        void packet_array_display();
        void build_pic();
        bool is_it_start_packet(DATA packet,int packet_size);
        bool is_it_end_packet(DATA packet,int packet_size);
		void set_event_mode(DATA packet,int packet_size);
		void depacketize(string event_module_name);
		//template <class T> 
		DATA_ELEMENT get_flit_data();
		seq_parameter_set get_sps_flit_data();
		pic_parameter_set get_pps_flit_data();
		nal_unit get_nalu_flit_data();
		slice_header get_sh_flit_data();
		void get_luma_chroma_DC_AC_flit_data(int LumaDCLevel[16], int LumaACLevel[16][16], int ChromaDCLevel[2][4], int ChromaACLevel[2][4][16]);
		mode_pred_info* get_mpi_flit_data();
		mode_pred_info_MB get_mpi_MB_flit_data();
		MB_frame get_MB_frame_flit_data();
};












#endif

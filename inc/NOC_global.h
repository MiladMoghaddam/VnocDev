#ifndef NOC_global
#define NOC_global
#include "stdio.h"
#include "time.h"
//#include "mbuffer.h"
#include <vector>

//#include "global.h"
#define imgpel unsigned short
//typedef vector<unsigned long long> DATA;


///////////////////////////////////////////////////////////////////////////////
//////////////////setting placement////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

///*
//good placement //p1
#define GET_NALU_ID 2
#define DECODE_HDR_ID 1
#define DECODE_MB_ID 4
#define INTRA_PRED_ID 5
#define INTER_PRED_ID 3
#define FRAME_BUFFER_ID 7 
#define DISPLAY_ID 6
//*/

/*
//bad //p2
#define GET_NALU_ID 5
#define DECODE_HDR_ID 1
#define DECODE_MB_ID 7
#define INTRA_PRED_ID 0
#define INTER_PRED_ID 2
#define FRAME_BUFFER_ID 8
#define DISPLAY_ID 3
*/
/*
// //p3
//decodeMB						getNal
//intra			Display			
//decodeHDR		inter			frameBuff
			
#define GET_NALU_ID 8
#define DECODE_HDR_ID 0
#define DECODE_MB_ID 2
#define INTRA_PRED_ID 1
#define INTER_PRED_ID 3
#define FRAME_BUFFER_ID 6
#define DISPLAY_ID 4
*/
/*
// //p4
//				frameBuff		decodeHDR	
//inter			getNal			intra
//Display		decodeMB	
#define GET_NALU_ID 4
#define DECODE_HDR_ID 8
#define DECODE_MB_ID 3
#define INTRA_PRED_ID 7
#define INTER_PRED_ID 1
#define FRAME_BUFFER_ID 5
#define DISPLAY_ID 0
*/


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////



#define LAST_PACKET_FLIT_SIGN 90909
#define TO_GET_NALU_FIRST_RUN_PACKET_FLIT_SIGN 11111
#define TO_GET_NALU_PACKET_FLIT_SIGN 11100
#define GET_NALU_TO_DECODE_HDR_FIRST_RUN_PACKET_FLIT_SIGN 22222
#define GET_NALU_TO_DECODE_HDR_PACKET_FLIT_SIGN 22200 
#define DECODE_HDR_TO_DECODE_MB_FIRST_RUN_PACKET_FLIT_SIGN 33333
#define DECODE_HDR_TO_DECODE_MB_PACKET_FLIT_SIGN 33300 
#define DECODE_MB_TO_INTRA_PRED_PACKET_FLIT_SIGN 44444 
#define DECODE_MB_TO_INTER_PRED_NORMAL_PACKET_FLIT_SIGN 55555 
#define DECODE_MB_TO_INTER_PRED_PSKIP_PACKET_FLIT_SIGN 55550 
#define INTRA_PRED_MB_FRAME_TO_INTER_PRED_PACKET_FLIT_SIGN 44400
#define INTER_PRED_MB_FRAME_TO_INTRA_PRED_PACKET_FLIT_SIGN 55500
#define INTRA_PRED_MB_FRAME_TO_FRAME_BUFFER_PACKET_FLIT_SIGN 44000
#define INTER_PRED_MB_FRAME_TO_FRAME_BUFFER_PACKET_FLIT_SIGN 55000
#define FRAME_BUFFER_MB_FRAME_TO_DISPLAY_PACKET_FLIT_SIGN 66666
#define TO_CONTINUE_DECODE_MB_PACKET_FLIT_SIGN 77777 
#define UPDATE_REF_FRAME_IN_INTER_PRED_PACKET_FLIT_SIGN 88888
#define UPDATE_FRAME_IN_FRAME_BUFFER_PACKET_FLIT_SIGN 888000




//TODO
//pay attention to enum EVENT_TYPE in vnoc_event.h
//enum H264_EVENT_TYPE {
//		GET_NALU_FIRST_RUN, GET_NALU, DECODE_HDR_FIRST_RUN, DECODE_HDR,
//		DECODE_MB_FIRST_RUN, DECODE_MB, INTRA_PRED, INTER_PRED_NORMAL, INTER_PRED_PSKIP, DISPLAY};
//enum EVENT_TYPE { 
//        PE, ROUTER_BOOST, ROUTER, ROUTER_THROTTLE_1, ROUTER_THROTTLE_2, 
//        LINK, CREDIT, DUMMY, ROUTER_SINGLE, SYNC_PREDICT_DVFS_SET,
//		GET_NALU_FIRST_RUN, GET_NALU, DECODE_HDR_FIRST_RUN, DECODE_HDR, DECODE_MB_FIRST_RUN, DECODE_MB, INTRA_PRED, INTER_PRED_NORMAL, INTER_PRED_PSKIP, DISPLAY};

typedef long long unsigned int DATA_ELEMENT;
typedef struct Storable_Picture {

  int         size_x, size_y, size_x_cr, size_y_cr;
  imgpel **     imgY;         //!< Y picture component
  imgpel ***    imgUV;        //!< U and V picture components
}StorablePicture;


extern long pic_x;
extern long pic_y;
extern long mpix;
extern long mpack;
extern clock_t my_start_time, my_estimated_time;
extern clock_t StartTime,EndTime;

extern bool MANUAL_PIC;
extern FILE *InFile;
extern FILE *OutFile;

extern int IP_CORE_DELAY;
extern int packet_size_g;
extern char *res_file_name;
extern int frame_num_max_run;

















 #endif

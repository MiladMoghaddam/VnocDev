########################### VnocDec = VNoC + H.264Dec #############################################
VnocDec is a full system simulation framework for a network-on-chip (NoC)-based H.264 video that is 
built by combinining a network-on-chip simulator (VNoC[1]) and a basic H.264 video decoder[2].
Here we have the first version of the simulator that can be considered as a base for further
development. 
This framework let us investigate the impact of having different placement of H.264 modules over a
NoC on latency and power.
The simulator also gives us the opportunity to investigate the DVFS for network-on-chip based H. 264
video decoders with truly real workload
For further explanation about the framework please look at [3][4].
****************************************************************************
(Please cite references [3] and [4] in case you used the code in your work)
****************************************************************************

[1] Software downloads at MESSLab, Marquette University, 2016.[On-line].
    Available: http://dejazzer.com/software.html.

[2] Martin Fiedler, Implementation of a basic H.264/AVC Decoder, 2016.[Online].
    Available:http://keyj.emphy.de/files/projects/h264-src.tar.gz

[3] M.G. Moghaddam and C. Ababei, “Performance Evaluation of Network-on-Chip Based H.264 Video Decoders
    via Full System Simulation,” IEEE Embedded Systems Letters, vol. 9, no. 2 , pp. 49-52, Mar. 2017.

[4] M.G Moghaddam and C. Ababei, “Investigation of DVFS for Network-on-Chip Based H.264 Video Decoders
    with Truly Real Workload,” IEEE Int. Workshop on Green and Sustainable Computing (IGSC), 2016.

###################################################################################################
Developed by:
   Milad Ghorbani Moghaddam, Cristinel Ababei
   milad.ghorbanimoghaddam@marquette.edu, cristinel.ababei@marquette.edu

###################################################################################################
Installation guide:

1- Install boost library
sudo apt-get install libboost-all-dev

2- Install SDL1
sudo apt-get install libsdl1.2-dev

3- cd /VnocDev/obj/
   ./make.sh


###################################################################################################
How to run:

- cd /VnocDev/obj/

1- Without DVFS:
    ./exe traffic: IPCORE ary_size: 3 injection_rate: 0.1 cycles: 10000 do_dvfs: 0

2- with DVFS
    ./exe traffic: IPCORE ary_size: 3 injection_rate: 0.1 cycles: 10000 do_dvfs: 1 use_boost: 0 hist_window: 500

3- with DVFS and boost
    ./exe traffic: IPCORE ary_size: 3 injection_rate: 0.1 cycles: 10000 do_dvfs: 1 use_boost: 1 hist_window: 500
###################################################################################################

- "ary_size" (Array size in mesh, 3 means a 3x3 mesh), "injection_rate","cycles", "do_dvfs",
  "use_boost" and "hist_window" can be set at the run command.

- do_dvfs = 0 means Dynamic Voltage and Frequency Scaling (DVFS) controller is off
- do_dvfs = 1 means DVFS controller is on
- when do_dvfs = 1, if use_boost = 0, it means that DVFS controller is not allowed to boost the frequency
- when do_dvfs = 1, if use_boost = 1, it means that DVFS controller is allowed to boost the frequency 
  to use higher frequencies
- hist_window allows the DVFS controller to use the past hist_window size operations in link and buffers
  to predict the optimal frequency 

######################################################################################################
Changing the benchmark:

In /VnocDev/inc/, open the "module.h" and under class "obj_get_nalu_PE", change the followings based on 
the benchmark:
  #define WIDTH 352  //set the proper pixel width of the behcnmark frame
  #define HEIGHT 288 //set the proper pixel height of the behcnmark frame
  _filename="../streams/foreman_cif_baseline.264";  //set the .264 benchmark file (I put several in stream folder)
  res_file_name="../results/test.txt"; //set the name and address to save the generated statistics
  (I left some results in the "result" folder, possibly useful) 

######################################################################################################
Changing the placement:
To set the desired placement, in /VnocDev/inc/NOC_global.h, set the location in front of each module

For example for a 3x3 mesh we have:

#define GET_NALU_ID 2 //at PE #2
#define DECODE_HDR_ID 1 //at PE #1
#define DECODE_MB_ID 4 //at PE #4
#define INTRA_PRED_ID 5 //at PE #5
#define INTER_PRED_ID 3 //at PE #3
#define FRAME_BUFFER_ID 7 //at PE #7
#define DISPLAY_ID 6 //at PE #6

And the PE locations are as follows:
2   5   8
1   4   7
0   3   6

######################################################################################################
Changing the packet size:
To change the packet size open /VnocDev/src/NOC_global.cpp and set "the packet_size_g"
######################################################################################################
Setting end simulated frame number:
To set end simulated frame open /VnocDev/src/NOC_global.cpp and set your frame number in "frame_num_max_run"
######################################################################################################








#ifndef _VNOC_EVENT_H_
#define _VNOC_EVENT_H_

#include <set>

#include "vnoc_topology.h"
#include "vnoc.h"


using namespace std;

////////////////////////////////////////////////////////////////////////////////
//
// EVENT
//
////////////////////////////////////////////////////////////////////////////////

class EVENT {
    public:
    //enum EVENT_TYPE { 
    //    PE, ROUTER_BOOST, ROUTER, ROUTER_THROTTLE_1, ROUTER_THROTTLE_2, 
    //    LINK, CREDIT, DUMMY, ROUTER_SINGLE, SYNC_PREDICT_DVFS_SET,
	//	GET_NALU_FIRST_RUN, GET_NALU, DECODE_HDR_FIRST_RUN, DECODE_HDR, DECODE_MB_FIRST_RUN, DECODE_MB, INTRA_PRED, INTER_PRED_NORMAL, INTER_PRED_PSKIP, DISPLAY};

    enum EVENT_TYPE { 
        PE, ROUTER_BOOST, ROUTER, ROUTER_THROTTLE_1, ROUTER_THROTTLE_2, 
        LINK, CREDIT, DUMMY, ROUTER_SINGLE, SYNC_PREDICT_DVFS_SET,
		TO_GET_NALU_FIRST_RUN, TO_GET_NALU, GET_NALU_TO_DECODE_HDR_FIRST_RUN, GET_NALU_TO_DECODE_HDR, DECODE_HDR_TO_DECODE_MB_FIRST_RUN, DECODE_HDR_TO_DECODE_MB, DECODE_MB_TO_INTRA_PRED, DECODE_MB_TO_INTER_PRED_NORMAL, DECODE_MB_TO_INTER_PRED_PSKIP, INTRA_PRED_MB_FRAME_TO_INTER_PRED, INTER_PRED_MB_FRAME_TO_INTRA_PRED, INTRA_PRED_MB_FRAME_TO_FRAME_BUFFER, INTER_PRED_MB_FRAME_TO_FRAME_BUFFER,FRAME_BUFFER_MB_FRAME_TO_DISPLAY,TO_CONTINUE_DECODE_MB,UPDATE_REF_FRAME_IN_INTER_PRED,UPDATE_FRAME_IN_FRAME_BUFFER};

    private:
        EVENT_TYPE _type;
        // stores id of router where the event is generated;
        long _from_router_id;
        // id of router where the LIN or CREDIT events go;
        long _to_router_id;
        double _start_time;
        long _pc; // port id;
        long _vc; // virtual channel id for the above port id;
        FLIT _flit; // the data (payload);
        //milad add
        DATA _packet_data;
        IP_CORE_NAME _IP_core;
        //////////////////////
    public:
        // ROUTER type events;
        EVENT( EVENT_TYPE type, double time, long from_router_id) : 
            _type(type), _start_time(time), _pc(), _vc(), _flit() { 
            _from_router_id = from_router_id; // src address; 
            _to_router_id = 0; 
        }
        // PE type event;
        EVENT( EVENT_TYPE type, double time) : 
            _type(type), _start_time(time), _pc(), _vc(), _flit() { 
            _from_router_id = 0; 
            _to_router_id = 0;
            
        }
        //milad PE_IP_CORE type event for IP_CORE
        EVENT( EVENT_TYPE type, double time, IP_CORE_NAME IP_core, DATA &packet_data ) : 
            _type(type), _start_time(time), _pc(), _vc(), _flit(), _IP_core(IP_core) { 
            _from_router_id = 0; 
            _to_router_id = 0;           
            _IP_core = IP_core;
            for (int i=0;i<packet_data.size();i++)
                _packet_data.push_back(packet_data[i]);
        }
        ///////////////////////////////////////////////////////
        // LINK type event;
        EVENT( EVENT_TYPE type, double time, long to_router_id,
            long pc, long vc, FLIT &flit) : _type(type), 
            _start_time(time), _pc(pc), _vc(vc), _flit(flit) { 
            _from_router_id = 0; 
            _to_router_id = to_router_id; // dest address; 
        }
        // CREDIT type event;
        EVENT( EVENT_TYPE type, double time, long to_router_id,
            long pc, long vc) : _type(type), _start_time(time),
            _pc(pc), _vc(vc), _flit() { 
            _from_router_id = 0; 
            _to_router_id = to_router_id; // dest address; 
        }

        EVENT( EVENT &event) : _type(event.type()),
            _start_time(event.start_time()),
            _from_router_id(event.from_router_id()),
            _to_router_id(event.to_router_id()),
                _pc(event.pc()), _vc(event.vc()), _flit(event.flit()),_IP_core(event.get_IP_core())  { _packet_data=event.get_packet_data();}
        EVENT( const EVENT &event) : _type(event.type()),
            _start_time(event.start_time()),
            _from_router_id(event.from_router_id()),
            _to_router_id(event.to_router_id()),
                _pc(event.pc()), _vc(event.vc()), _flit(event.flit()),_IP_core(event.get_IP_core())  { _packet_data=event.get_packet_data();}
            
        ~EVENT() {}


        EVENT_TYPE type() const { return _type; }
        double start_time() const { return _start_time; }
        long pc() const { return _pc; }
        long vc() const { return _vc; }
        FLIT &flit() { return _flit; }
        const FLIT &flit() const { return _flit;}

        long from_router_id() const { return _from_router_id; }
        long to_router_id() const { return _to_router_id; }
        //milad add
        DATA &get_packet_data() { return _packet_data; }
        IP_CORE_NAME get_IP_core() {return _IP_core; }
        
            
            
};

////////////////////////////////////////////////////////////////////////////////
//
// EVENT_QUEUE
//
// the simulator is nothing but a queue of events that are continuously added
// to the queue and continuously processed from the queue;
//
////////////////////////////////////////////////////////////////////////////////

inline bool operator<(const EVENT &a, const EVENT &b) {
    return a.start_time() < b.start_time();
}

class EVENT_QUEUE {
    private:
        multiset<EVENT> _events;
        double _current_sim_time;
        long _event_count;
        long _queue_events_simulated;
    public:
        TOPOLOGY *_topology;
        VNOC *_vnoc;

    public:
        EVENT_QUEUE( double start_time, TOPOLOGY *topology);
        ~EVENT_QUEUE() {}

        typedef multiset<EVENT>::size_type size_type; 
        typedef multiset<EVENT>::iterator iterator;

        VNOC *vnoc() { return _vnoc; }
        void set_vnoc(VNOC *vnoc) { _vnoc = vnoc; };
        TOPOLOGY *topology() const { return _topology; }
        double current_sim_time() const { return _current_sim_time; }
        long event_count() const { return _event_count; }
        long queue_events_simulated() const { return _queue_events_simulated; }
        iterator get_event() { return _events.begin(); }
        void remove_event( iterator pos) { _events.erase(pos); }
        void remove_top_event() { 
            _events.erase(_events.begin());
            _event_count --; 
        }
        size_type event_queue_size() const { return _events.size(); }
        void add_event( const EVENT &event) { 
            _event_count ++; 
            _events.insert(event); 
        }

        void insert_initial_events();
        bool run_simulation();
};

#endif

#include "../includes/ac_simul.hpp"

#define SERVICE_INTERVAL        (500)

int fault_signals[TOTAL_AIRCRAFTS];
bool global_terminate;


void spawn_threads(vector<thread> *th_pool, int total_ac, aircraft **ac_array) {
    if(th_pool && ac_array) {
        for(auto th=0; th<total_ac; th++) {
            th_pool->emplace_back(aircraft_simul, th, ac_array[th]);
        }
    }   
} 

void aircraft_simul(int tid, aircraft *plane)  {
    thread_local milliseconds interval(SERVICE_INTERVAL), ref;
    if(plane) {
        // TO DO: add a barrier to synchronize the start 
        while(!global_terminate) {
            if(isduration(ref, interval)) {
                plane->state_machine();
                get_counter_val(&ref);
            }
        }
        cout << "Terminating service for aircraft: " << plane->get_ac_num() << endl;
    }
}

void set_fault_sig(int ac, int state) {
    if(ac<=TOTAL_AIRCRAFTS) {
        fault_signals[ac-1] = state;
    }
}

int get_fault_sig(int ac) {
    int ret=0;
    if(ac<=TOTAL_AIRCRAFTS) {
        ret = fault_signals[ac-1];
    }
    return ret;
}

void set_terminate_sig(bool state) {
    global_terminate = state;
}

int get_fault_sig() {
    return global_terminate;
}



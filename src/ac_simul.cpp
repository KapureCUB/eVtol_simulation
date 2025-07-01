#include "../includes/ac_simul.hpp"

#define SERVICE_INTERVAL        (500)
#define CHARGING_INTERVAL       (250)

int fault_signals[TOTAL_AIRCRAFTS];
// 0 = not charging/done charging, 1,2,3 = on charging with resp charger 
int charge_signals[TOTAL_AIRCRAFTS];
bool global_terminate;

static _c_live_info charger1_live;
static _c_live_info charger2_live;
static _c_live_info charger3_live;

void spawn_threads(vector<thread> *th_pool, int total_ac, aircraft **ac_array, queue<_c_queue_entry*> *cq) {
    if(th_pool && ac_array) {
        for(auto th=0; th<total_ac; th++) {
            th_pool->emplace_back(aircraft_simul, th, ac_array[th], cq);
        }
    }   
} 

void aircraft_simul(int tid, aircraft *plane, queue<_c_queue_entry*> *cq)  {
    thread_local milliseconds interval(SERVICE_INTERVAL), ref;
    if(plane) {
        // TO DO: add a barrier to synchronize the start 
        plane->set_status(IN_FLIGHT);
        while(!global_terminate) {
            if(isduration(ref, interval)) {
                plane->state_machine(interval, charge_signals[plane->get_ac_num()], 
                                     fault_signals[plane->get_ac_num()], cq);
                get_counter_val(&ref);
            }
        }
        cout << "Terminating service for aircraft: " << plane->get_ac_num() << endl;
    }
}

void charging_service(charger *ch, queue<_c_queue_entry*> *cq) {
    milliseconds interval(CHARGING_INTERVAL);
    static milliseconds ref;
    if(ch && cq && isduration(ref, interval)) {
        if(charger1_live.status == BUSY_CHARGING) {     // update live status 
            charger1_live.c_time_left -= interval.count();
            if(charger1_live.c_time_left <= 0) {        // check if done charging   
                charge_signals[charger1_live.ac_num] = 0;
                charger1_live.status = READY_TO_CHARGE;
                charger1_live.ac_num = -1;
                charger1_live.c_time_left = 0;
                ch->update_charger_stat(CHARGER_1, READY_TO_CHARGE);
            } else {
                ch->update_usetime(CHARGER_1, interval);    // update use time for charger 1
            }
        }
        if(charger2_live.status == BUSY_CHARGING) {     // update live status
            charger2_live.c_time_left -= interval.count();
            if(charger2_live.c_time_left <= 0) {        // check if done charging   
                charge_signals[charger2_live.ac_num] = 0;
                charger2_live.status = READY_TO_CHARGE;
                charger2_live.ac_num = -1;
                charger2_live.c_time_left = 0;
                ch->update_charger_stat(CHARGER_2, READY_TO_CHARGE);
            } else {
                ch->update_usetime(CHARGER_2, interval);    // update use time for charger 2
            }
        }
        if(charger3_live.status == BUSY_CHARGING) {     // update live status
            charger3_live.c_time_left -= interval.count();
            if(charger3_live.c_time_left <= 0) {        // check if done charging   
                charge_signals[charger3_live.ac_num] = 0;
                charger3_live.status = READY_TO_CHARGE;
                charger3_live.ac_num = -1;
                charger3_live.c_time_left = 0;
                ch->update_charger_stat(CHARGER_3, READY_TO_CHARGE);
            } else {
                ch->update_usetime(CHARGER_3, interval);    // update use time for charger 3
            }
        }

        if(!cq->empty()) {                              // check if any aircrafts in queue
            if(charger1_live.status == READY_TO_CHARGE) {     
                _c_queue_entry *entry = cq->front();
                charger1_live.ac_num = entry->ac_num;
                charger1_live.c_time_left = entry->charge_time;
                charger1_live.status = BUSY_CHARGING;
                cq->pop();
                charge_signals[charger1_live.ac_num] = CHARGER_1;
                ch->update_charger_stat(CHARGER_1, BUSY_CHARGING);
            } 
            if(charger2_live.status == READY_TO_CHARGE) {     
                _c_queue_entry *entry = cq->front();
                charger2_live.ac_num = entry->ac_num;
                charger2_live.c_time_left = entry->charge_time;
                charger2_live.status = BUSY_CHARGING;
                cq->pop();
                charge_signals[charger2_live.ac_num] = CHARGER_2;
                ch->update_charger_stat(CHARGER_2, BUSY_CHARGING);
            }
            if(charger3_live.status == READY_TO_CHARGE) {     
                _c_queue_entry *entry = cq->front();
                charger3_live.ac_num = entry->ac_num;
                charger3_live.c_time_left = entry->charge_time;
                charger3_live.status = BUSY_CHARGING;
                cq->pop();
                charge_signals[charger3_live.ac_num] = CHARGER_3;
                ch->update_charger_stat(CHARGER_3, BUSY_CHARGING);
            }
        } 
        get_counter_val(&ref);
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

void set_charge_sig(int ac, int state) {
    if(ac<=TOTAL_AIRCRAFTS) {
        charge_signals[ac-1] = state;
    }
}

int get_charge_sig(int ac) {
    int ret=0;
    if(ac<=TOTAL_AIRCRAFTS) {
        ret = charge_signals[ac-1];
    }
    return ret;
}



/**
 * @brief   Aircraft Simulation File  
 * @details This file contains the simulation function for the eVtol simulation problem from Joby Avation.
 *          Fucntions here are used to init the aircraft objects based on set aircrafts and also include 
 *          different services like simulation and charging.
 * @author  Deepak E Kapure
 * @date    07-02-2025 
 * 
 */

#include "../includes/ac_simul.hpp"

/**
 * @brief Macros to define the execution intervals for the simulation and charging service
 *        Units are in milliseconds.
 * 
 */
#define SERVICE_INTERVAL        (10)
#define CHARGING_INTERVAL       (5)                     // lower than simulation service to reduce downtime

/**
 * @brief Global flags for fault, termination and charge signalling
 * 
 */
// 0 - no fault, 1 - fault, 2 - fault during charging (to notify charging service)
int fault_signals[TOTAL_AIRCRAFTS] = {0};
// 0 = not charging/done charging, 1,2,3 = on charging with resp charger 
int charge_signals[TOTAL_AIRCRAFTS] = {0};
// false - running, true - terminate
bool global_terminate = false;

// Local file specific variables
static milliseconds charging_ref(0);

// Obecjts to keep track of live charging status of chargers
static _c_live_info charger1_live = {READY_TO_CHARGE, -1, 0};
static _c_live_info charger2_live = {READY_TO_CHARGE, -1, 0};
static _c_live_info charger3_live = {READY_TO_CHARGE, -1, 0};

/**
 * @brief Function to init threads for simulation
 * 
 * @param th_pool vector to store pointers to threads
 * @param total_ac total number of aircrafts
 * @param ac_array vector to store AC objects
 * @param cq charging queue pointer 
 * 
 * @return None
 * 
 */
void spawn_threads(vector<thread> *th_pool, int total_ac, aircraft **ac_array, queue<_c_queue_entry*> *cq) {
    if(th_pool && ac_array) {
        for(auto th=0; th<total_ac; th++) {
            th_pool->emplace_back(aircraft_simul, th, ac_array[th], cq);
        }
    }   
} 

/**
 * @brief Simulates aircraft behavior in a periodic loop based on SERVICE_INTERVAL
 *
 * @param tid Thread ID for the simulation.
 * @param plane Pointer to the aircraft object.
 * @param cq Pointer to the charging queue.
 *
 * @return None
 */
void aircraft_simul(int tid, aircraft *plane, queue<_c_queue_entry*> *cq)  {
    thread_local milliseconds interval(SERVICE_INTERVAL), ref;
    if(plane) {
        // TO DO: add a barrier to synchronize the start 
        plane->set_status(IN_FLIGHT);
        while(!global_terminate) {
            if(isduration(ref, interval)) {
                plane->state_machine(interval, charge_signals[plane->get_ac_num()], 
                                     &fault_signals[plane->get_ac_num()], cq);
                get_counter_val(&ref);
            }
        }
    }
}

/**
 * @brief Updates charger states and processes the charging queue.
 *        Handles ongoing charging sessions, checks for completion or faults,
 *        and assigns queued aircraft to available chargers.
 *
 * @param ch Pointer to the charger manager.
 * @param cq Pointer to the aircraft charging queue.
 *
 * @return None
 */
void charging_service(charger *ch, queue<_c_queue_entry*> *cq) {
    milliseconds interval(CHARGING_INTERVAL);
    if(ch && cq && isduration(charging_ref, interval)) {
        if(charger1_live.status == BUSY_CHARGING) {     // update live status 
            charger1_live.c_time_left -= interval.count();
            if((charger1_live.c_time_left <= 0) || (fault_signals[charger1_live.ac_num]==2)) {        // check if done charging   
                charge_signals[charger1_live.ac_num] = 0;
                //cout << "Charging done for: " << charger1_live.ac_num << endl;
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
            if((charger2_live.c_time_left <= 0) || (fault_signals[charger2_live.ac_num]==2)) {        // check if done charging   
                charge_signals[charger2_live.ac_num] = 0;
                //cout << "Charging done for: " << charger2_live.ac_num << endl;
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
            if((charger3_live.c_time_left <= 0) || (fault_signals[charger3_live.ac_num]==2)) {        // check if done charging   
                charge_signals[charger3_live.ac_num] = 0;
                //cout << "Charging done for: " << charger3_live.ac_num << endl;
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
                //cout << "Charging started for: " << charger1_live.ac_num << " on charger: 1"<< endl; 
            } else if(charger2_live.status == READY_TO_CHARGE) {     
                _c_queue_entry *entry = cq->front();
                charger2_live.ac_num = entry->ac_num;
                charger2_live.c_time_left = entry->charge_time;
                charger2_live.status = BUSY_CHARGING;
                cq->pop();
                charge_signals[charger2_live.ac_num] = CHARGER_2;
                ch->update_charger_stat(CHARGER_2, BUSY_CHARGING);
                //cout << "Charging started for: " << charger2_live.ac_num << " on charger: 2"<<  endl; 
            } else if(charger3_live.status == READY_TO_CHARGE) {     
                _c_queue_entry *entry = cq->front();
                charger3_live.ac_num = entry->ac_num;
                charger3_live.c_time_left = entry->charge_time;
                charger3_live.status = BUSY_CHARGING;
                cq->pop();
                charge_signals[charger3_live.ac_num] = CHARGER_3;
                ch->update_charger_stat(CHARGER_3, BUSY_CHARGING);
                //cout << "Charging started for: " << charger3_live.ac_num << " on charger: 3"<<  endl;
            }
        } 
        get_counter_val(&charging_ref);
    }
}

/**
 * @brief Sets the fault signal state for a given aircraft.
 *
 * @param ac Aircraft index.
 * @param state Fault state to set.
 *
 * @return None
 */
void set_fault_sig(int ac, int state) {
    if(ac<TOTAL_AIRCRAFTS) {
        fault_signals[ac] = state;
    }
}

/**
 * @brief Gets the fault signal state for a given aircraft.
 *
 * @param ac Aircraft index (1-based).
 *
 * @return Fault state of the aircraft.
 */
int get_fault_sig(int ac) {
    int ret=0;
    if(ac<=TOTAL_AIRCRAFTS) {
        ret = fault_signals[ac-1];
    }
    return ret;
}

/**
 * @brief Sets the global termination signal.
 *
 * @param state Termination flag value.
 *
 * @return None
 */
void set_terminate_sig(bool state) {
    global_terminate = state;
}

/**
 * @brief Sets the charge signal state for a given aircraft.
 *
 * @param ac Aircraft index (1-based).
 * @param state Charge state to set.
 *
 * @return None
 */
int get_fault_sig() {
    return global_terminate;
}

/**
 * @brief Gets the charge signal state for a given aircraft.
 *
 * @param ac Aircraft index (1-based).
 *
 * @return Charge state of the aircraft.
 */
void set_charge_sig(int ac, int state) {
    if(ac<=TOTAL_AIRCRAFTS) {
        charge_signals[ac-1] = state;
    }
}

/**
 * @brief Gets the charge signal state for a given aircraft.
 *
 * @param ac Aircraft index (1-based).
 *
 * @return Charge state of the aircraft.
 */
int get_charge_sig(int ac) {
    int ret=0;
    if(ac<=TOTAL_AIRCRAFTS) {
        ret = charge_signals[ac-1];
    }
    return ret;
}

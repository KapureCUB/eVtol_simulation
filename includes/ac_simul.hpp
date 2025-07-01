#ifndef _AIRCRAFT_SIMULATION_
#define _AIRCRAFT_SIMULATION_

#include "../includes/definitions.hpp"
#include <thread>
#include <barrier>

typedef struct CHARGER_LIVE_INFO {
    _charger_stat status;
    int ac_num;
    int c_time_left;
} _c_live_info;

void spawn_threads(vector<thread> *th_pool, int total_ac, aircraft **ac_array, queue<_c_queue_entry*> *cq);
void aircraft_simul(int tid, aircraft *plane, queue<_c_queue_entry*> *cq);
void charging_service(charger *ch, queue<_c_queue_entry> *cq);
void set_fault_sig(int ac, int state);
int get_fault_sig(int ac);
void set_terminate_sig(bool state);
int get_fault_sig();
void set_charge_sig(int ac, int state);
int get_charge_sig(int ac);

#endif //_AIRCRAFT_SIMULATION_
#ifndef _AIRCRAFT_SIMULATION_
#define _AIRCRAFT_SIMULATION_

#include "../includes/definitions.hpp"
#include <thread>
#include <barrier>

void spawn_threads(vector<thread> *th_pool, int total_ac, aircraft **ac_array);
void aircraft_simul(int tid, aircraft *plane);
void set_fault_sig(int ac, int state);
int get_fault_sig(int ac);
void set_terminate_sig(bool state);
int get_fault_sig();

#endif //_AIRCRAFT_SIMULATION_
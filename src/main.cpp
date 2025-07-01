#include "../includes/definitions.hpp"
#include "../includes/ac_simul.hpp"

static _ac_map paramter_map = {
    { ALPHA,   {120, 320000, 60, 1600, 4} },
    { BRAVO,   {100, 100000, 20, 1500, 5} },
    { CHARLIE, {160, 220000, 80, 2200, 3}},
    { DELTA,   {90, 120000, 62, 800, 2} },
    { ECHO,    {30, 150000, 30, 5800, 2} }
};

static _prob_map probablity_map = {
    { ALPHA,   0.25 },
    { BRAVO,   0.10 },
    { CHARLIE, 0.05 },
    { DELTA,   0.22 },
    { ECHO,    0.61 }
};

_fault_map fault_queue;
queue<_c_queue_entry*> charger_queue;

int main() {

    vector<thread> threadpool;
    /*
    milliseconds curr, interval(50), ref;
    for(auto i=0;i<10;) {
        while(isduration(ref, interval)) {
            cout << "Elapsed time: " << curr.count() << endl;
            i++;
            ref = curr;
        }
        update_Timer();
        get_counter_val(&curr);
    }
    */
    
    aircraft *aircraft_array[TOTAL_AIRCRAFTS];
    create_aircrafts(aircraft_array, TOTAL_AIRCRAFTS, &paramter_map, TOTAL_CATEGORIES);
    // for(auto i: aircraft_array) {
    //     cout << i->get_company() << " passengers: " << i->get_passengers() << endl;
    // }
    fault_injection(&probablity_map, aircraft_array, TOTAL_AIRCRAFTS, &fault_queue);
    // cout << "Faults at --" << endl;
    // for(auto i: fault_queue) {
    //     cout << "AC: " << i.second << ", time: " << (i.first).count() << endl;
    // }

    // Spawn threads
    spawn_threads(&threadpool, TOTAL_AIRCRAFTS, aircraft_array, &charger_queue);

    init_Timer();

    milliseconds dummy(5000), curr(0);
    while(curr < dummy) {
        update_Timer();
        get_counter_val(&curr);
    }
    // terminate threads
    set_terminate_sig(true);
    for(auto &th: threadpool) {
        th.join();
    }

    delete_aircrafts(aircraft_array, TOTAL_AIRCRAFTS);
    
    return 0;
}
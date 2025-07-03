#include "../includes/definitions.hpp"
#include "../includes/ac_simul.hpp"

string log_file = "evtol_sim_log.txt";

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

int main() {

    ofstream fp;
    _fault_map fault_queue;
    charger global_charger;
    queue<_c_queue_entry*> charger_queue;
    
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
    cout << "--------Starting eVtol simulation--------" << endl;
    cout << "Spawning " << TOTAL_AIRCRAFTS << " aircrafts" << endl;
    create_aircrafts(aircraft_array, TOTAL_AIRCRAFTS, &paramter_map, TOTAL_CATEGORIES);
    // for(auto i: aircraft_array) {
    //     cout << i->get_company() << " passengers: " << i->get_passengers() << endl;
    // }
    fault_injection(&probablity_map, aircraft_array, TOTAL_AIRCRAFTS, &fault_queue);
    cout << "Faults at --" << endl;
    for(auto i: fault_queue) {
        cout << "AC: " << i.second << ", time: " << (i.first).count() << endl;
    }

    // Spawn threads
    spawn_threads(&threadpool, TOTAL_AIRCRAFTS, aircraft_array, &charger_queue);

    fp = open_log_file(log_file);
    init_Timer();

    int total_time = SIMULATION_TIME_HRS * SIMULATION_FACTOR;
    cout << "Simulating for " << SIMULATION_TIME_HRS << " hours." << " Time: " << SIMULATION_TIME_HRS << " minutes (" << total_time << ")"<< endl;
    cout << "All fights airborne!" << endl;
    milliseconds total_sim_time(total_time), curr(0);
    while(curr < total_sim_time) {
        fault_service(&fault_queue);
        charging_service(&global_charger, &charger_queue);
        data_recorder_service(aircraft_array, TOTAL_AIRCRAFTS, fp);
        update_Timer();
        get_counter_val(&curr);
    }
    // terminate threads
    cout << "Terminating all fight sims.." << endl;
    set_terminate_sig(true);
    for(auto &th: threadpool) {
        th.join();
    }

    for(auto a: aircraft_array) {
        cout << "Aircraft: " << a->get_ac_num() << " -- flight time: " << a->get_flight_time() << \
        " hours, miles: " << a->get_miles() << ", faults: " << a->get_fault_count() << endl;  
    }

    cout << "\nFlight data recorded in file: " << log_file << endl;
    close_file(fp);
    delete_aircrafts(aircraft_array, TOTAL_AIRCRAFTS);

    cout << "-----------End of simulation----------" << endl;
    
    return 0;
}
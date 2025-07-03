/**
 * @brief   eVtol Simulation  
 * @details This file contains the main function and top level functions for the eVtol simulation problem from Joby Avation.
 *          Simulation run-time is set at 3 hours for 20 aircrafts as default. These parameters can be chaged by setting the macros in definition.hpp
 *          The simulation time resolution is 1 milliseconds and 1 minute simulation time = 1 hours real world time.
 * @author  Deepak E Kapure
 * @date    07-02-2025 
 * 
 */
#include "../includes/definitions.hpp"
#include "../includes/ac_simul.hpp"

/**
 * @brief Aircraft parameters and log file literals
 * 
 */
const string log_file = "evtol_sim_log.txt";
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

    // Shared global variables  
    ofstream fp;                                                // log file pointer
    _fault_map fault_queue;                                     // fault service queue
    charger global_charger;                                     // charger object TO DO: For stroing charger info
    queue<_c_queue_entry*> charger_queue;                       // charging queue
    vector<thread> threadpool;                                  // threads for spawning 
    aircraft *aircraft_array[TOTAL_AIRCRAFTS];                  // global aircraft object array

    cout << "--------Starting eVtol simulation--------" << endl;
    cout << "Spawning " << TOTAL_AIRCRAFTS << " aircrafts" << endl;

    // Create aircraft objects 
    create_aircrafts(aircraft_array, TOTAL_AIRCRAFTS, &paramter_map, TOTAL_CATEGORIES);
    // Pre-calculate faults
    fault_injection(&probablity_map, aircraft_array, TOTAL_AIRCRAFTS, &fault_queue);
    
    cout << "Faults at --" << endl;
    for(auto i: fault_queue) {
        cout << "Aircraft number: " << i.second << ", time: " << (i.first).count() << endl;
    }

    // Spawn threads
    spawn_threads(&threadpool, TOTAL_AIRCRAFTS, aircraft_array, &charger_queue);

    // open log file for dumping flight data
    fp = open_log_file(log_file);
    
    // Initialize global timer
    init_Timer();

    // Prepare best-effort loop for simulation
    int total_time = SIMULATION_TIME_HRS * SIMULATION_FACTOR;
    
    cout << "Simulating for " << SIMULATION_TIME_HRS << " hours." << " Time: " << SIMULATION_TIME_HRS << " minutes (" << total_time << ")"<< endl;
    cout << "All fights airborne!" << endl;
    
    milliseconds total_sim_time(total_time), curr(0);
    while(curr < total_sim_time) {
        // Call fault handling service to inject faults  
        fault_service(&fault_queue);
        // Service to handle charging for aircrafts
        charging_service(&global_charger, &charger_queue);
        // Flight Data Recorder service to log aircraft info
        data_recorder_service(aircraft_array, TOTAL_AIRCRAFTS, fp);
        // Update simulation counter
        update_Timer();
        get_counter_val(&curr);
    }

    // Terminate threads
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
    
    // Executing exit sequence
    close_file(fp);
    delete_aircrafts(aircraft_array, TOTAL_AIRCRAFTS);

    cout << "-----------End of simulation----------" << endl;
    
    return 0;
}
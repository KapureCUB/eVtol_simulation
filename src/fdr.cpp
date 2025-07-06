/**
 * @brief   Flight Data Recorder file  
 * @details This file contains the flight data recorder functions for the eVtol simulation problem from Joby Avation.
 *          File contains file IO functions, fault service and fault injection algorithm. 
 * 
 * @author  Deepak E Kapure
 * @date    07-02-2025 
 * 
 */

#include "../includes/definitions.hpp"
#include "../includes/ac_simul.hpp"
#include <random>
#include <cmath>
#include <sstream>
#include <iomanip>

/**
 * @brief Macro for fault service execution interval in milliseconds
 * 
 */
#define FDR_INTERVAL                (2000)
#define FAULT_SERVICE_INTERVAL      (20)

static string input_log = "evtol_sim_input.txt";
ofstream fp_in;

static map<int, string> comp_map = {
    { 0, "ALPHA" },
    { 1, "BETA" },
    { 2, "CHARLIE" },
    { 3, "DELTA" },
    { 4, "ECHO" }
};

static map<int, int> p_map = {
    { 0, 4 },
    { 1, 5 },
    { 2, 3 },
    { 3, 2 },
    { 4, 2 }
};

milliseconds fdr_curr(0);
milliseconds fault_curr(0);

/**
 *  @brief Real-time calculation factors. The eVTOL_Simul_analysis.xlsx file contains 
 *         the analysis behind these factors.
 *         Format:
 *         { company, { energy used per simulation time, battery cap per % soc, miles travelled per simul time x 100 } }
 */  
map<_ac_type, vector<double>> calc_factors = {
    { ALPHA,   {3.2, 3200, 0.002} },
    { BRAVO,   {2.5, 1000, 0.001666} },
    { CHARLIE, {5.8666, 2200, 0.002666} },
    { DELTA,   {1.2, 1200, 0.0015} },
    { ECHO,    {2.9, 1500, 0.0005} }
}; 

/**
 * @brief Initializes and populates the aircraft array with categorized aircraft.
 *        Randomly distributes aircraft across types and creates instances accordingly.
 *
 * @param ac_array Array to store aircraft pointers.
 * @param size Total number of aircraft.
 * @param map Pointer to aircraft configuration map.
 * @param categories Number of aircraft categories.
 *
 * @return None
 */
void create_aircrafts(aircraft **ac_array, int size, _ac_map *map, int categories) {
    vector<int> cat_count(categories, 1);            // init array to atleasst 1 for each type
    ostringstream line;

    int remain = (size - categories);
    srand(time(0));                                  // init RNG
    while(remain--) {
        cat_count[(rand()%categories)]++;            // assign randomly count for each type 
    }
    cout << "Alpha: " << cat_count[0] << " ";
    cout << "Bravo: " << cat_count[1] << " ";
    cout << "Charlie: " << cat_count[2] << " ";
    cout << "Delta: " << cat_count[3] << " ";
    cout << "Echo: " << cat_count[4] << endl;

    line << "Alpha: " << cat_count[0] << " ";
    line << "Bravo: " << cat_count[1] << " ";
    line << "Charlie: " << cat_count[2] << " ";
    line << "Delta: " << cat_count[3] << " ";
    line << "Echo: " << cat_count[4];

    fp_in = open_log_file(input_log);
    write_to_file(fp_in, line.str());

    size--;
    for(int type=(TOTAL_CATEGORIES-1); type>=0; type--) {       // fill aircraft array
        while(cat_count[type]) {
            ac_array[size] = new aircraft(size, (_ac_type)type, map, &calc_factors);
            size--;
            cat_count[type]--;
        }
    }
}

/**
 * @brief Deletes dynamically allocated aircraft objects in the array.
 *
 * @param ac_array Array of pointers to aircraft objects.
 * @param size Number of aircraft in the array.
 *
 * @return None
 */
void delete_aircrafts(aircraft **ac_array, int size) {
    if((ac_array) && (size)) {
        size--;
        while(size) {
            delete ac_array[size--];
        }
    }
}   
/**
 * 
 */
/**
 * @brief Injects faults into aircraft based on exponential failure probability.
 *        Generates fault events over simulation time and inserts them into the fault map.
 *        Ref: https://cplusplus.com/reference/random/exponential_distribution/
 *             https://www.geeksforgeeks.org/probability-distributions-exponential-distribution/
 *             https://www.scribbr.com/statistics/poisson-distribution/
 *
 * @param pmap Pointer to the failure probability map by aircraft company.
 * @param ac_array Array of aircraft pointers.
 * @param size Number of aircraft.
 * @param q Pointer to the fault event map (timestamp to aircraft number).
 *
 * @return None
 */
void fault_injection(_prob_map *pmap, aircraft **ac_array, int size, _fault_map *q) {
    double lambda_min, next_failure;
    int total_minutes = SIMULATION_TIME_HRS * HRS_TO_MINUTES; 
    ostringstream line;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::exponential_distribution<> exp_dist;

    int current_time = 0;
    _ac_info *plane;
    for(int i=0; i<size; i++) {
        plane = (ac_array[i])->get_ac_info();
        lambda_min = (pmap->at(plane->company))/60.0;
        exp_dist.param(std::exponential_distribution<>::param_type(lambda_min));
        while (current_time < total_minutes) {
            next_failure = exp_dist(gen);
            current_time += next_failure;
            if (current_time < total_minutes) {
                q->insert({ milliseconds(current_time*1000), (plane->ac_num)});
            }
        }
        current_time = 0;
    }
    line << "Faults:" << endl;
    for(auto& i: *q) {
        line << "Aircraft_number: " << i.second << " Time: " << (i.first).count() << endl;
    }
    line << "Total_time: " << SIMULATION_TIME_HRS << " hours " 
         << "Simulation_Time: " << SIMULATION_TIME_HRS << " minutes";
    write_to_file(fp_in, line.str());
    
    close_file(fp_in);
}

/**
 * @brief Checks and injects faults based on scheduled fault events.
 *        Sets fault signals for aircraft when the fault time is reached and removes events from the queue.
 *
 * @param q Pointer to the fault event map (timestamp to aircraft number).
 *
 * @return None
 */
void fault_service(_fault_map *q) {
    milliseconds interval(FAULT_SERVICE_INTERVAL);
    if(isduration(fault_curr, interval) && q && !(q->empty())) {                        // enter only if map list is not empty
        auto entry = q->begin();
        get_counter_val(&fault_curr);
        if((fault_curr) >= (entry->first)) {              // check if its time for fault 
            //cout << "Injecting fault for " << entry->second << endl; 
            set_fault_sig(entry->second, 1);        // set the fault signal for Aircraft
            q->erase(entry);                        // remove from map
        }
    }
}

/**
 * @brief Opens a file for writing, creating or overwriting it.
 *
 * @param filename Name of the file to open.
 *
 * @return ofstream object associated with the file.
 */
ofstream open_log_file(const string &filename) {
    ofstream outfile(filename, ios::out);  
    return outfile;  
}

/**
 * @brief Closes the given file stream if it is open.
 *
 * @param outfile Reference to the ofstream to close.
 *
 * @return None
 */
void close_file(ofstream &outfile) {
    if (outfile.is_open()) {
        outfile.close();
    }
}

/**
 * @brief Writes a line to the opened file stream.
 *
 * @param outfile Reference to the open ofstream.
 * @param line The string line to write.
 *
 * @return True if write was successful; false otherwise.
 */
bool write_to_file(ofstream &outfile, const string &line) {
    bool ret = true;
    if (outfile.is_open()) {
        ret = true;
        outfile << line << '\n';
    }
    
    return ret;
}

/**
 * @brief Records aircraft data at fixed intervals and writes to the output file.
 *        Collects flight and charging parameters from all aircraft and logs them as a single line.
 *        Data line:
 *        {"timestamp" "ac_num" "company" "status" "flight_time" "miles_travelled" "battery_soc" "c_id" "charge_time" "fault_count" "charge_sessions" ...}
 * 
 * @param ac_array Array of pointers to aircraft objects.
 * @param size Number of aircraft in the array.
 * @param outfile Output file stream to write the data.
 *
 * @return None
 */
void data_recorder_service(aircraft **ac_array, int size, ofstream &outfile) {
    milliseconds interval(FDR_INTERVAL);
    if((ac_array) && (size>=0) && isduration(fdr_curr, interval)) {
        get_counter_val(&fdr_curr);

        ostringstream line;
        line << fdr_curr.count() << " ";
        for (int i = 0; i < TOTAL_AIRCRAFTS; ++i) {
            aircraft *ac = ac_array[i];

            line << ac->get_ac_num() << " ";
            line << comp_map.at(ac->get_company()) << " ";
            line << ac->get_ac_status() << " ";
            line << fixed << setprecision(4) << ac->get_flight_time() << " ";
            line << fixed << setprecision(4) << ac->get_miles() << " ";
            line << fixed << setprecision(4) << ac->get_battery_soc() << " ";
            line << ac->get_charger_id() << " ";
            line << fixed << setprecision(4) << ac->get_charge_time() << " ";
            line << ac->get_fault_count() << " ";
            line << ac->get_charger_sessions() << " ";
        }
        // dump to file
        write_to_file(outfile, line.str());
    }
}

void sim_analysis(aircraft **ac_array, int size, int categories, ofstream &outfile) {
    vector<int> cat_count(categories, 0);
    vector<double>  f_time(categories, 0.0);
    vector<double>  miles(categories, 0.0);
    vector<double>  c_time(categories, 0.0);
    vector<int>  c_sessions(categories, 0.0);
    vector<int>  f_count(categories, 0);

    for(int i=0; i<size; i++) {
        cat_count.at(ac_array[i]->get_company())++;
        f_time[ac_array[i]->get_company()] += ac_array[i]->get_flight_time();
        miles[ac_array[i]->get_company()] += ac_array[i]->get_miles();
        c_time[ac_array[i]->get_company()] += ac_array[i]->get_charge_time();
        c_sessions[ac_array[i]->get_company()] += ac_array[i]->get_charger_sessions();
        f_count[ac_array[i]->get_company()] += ac_array[i]->get_fault_count();
    }

    ostringstream line;  line << "\n\n";
    line << "Simulation_Results:\n";
    for(int i=0; i<categories; i++) {
        line << comp_map.at(i) << "\n";
        line << "Number_of_Flights: " << cat_count.at(i) << "\n";
        line << "Avg_flight_time(hrs): " << (f_time.at(i)/cat_count.at(i)) << "\n";
        line << "Avg_distance_per_flight(mile): " << (miles.at(i)/cat_count.at(i)) << "\n";
        if(c_sessions.at(i) == 0) c_sessions[i] = 1; 
        line << "Average_charge_time(hrs): " << (c_time.at(i)/c_sessions.at(i)) << "\n";
        line << "Total_faults: " << f_count.at(i) << "\n";
        line << "Total_passenger_miles(miles): " << (miles.at(i)*cat_count.at(i)*p_map.at(i)) << "\n\n";
    }    

    write_to_file(outfile, line.str());
}
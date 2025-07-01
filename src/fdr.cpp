#include "../includes/definitions.hpp"
#include "../includes/ac_simul.hpp"

#include <random>
#include <cmath>

/**
 *  Real-time calculation factors 
 *  
 */ 
// { company, { energy used per simulation time, battery cap per % soc, miles travelled per simul time x 100 } }
map<_ac_type, vector<int>> calc_factors = {
    { ALPHA,   {192, 3200, 12} },
    { BRAVO,   {150, 1000, 10} },
    { CHARLIE, {352, 2200, 16} },
    { DELTA,   {72, 1200, 9} },
    { ECHO,    {174, 1500, 3} }
}; 

void create_aircrafts(aircraft **ac_array, int size, _ac_map *map, int categories) {
    vector<int> cat_count(categories, 1);            // init array to atleasst 1 for each type
    int remain = (size - categories);
    srand(time(0));                                  // init RNG
    while(remain--) {
        cat_count[(rand()%categories)]++;            // assign randomly count for each type 
    }
    size--;
    for(int type=0; type<categories; type++) {       // fill aircraft array
        while(cat_count[type]) {
            ac_array[size] = new aircraft(size, (_ac_type)type, map, &calc_factors);
            size--;
            cat_count[type]--;
        }
    }
}

void delete_aircrafts(aircraft **ac_array, int size) {
    if((ac_array) && (size)) {
        size--;
        while(size) {
            delete ac_array[size--];
        }
    }
}   
/**
 * Ref: https://cplusplus.com/reference/random/exponential_distribution/
 *      https://www.geeksforgeeks.org/probability-distributions-exponential-distribution/
 *      https://www.scribbr.com/statistics/poisson-distribution/
 */
void fault_injection(_prob_map *pmap, aircraft **ac_array, int size, _fault_map *q) {
    double lambda_min, next_failure;
    int total_minutes = SIMULATION_TIME_HRS * HRS_TO_MINUTES; 

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
}

void fault_service(_fault_map *q) {
    milliseconds curr;
    if(q && !(q->empty())) {                        // enter only if map list is not empty
        auto entry = q->begin();
        get_counter_val(&curr);
        if((curr) >= (entry->first)) {              // check if its time for fault 
            set_fault_sig(entry->second, 1);        // set the fault signal for Aircraft
            q->erase(entry);                        // remove from map
        }
    }
}

int data_recorder_service() {
    return 0;
}
#ifndef _DEFINITIONS_
#define _DEFINITIONS_

#include <iostream>
#include <queue>
#include <map>
#include <thread>
#include "../includes/timer.hpp"

#define TOTAL_AIRCRAFTS             (5)
#define SIMULATION_TIME_HRS         (3)


#define HRS_TO_MINUTES              (60)

using namespace std;

typedef enum COMPANY {
    ALPHA=0,
    BRAVO=1,
    CHARLIE=2,
    DELTA=3,
    ECHO=4,
    TOTAL_CATEGORIES
} _ac_type;

typedef enum STATUS {
    STANDBY=-1,
    IN_FLIGHT=0,
    IN_CHARGE_QUEUE=1,
    CHARGING=2,
    UNDER_MAINTENANCE=3,
    SUSPENDED=4
} _ac_stat;

typedef enum CHARGER {
    NO_CHARGER=0,
    CHARGER_1=1,
    CHARGER_2=2,
    CHARGER_3=3
} _charger_id;


typedef struct AC_INFO {
    int ac_num;
    _ac_type company;
    int speed;              // miles per hour
    int batt_cap;           // kWh
    int toc_min;            // time to charge in minutes
    int energy_use;         // Energy used at Cruise in Wh/mile
    int passengers;         // number of passengers
} _ac_info;

typedef map<_ac_type, vector<int>> _ac_map;
typedef map<_ac_type, double> _prob_map;
typedef map<milliseconds, int> _fault_map;

class aircraft {
    private:
        pid_t tid;
        _ac_info ac;
        int flight_time;
        _ac_stat status;
        int fault_count;
        int charge_time;
        _charger_id c_id;
    public:
        aircraft(int num, _ac_type com, _ac_map *map) {
            if((com<=4) && (com>=0) && map) {
                vector<int> para = map->at(com);
                ac.ac_num = num;
                ac.company = com;          
                // fill parameters
                ac.speed = para[0];
                ac.batt_cap = para[1];
                ac.toc_min = para[2];
                ac.energy_use = para[3];
                ac.passengers = para[4];
                // init status
                flight_time = 0;
                status = STANDBY;
                fault_count = 0;
                charge_time = 0;
                c_id = NO_CHARGER;
            }
        }
        int get_ac_num() {
            return (this->ac).ac_num;
        }

        _ac_type get_company() {
            return (this->ac).company;
        }

        int get_passengers() {
            return (this->ac).passengers;
        }

        _ac_info *get_ac_info() {
            return (&ac);
        }

        ~aircraft() {}
        void state_machine() {
            cout << "Servicing aircraft: " << ac.ac_num << endl;
        }
};


void create_aircrafts(aircraft **ac_array, int size, _ac_map *map, int categories);
void delete_aircrafts(aircraft **ac_array, int size);
void fault_injection(_prob_map *pmap, aircraft **ac_array, int size, _fault_map *q);

#endif //_DEFINITIONS_
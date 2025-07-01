#ifndef _DEFINITIONS_
#define _DEFINITIONS_

#include <iostream>
#include <queue>
#include <map>
#include <thread>
#include "../includes/timer.hpp"

#define TOTAL_AIRCRAFTS             (5)
#define SIMULATION_TIME_HRS         (3)
#define SIMULATION_FACTOR           (1000.0)            // 1 HOUR = 1 MINUTE SIMULATION = 1000 MILLISEC

#define DOWNTIME_HOURS              (0.5)
#define DOWNTIME_SIMUL_TIME         (DOWNTIME_HOURS * SIMULATION_FACTOR)     //msec to wait in simulation
#define HRS_TO_MINUTES              (60)
#define REAL_TO_REEL_TIME_FACTOR    (0.001)
#define BATTERY_SOC_THREASHOLD      (10)

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

typedef enum {
    OUT_OF_SERVICE=0,
    READY_TO_CHARGE=1,
    BUSY_CHARGING=2
} _charger_stat;

typedef enum CHARGER {
    NO_CHARGER=0,
    CHARGER_1=1,
    CHARGER_2=2,
    CHARGER_3=3
} _charger_id;

typedef struct CHARGER_INFO {
    _charger_stat status;
    int use_time;
    vector<int> history;
    // TO DO: More parameters can be added
} _charger_info;

typedef struct CHARGE_QUEUE_ENTRY {
    int ac_num;
    int charge_time;
} _c_queue_entry;

typedef struct AC_INFO {
    int ac_num;
    _ac_type company;
    int speed;              // miles per hour
    int batt_cap;           // Wh
    int toc_hrs;         // time to charge in HOURS*100
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
        int flight_time;                    // in hours x 100
        int miles_travelled;                // in mile x 100        
        _ac_stat status;
        _ac_stat prev_status;
        int fault_count;
        int battery_soc;                    // 100 to 0
        int bat_cap_used;
        _charger_id c_id;
        int downtime;
        map<_ac_type, vector<int>> *calc_factors;
    public:
        aircraft(int num, _ac_type com, _ac_map *m, map<_ac_type, vector<int>> *c) {
            if((com<=4) && (com>=0) && m) {
                vector<int> para = m->at(com);
                ac.ac_num = num;
                ac.company = com;          
                // fill parameters
                ac.speed = para[0];
                ac.batt_cap = para[1];
                ac.toc_hrs = para[2];
                ac.energy_use = para[3];
                ac.passengers = para[4];
                // init status
                flight_time = 0;
                miles_travelled = 0;
                status = STANDBY;
                fault_count = 0;
                battery_soc = 100;
                bat_cap_used = 0;
                c_id = NO_CHARGER;
                calc_factors = c;
            }
        }
        
        ~aircraft() {}

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

        void set_status(_ac_stat s) {
            status = s;
        }

        void update_ac_stats(milliseconds t) {
            if(status==IN_FLIGHT) {
                flight_time += (t.count() * REAL_TO_REEL_TIME_FACTOR * 100);
                miles_travelled += t.count() * (calc_factors->at(ac.company)[2]);
                bat_cap_used += (t.count() * (calc_factors->at(ac.company)[0]));
                battery_soc = (100 - (bat_cap_used % (calc_factors->at(ac.company)[1])));
            }
        }

        void state_machine(milliseconds t, int charge_sig, int fault_sig, queue<_c_queue_entry*> *cq) {
            switch(status) {
                case IN_FLIGHT:
                    if(fault_sig) {
                        fault_sig = 0;
                        status = UNDER_MAINTENANCE;
                    } else {
                        update_ac_stats(t);
                        // check battery
                        if(battery_soc <= BATTERY_SOC_THREASHOLD) {
                            _c_queue_entry *n = new _c_queue_entry;
                            n->ac_num = ac.ac_num;
                            n->charge_time = ac.toc_hrs*SIMULATION_FACTOR/100;
                            cq->push(n);
                            status = IN_CHARGE_QUEUE;
                        }
                    }
                    break;
                case IN_CHARGE_QUEUE:
                    if(fault_sig) {
                        fault_sig = 0;
                        status = UNDER_MAINTENANCE;
                    } else {
                        if(charge_sig > 0) {
                            c_id = (_charger_id)(charge_sig);
                            status = CHARGING;
                        }
                    }
                    break;
                case CHARGING:
                    if(fault_sig) {
                        c_id = NO_CHARGER;
                        fault_sig = 0;
                        status = UNDER_MAINTENANCE;
                    } else {
                        if(charge_sig == 0) {
                            bat_cap_used = 0;
                            battery_soc = 100;
                            c_id = NO_CHARGER;
                            status = IN_FLIGHT;
                        }
                    }
                    break;
                case UNDER_MAINTENANCE:
                    if(fault_sig) {                     // restart servicing again
                        downtime = 0;
                        fault_sig = 0;  
                    }
                    downtime += t.count();
                    if(downtime >= DOWNTIME_SIMUL_TIME) {
                        downtime = 0;
                        if(prev_status == CHARGING || prev_status == IN_CHARGE_QUEUE) {
                            _c_queue_entry *n = new _c_queue_entry;
                            n->ac_num = ac.ac_num;
                            n->charge_time = ac.toc_hrs*SIMULATION_FACTOR/100;
                            cq->push(n);
                            status = IN_CHARGE_QUEUE;
                        } else {
                            status = prev_status;
                        }
                    }
                    break;
                case STANDBY:
                case SUSPENDED:
                default:
                    break;

            };
            //cout << "Servicing aircraft: " << ac.ac_num << endl;
        }
};

class charger {
    private:
        _charger_info charger1;
        _charger_info charger2;
        _charger_info charger3;
    public:
        charger() {
            charger1.status = READY_TO_CHARGE;
            charger2.status = READY_TO_CHARGE;
            charger3.status = READY_TO_CHARGE;
        }
        ~charger() = default;

        _charger_stat check_charger(_charger_id id) {
            _charger_stat stat=BUSY_CHARGING;
            if(id==CHARGER_1) {
                stat = charger1.status;
            } else if(id==CHARGER_2) {
                stat = charger2.status;
            } else if(id==CHARGER_3) {
                stat = charger3.status;
            }
            return stat;
        }   

        void assign_charger(_charger_id id, int ac_num) {
            if(id==CHARGER_1) {
                charger1.history.push_back(ac_num);
            } else if(id==CHARGER_2) {
                charger2.history.push_back(ac_num);
            } else if(id==CHARGER_3) {
                charger3.history.push_back(ac_num);
            }
        }

        void update_usetime(_charger_id id, milliseconds time) {
           if(id==CHARGER_1) {
                charger1.use_time += time.count();
            } else if(id==CHARGER_2) {
                charger2.use_time += time.count();
            } else if(id==CHARGER_3) {
                charger3.use_time += time.count();
            } 
        }

        void update_charger_stat(_charger_id id, _charger_stat stat) {
           if(id==CHARGER_1) {
                charger1.status = stat;
            } else if(id==CHARGER_2) {
                charger2.status = stat;
            } else if(id==CHARGER_3) {
                charger3.status = stat;
            } 
        }

};

void create_aircrafts(aircraft **ac_array, int size, _ac_map *map, int categories);
void delete_aircrafts(aircraft **ac_array, int size);
void fault_injection(_prob_map *pmap, aircraft **ac_array, int size, _fault_map *q);

#endif //_DEFINITIONS_
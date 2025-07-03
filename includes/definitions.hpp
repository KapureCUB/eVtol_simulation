#ifndef _DEFINITIONS_
#define _DEFINITIONS_

#include <iostream>
#include <queue>
#include <map>
#include <thread>
#include <fstream>
#include "../includes/timer.hpp"

/**
 * @brief Simulation macros. Set these for changing simulation conditions
 * 
 */
#define AIRCRAFTS                   (20)  // -- Set this for aircrafts                 
#define SIMULATION_TIME_HRS         (3)   // -- Set this for hours 

// Derived and system macros
#define TOTAL_AIRCRAFTS             ((AIRCRAFTS < 5) ? (5) : (AIRCRAFTS))    // MINIMUM 5 AIRCRAFTS     
#define SIMULATION_FACTOR           (60000.0)                                // 1 HOUR = 1 MINUTE SIMULATION = 60000 MILLISEC
#define DOWNTIME_HOURS              (0.5)
#define DOWNTIME_SIMUL_TIME         (DOWNTIME_HOURS * SIMULATION_FACTOR)     //msec to wait in simulation
#define HRS_TO_MINUTES              (60)
#define REAL_TO_REEL_TIME_FACTOR    (0.00001666)         
#define BATTERY_SOC_THREASHOLD      (10)

using namespace std;

// Aircraft companies
typedef enum COMPANY {
    ALPHA=0,
    BRAVO=1,
    CHARLIE=2,
    DELTA=3,
    ECHO=4,
    TOTAL_CATEGORIES
} _ac_type;

// Aircraft status
typedef enum STATUS {
    STANDBY=-1,
    IN_FLIGHT=0,
    IN_CHARGE_QUEUE=1,
    CHARGING=2,
    UNDER_MAINTENANCE=3,
    SUSPENDED=4
} _ac_stat;

// Charger status
typedef enum {
    OUT_OF_SERVICE=0,
    READY_TO_CHARGE=1,
    BUSY_CHARGING=2
} _charger_stat;

// Chargers
typedef enum CHARGER {
    NO_CHARGER=0,
    CHARGER_1=1,
    CHARGER_2=2,
    CHARGER_3=3
} _charger_id;

/**
 * @brief Stores information and usage history of a charger.
 *
 * @var status Current status of the charger.
 * @var use_time Total usage time in milliseconds.
 * @var history Vector recording past usage durations or events.
 */
typedef struct CHARGER_INFO {
    _charger_stat status;
    int use_time;
    vector<int> history;
    // TO DO: More parameters can be added
} _charger_info;

/**
 * @brief Represents an entry in the charging queue.
 *
 * @var ac_num Aircraft number associated with this entry.
 * @var charge_time Charging duration required (in milliseconds or hours).
 */
typedef struct CHARGE_QUEUE_ENTRY {
    int ac_num;
    int charge_time;
} _c_queue_entry;

/**
 * @brief Contains static information about an aircraft.
 *
 * @var ac_num Aircraft identification number.
 * @var company Aircraft manufacturer type.
 * @var speed Cruise speed in miles per hour.
 * @var batt_cap Battery capacity in watt-hours (Wh).
 * @var toc_hrs Time to charge in hundredths of hours (hours * 100).
 * @var energy_use Energy consumption at cruise in Wh per mile.
 * @var passengers Number of passengers the aircraft can carry.
 */
typedef struct AC_INFO {
    int ac_num;
    _ac_type company;
    int speed;              // miles per hour
    int batt_cap;           // Wh
    int toc_hrs;         // time to charge in HOURS*100
    int energy_use;         // Energy used at Cruise in Wh/mile
    int passengers;         // number of passengers
} _ac_info;

// Definitions for maps
typedef map<_ac_type, vector<int>> _ac_map;
typedef map<_ac_type, double> _prob_map;
typedef map<milliseconds, int> _fault_map;

/**
 * @class aircraft
 * @brief Represents an aircraft with status, flight, battery, and charging management.
 *        Stores static aircraft info and dynamic state such as flight time, battery state,
 *        charging status, and fault tracking. Supports state machine logic for flight,
 *        charging, and maintenance states.
 */
class aircraft {
    private:
        pid_t tid;
        _ac_info ac;
        double flight_time;                    // in hours
        double miles_travelled;                // in mile        
        _ac_stat status;
        _ac_stat prev_status;
        int fault_count;
        double battery_soc;                    // 100 to 0
        double bat_cap_used;
        _charger_id c_id;
        double charge_time;                    // in hours
        int charge_time_offset;                // offset to subtract from charge time
        int downtime;
        map<_ac_type, vector<double>> *calc_factors;
    public:
        // Constructors
        aircraft(int num, _ac_type com, _ac_map *m, map<_ac_type, vector<double>> *c) {
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
                charge_time = 0;
                charge_time_offset = 0;
                downtime = 0;
                c_id = NO_CHARGER;
                calc_factors = c;
            }
        }
        // Destructors
        ~aircraft() {}

        // Setter functions
        void set_status(_ac_stat s) {
            status = s;
        }
        void update_ac_stats(milliseconds t) {
            if(status==IN_FLIGHT) {
                flight_time += (t.count() * REAL_TO_REEL_TIME_FACTOR);
                miles_travelled += t.count() * (calc_factors->at(ac.company)[2]);
                bat_cap_used += (t.count() * (calc_factors->at(ac.company)[0]));
                battery_soc = (100 - ((int)bat_cap_used / (int)(calc_factors->at(ac.company)[1])));
            }
        }

        // Getter functions
        int get_ac_num() {
            return (this->ac).ac_num;
        }
        int get_ac_status() {
            return status;
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
        double get_flight_time() { return flight_time; }
        double get_charge_time() { return charge_time; }
        double get_miles() { return miles_travelled; }
        double get_fault_count() { return fault_count; }
        double get_battery_soc() { return battery_soc; }
        int get_charger_id() { return c_id; }

        // State machine for aircraft simulation
        void state_machine(milliseconds t, int charge_sig, int *fault_sig, queue<_c_queue_entry*> *cq) {
            switch(status) {
                case IN_FLIGHT:
                    if(*fault_sig==1) {
                        fault_count++;
                        *fault_sig = 0;
                        cout << "Flight: " << ac.ac_num << " sent to maintenance" << endl;
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
                            cout << "Flight: " << ac.ac_num << " sent to charging" << endl;
                        }
                    }
                    break;
                case IN_CHARGE_QUEUE:
                    if(*fault_sig==1) {
                        fault_count++;
                        *fault_sig = 0;
                        cout << "Flight: " << ac.ac_num << " sent to maintenance" << endl;
                        status = UNDER_MAINTENANCE;
                    } else {
                        if(charge_sig > 0) {
                            c_id = (_charger_id)(charge_sig);
                            status = CHARGING;
                        }
                    }
                    break;
                case CHARGING:
                    if(*fault_sig==1) {
                        fault_count++;
                        c_id = NO_CHARGER;
                        *fault_sig = 2;                                 // setting to 2 to notify charging service
                        cout << "Flight: " << ac.ac_num << " sent to maintenance" << endl;
                        status = UNDER_MAINTENANCE;
                    } else {
                        charge_time += (t.count() * REAL_TO_REEL_TIME_FACTOR);
                        charge_time_offset += t.count();            // keep a record for charge time 
                        if(charge_sig == 0) {
                            charge_time_offset = 0;          // reset the offset to 0
                            bat_cap_used = 0;
                            battery_soc = 100;
                            c_id = NO_CHARGER;
                            status = IN_FLIGHT;
                            cout << "Flight: " << ac.ac_num << " air borne after charging" << endl;
                        }
                    }
                    break;
                case UNDER_MAINTENANCE:
                    if(*fault_sig==1) {                     // restart servicing again
                        fault_count++;
                        downtime = 0;
                        *fault_sig = 0;  
                    }
                    downtime += t.count();
                    if(downtime >= DOWNTIME_SIMUL_TIME) {
                        downtime = 0;
                        if(prev_status == CHARGING || prev_status == IN_CHARGE_QUEUE) {
                            _c_queue_entry *n = new _c_queue_entry;
                            n->ac_num = ac.ac_num;
                            n->charge_time = (ac.toc_hrs*SIMULATION_FACTOR/100) - charge_time_offset;
                            cq->push(n);
                            charge_time_offset = 0;
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

/**
 * @class charger
 * @brief Manages the state, usage, and history of three chargers.
 *        Tracks the status, usage time, and assigned aircraft for each charger.
 */
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
void fault_service(_fault_map *q);
void data_recorder_service(aircraft **ac_array, int size, ofstream &outfile);
ofstream open_log_file(const string &filename);
void close_file(ofstream &outfile);
bool write_to_file(ofstream &outfile, const string &line);

#endif //_DEFINITIONS_
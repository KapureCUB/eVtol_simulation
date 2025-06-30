#include "../includes/timer.hpp"

static time_point<high_resolution_clock> refernce_pt;
static milliseconds counter_val;

void init_Timer(void) {
    refernce_pt = high_resolution_clock::now();
}

void update_Timer(void) {
    time_point<high_resolution_clock> curr = high_resolution_clock::now();
    counter_val = duration_cast<milliseconds>(curr - refernce_pt);
}

int convert_to_hours(milliseconds diff, int factor) {
    return ((static_cast<int>(diff.count())) * factor);
}

int isduration(milliseconds ref, milliseconds msec) {
    int ret=0;
    if((counter_val - ref) >= msec) {
        ret = 1;
    }
    return ret;
}

void get_counter_val(milliseconds *m) {
    if(m) {
        *m = counter_val;
    }
}



/**
 * @brief   Timer file  
 * @details This file contains the timer functions for the eVtol simulation problem from Joby Avation.
 * 
 * @author  Deepak E Kapure
 * @date    07-02-2025 
 * 
 */

#include "../includes/timer.hpp"

// Time keeping variables 
static time_point<high_resolution_clock> refernce_pt;
static milliseconds counter_val;

/**
 * @brief Initializes the timer reference point to the current time.
 *
 * @return None
 */
void init_Timer(void) {
    refernce_pt = high_resolution_clock::now();
}

/**
 * @brief Updates the global counter with elapsed milliseconds since reference.
 *
 * @return None
 */
void update_Timer(void) {
    time_point<high_resolution_clock> curr = high_resolution_clock::now();
    counter_val = duration_cast<milliseconds>(curr - refernce_pt);
}

/**
 * @brief Converts a duration in milliseconds to hours multiplied by a factor.
 *
 * @param diff Duration in milliseconds.
 * @param factor Multiplication factor.
 *
 * @return Converted time as an integer.
 */
int convert_to_hours(milliseconds diff, int factor) {
    return ((static_cast<int>(diff.count())) * factor);
}

/**
 * @brief Checks if the elapsed duration since a reference time exceeds a threshold.
 *
 * @param ref Reference time in milliseconds.
 * @param msec Duration threshold in milliseconds.
 *
 * @return 1 if elapsed duration >= threshold, else 0.
 */
int isduration(milliseconds ref, milliseconds msec) {
    int ret=0;
    if((counter_val - ref) >= msec) {
        ret = 1;
    }
    return ret;
}

/**
 * @brief Retrieves the current global counter value.
 *
 * @param m Pointer to milliseconds variable to store the counter value.
 *
 * @return None
 */
void get_counter_val(milliseconds *m) {
    if(m) {
        *m = counter_val;
    }
}



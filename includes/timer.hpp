#ifndef _TIMER_
#define _TIMER_

#include <chrono>

using namespace std::chrono;

void init_Timer(void);
void update_Timer(void);
int convert_to_hours(milliseconds diff, int factor);
int isduration(milliseconds ref, milliseconds msec);
void get_counter_val(milliseconds *m);

#endif //_TIMER_
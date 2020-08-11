%module covid
%{
#include "covid_wrapper.h"
%}

struct Model_30 {
    int _day[365];
    int size;
};
extern void Model_to_file(float r0_num, int max_dur, float mult);
extern Model_30 Model_30_set(float r0_num, int max_dur, float mult);
extern int Model_30_get_size(const Model_30& this_model);
extern int Model_30_get_cases(const Model_30& this_model, int day);

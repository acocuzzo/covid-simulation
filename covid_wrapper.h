#ifndef COVIDWRAPPER_H_
#define COVIDWRAPPER_H_

extern "C" {
struct Model_30 {
    int _day[365];
    int size;
};

void Model_to_file(float r0_num, int max_dur, float mult);
Model_30 Model_30_set(float r0_num, int max_dur, float mult);
int Model_30_get_size(const Model_30& this_model);
int Model_30_get_cases(const Model_30& this_model, int day);
}

#endif
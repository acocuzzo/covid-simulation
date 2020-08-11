#include "covid_wrapper.h"

#include "covid.h"
#include "threadpool.h"
#include <vector>
#include <string>

void Model_to_file(float r0_num, int max_dur, float mult){
  std::int32_t cont_dur = 7;
  util::ThreadPool pool(10);
  std::int32_t pop_size = 8399000;
  std::int32_t days = 30;
  covid::Population this_pop(pop_size, r0_num, max_dur, cont_dur, mult, &pool);
  std::string r0_num_str = std::to_string(static_cast<std::int32_t>(r0_num)) + "." + std::to_string(static_cast<std::int32_t>((r0_num - static_cast<std::int32_t>(r0_num)) * 100));
  std::string mult_str = std::to_string(static_cast<std::int32_t>(mult)) + "." + std::to_string(static_cast<std::int32_t>((mult - static_cast<std::int32_t>(mult)) * 100));
  this_pop.project_and_write(days, r0_num_str, mult_str);
}

Model_30 Model_30_set(float r0_num, int max_dur, float mult){
  Model_30 this_model;
  if (max_dur <= 0 || r0_num < 0){
      for (auto& d : this_model._day){
          d = -1;
      }
      this_model.size = -1;
      return this_model;
  }
  std::int32_t cont_dur = 7;
  util::ThreadPool pool(10);
  std::int32_t pop_size = 8399000;
  //std::int32_t days = 30;
  covid::Population this_pop(pop_size, r0_num, max_dur, cont_dur, mult, &pool);
  std::vector<int> this_proj = this_pop.project_30_day_vector();
  this_model.size = this_proj.size();
  for (std::int32_t i = 0; i < this_proj.size(); ++i){
      this_model._day[i] = this_proj[i];
  }
 for (std::int32_t i = this_proj.size(); i < 365; ++ i){
      this_model._day[i] = 0;
  }
  return this_model;
}
int Model_30_get_size(const Model_30& this_model){
    return this_model.size;
}
int Model_30_get_cases(const Model_30& this_model, int day){
    if (day < 0 || day >= 365){
        return -1;
    }
    return this_model._day[day];
}
#include "covid.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <chrono>
#include <experimental/random>
#include <fstream>
#include <functional>
#include <iterator>
#include <list>
#include <memory>
#include <random>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <vector>

#include "mutex.h"
#include "threadpool.h"

namespace covid {

namespace
{
constexpr float kTotalPopulation = 8399000.0f;
}  // namespace

Population::Population(std::int32_t size, float r0_num, std::int32_t max_dur,
                       std::int32_t cont_dur, float mult,
                       util::ThreadPool* pool)
    : _pop_size(size),
      _max_dur(max_dur),
      _cont_dur(cont_dur),
      _CHD(build_CHD()),
      _rbs(build_rbs()),
      _age_odds(generate_age_odds()),
      _age_group(build_age_groups()),
      _r0_num(r0_num),
      _ref(0),
      _pool(pool) {
  modify_CHD(mult);
  _population.reserve(size);
  std::int32_t healthy_pop_size;
  std::vector<std::int32_t> inf_per_dur;

  if (_pop_size == kTotalPopulation) {
    _d = d_from_CHD();
    _r = r_from_CHD();
    _i = i_from_CHD();
    inf_per_dur = dur_from_CHD();
  } else {
    _d = int((d_perc() * _pop_size) + .5);
    _r = int((r_perc() * _pop_size) + .5);
    _i = int((i_perc() * _pop_size) + .5);
    for (const auto& inf : dur_perc()) {
      inf_per_dur.push_back(int((inf * _pop_size) + .5));
    }
  }
  for (std::int32_t dur = 0; dur < inf_per_dur.size(); ++dur) {
    add_n_people(inf_per_dur[dur], 1);
  }
  healthy_pop_size = _pop_size - (_d + _r + _i);
  _h.reserve(healthy_pop_size);
  add_n_people(healthy_pop_size, 0);
  std::random_shuffle(_h.begin(), _h.end());
}

void Population::add_n_people(std::int32_t n, std::int32_t status) {
  std::int32_t age_idx = 0;
  _population.reserve(_population.size() + n);
  std::vector<Person*> these_infected;
  these_infected.reserve(these_infected.size() + n);
  for (std::int32_t i = 0; i < n; ++i) {
    float age_r = static_cast<float>(i % (n / 2)) / static_cast<float>(n / 2);
    while (_age_odds[age_idx] < age_r * 100) {
      ++age_idx;
    }
    std::int32_t age = age_idx * 5 + std::experimental::randint(0, 5);
    bool isfemale = i >= (n / 2);
    _population.push_back(std::make_unique<Person>(
        status, age, isfemale, death_chance(age, isfemale)));
    if (status == 1) {
      these_infected.push_back(_population.back().get());
    } else if (status == 0) {
      _h.push_back(_population.back().get());
    }
  }
  if (status == 1) {
    _i_deque.push_front(these_infected);
  }
}

std::vector<float> Population::generate_age_odds() {
  std::vector<float> bounds;
  std::vector<float> chances{5.8,  5.5, 5.9, 6.2, 6.6, 14.7, 12.5,
                             13.1, 6.8, 6.5, 9.3, 5,   2.2};
  float total = 0;
  for (const auto& c : chances) {
    bounds.push_back(total + c);
    total += c;
  }
  assert(abs(bounds.rbegin()[0] - 100) < 1);
  assert(abs(bounds.rbegin()[1]) < 100);
  bounds.rbegin()[0] = 100;
  return bounds;
}

std::vector<std::vector<std::string>> Population::read_csv(
    const std::string& pathname) {
  std::ifstream file(pathname);
  CSVRow row;
  std::vector<std::vector<std::string>> csv_str;
  while (file >> row) {
    csv_str.push_back(std::move(row.m_data));
  }
  return csv_str;
}

std::vector<std::vector<std::int32_t>> Population::build_CHD() {
  std::vector<std::vector<std::string>> csv_str =
      read_csv("/home/anna/code/exercises/probability/CHD.csv");
  std::vector<std::vector<std::int32_t>> CHD;
  for (std::int32_t v = 1; v < csv_str.size() - 4; ++v) {
    std::vector<std::int32_t> CHD_line;
    for (std::int32_t s = 1; s < 4; ++s) {
      std::int32_t val;
      std::stringstream(csv_str[v][s]) >> val;
      CHD_line.push_back(std::move(val));
    }
    CHD.push_back(std::move(CHD_line));
  }
  return CHD;
}

void Population::modify_CHD(float mult) {
  if (mult == 0) {
    return;
  }
  if (mult == 1) {
    for (auto& v : _CHD) {
      v[0] *= 10;
    }
    return;
  }
  if (mult == 2) {
    for (auto& v : _CHD) {
      v[0] = v[2] * 100;
    }
    return;
  }
  if (mult == 3) {
    for (auto& v : _CHD) {
      v[0] = v[2] * 50;
    }
    return;
  }
}
std::vector<std::vector<float>> Population::build_rbs() {
  std::vector<std::vector<std::string>> csv_str =
      read_csv("/home/anna/code/exercises/probability/rates_by_sex.csv");
  std::vector<std::vector<float>> rbs;
  for (std::int32_t v = 1; v < csv_str.size() - 1; ++v) {
    std::vector<float> rbs_line;
    for (std::int32_t s = 1; s < 4; ++s) {
      float val;
      std::stringstream(csv_str[v][s]) >> val;
      rbs_line.push_back(val);
    }
    rbs.push_back(rbs_line);
  }
  return rbs;
}

std::int32_t Population::d_from_CHD() const {
  std::int32_t deaths = 0;
  for (const auto& v : _CHD) {
    deaths += v[2];
  }
  return deaths;
}

float Population::d_perc() const {
  float d_perc = d_from_CHD() / kTotalPopulation;
  return d_perc;
}
std::int32_t Population::r_from_CHD() const {
  std::int32_t recoveries = 0;
  std::int32_t current_day = _CHD.size() - 1;
  std::int32_t last_infection = current_day - (_max_dur - 1);
  for (std::int32_t day = 0; day < last_infection; ++day) {
    recoveries += _CHD[day][0];
  }
  return recoveries;
}

float Population::r_perc() const {
  return r_from_CHD() / kTotalPopulation;
}

std::int32_t Population::i_from_CHD() const {
  std::int32_t current_infections = 0;
  std::int32_t current_day = _CHD.size() - 1;
  std::int32_t last_infection = current_day - (_max_dur - 1);
  for (std::int32_t day = last_infection; day < _CHD.size(); ++day) {
    current_infections += _CHD[day][0];
  }
  return current_infections;
}

float Population::i_perc() const {
  return i_from_CHD() / kTotalPopulation;
}

std::vector<std::int32_t> Population::dur_from_CHD() const {
  std::vector<std::int32_t> inf_by_dur;
  std::int32_t current_day = _CHD.size() - 1;
  for (std::int32_t dur = 0; dur < _max_dur; ++dur) {
    inf_by_dur.push_back(_CHD[current_day - dur][0]);
  }
  return inf_by_dur;
}

std::vector<float> Population::dur_perc() const {
  std::vector<float> inf_by_dur;
  std::int32_t current_day = _CHD.size() - 1;
  for (std::int32_t dur = 0; dur < _max_dur; ++dur) {
    float inf_perc = _CHD[current_day - dur][0] / kTotalPopulation;
    inf_by_dur.push_back(inf_perc);
  }
  return inf_by_dur;
}
std::unordered_map<std::int32_t, std::int32_t> Population::build_age_groups() {
  std::unordered_map<std::int32_t, std::int32_t> age_group;
  for (std::int32_t i = 0; i <= 17; ++i) {
    age_group[i] = 0;
  }
  for (std::int32_t i = 18; i <= 44; ++i) {
    age_group[i] = 1;
  }
  for (std::int32_t i = 45; i <= 64; ++i) {
    age_group[i] = 2;
  }
  for (std::int32_t i = 65; i <= 74; ++i) {
    age_group[i] = 3;
  }
  for (std::int32_t i = 75; i <= 100; ++i) {
    age_group[i] = 4;
  }
  return age_group;
}
std::int32_t Population::death_chance(std::int32_t age, bool isfemale) const {
  std::int32_t age_chance = std::experimental::randint(0, 1000);
  if (age >= 80 && age_chance <= 148) {
    return 2;
  } else if (age >= 70 && age_chance <= 80) {
    return 2;
  } else if (age_chance <= 23) {
    return 2;
  }
  return 3;
}
void Population::daily_change_MT() {
  d_r_MT_all();
  wait_till_ref_0();
  _i_deque.pop_front();
  std::int32_t contagious = get_contagious_MT();
  std::int32_t num_to_infect = static_cast<float>(contagious) * _r0_num / float(_cont_dur) *
                               float(_h.size() / float(_pop_size));
  _daily_change[0] = num_to_infect;
  infect_n_MT(num_to_infect);
  change_status_MT_part(0, _i_deque.back().size());
}
void Population::d_r_MT_all() {
  std::int32_t oldest_group = _i_deque.front().size();
  std::int32_t chunkSize = oldest_group / 10;
  std::int32_t partSize = chunkSize;
  std::int32_t start = 0;
  std::int32_t end = partSize;
  _daily_change[1] = 0;
  _daily_change[2] = 0;
  while (oldest_group > 0) {
    assert(end <= start + oldest_group);
    increment_ref_count();
    _pool->enqueue(std::bind(&Population::d_r_MT_part, this, start, end));
    oldest_group -= partSize;
    partSize = chunkSize < oldest_group ? chunkSize : oldest_group;
    start = end;
    end = start + partSize;
  }
}
void Population::d_r_MT_part(std::int32_t start, std::int32_t end) {
  std::int32_t to_add_d = 0;
  std::int32_t to_add_r = 0;
  for (std::int32_t p = start; p < end && p < _i_deque.front().size(); ++p) {
    if (_i_deque.front()[p]->_death_chance == 2) {
      ++to_add_d;
    } else {
      ++to_add_r;
    }
  }
  {
    MutexLock l(&_mu);
    _daily_change[2] += to_add_d;
    _d += to_add_d;
    _daily_change[1] += to_add_r;
    _r += to_add_r;
    --_ref;
  }
}
std::int32_t Population::get_contagious_MT() const {
  std::int32_t contagious = 0;
  std::int32_t pos_from_end = 0;
  auto it = _i_deque.rbegin();
  while (pos_from_end < _cont_dur) {
    contagious += (*it).size();
    ++pos_from_end;
    ++it;
  }
  return contagious;
}
void Population::infect_n_MT(std::int32_t num_to_infect) {
  // if num to infect is less than or equal to _h
  if (_h.size() <= num_to_infect) {
    _i_deque.push_back(std::move(_h));
    return;
  }
  // if num to infect more than half of _h, faster to copy remaining than remove
  else if (num_to_infect * 2 > _h.size()) {
    std::int32_t remaining = _h.size() - num_to_infect;
    auto h_start = _h.begin();
    auto h_end = _h.begin() + remaining;
    std::vector<Person*> remaining_h(h_start, h_end);
    assert(remaining_h.size() + num_to_infect == _h.size());
    _i_deque.emplace_back(_h.rbegin(), _h.rbegin() + num_to_infect);
    assert(_i_deque.back().size() == num_to_infect);
    _h = std::move(remaining_h);
  }
  // num to infect is half or less than half of _h. remove last n elements of _h
  else {
    auto start = _h.rbegin();
    auto end = _h.rbegin() + num_to_infect;
    _i_deque.emplace_back(start, end);
    assert(_i_deque.back().size() == num_to_infect);
    for (std::int32_t i = 0; i < num_to_infect; ++i) {
      _h.pop_back();
    }
  }
}
/*
void Population::change_status_MT_all() {
  std::int32_t newest_group = _i_deque.back().size();
  std::int32_t chunkSize = newest_group / 10;
  std::int32_t partSize = chunkSize;
  std::int32_t start = 0;
  std::int32_t end = partSize;
  while (newest_group > 0) {
    assert(end <= start + newest_group);
    increment_ref_count();
    _pool->enqueue(
        std::bind(&Population::change_status_MT_part, this, start, end));
    newest_group -= partSize;
    partSize = chunkSize < newest_group ? chunkSize : newest_group;
    start = end;
    end = start + partSize;
  }
}
*/
void Population::change_status_MT_part(std::int32_t start, std::int32_t end) {
  for (std::int32_t p = start; p < end && p < _i_deque.back().size(); ++p) {
    _i_deque.back()[p]->_status = 1;
  }
}
void Population::increment_ref_count() {
  MutexLock l(&_mu);
  ++_ref;
}
void Population::decrement_ref_count() {
  MutexLock l(&_mu);
  --_ref;
}
void Population::wait_till_ref_0() {
  ReaderMutexLock l(&_mu);
  _mu.await([this]() { return _ref == 0; });
}
void Population::project_and_write(std::int32_t days, std::string& r0_str,
                                   std::string& mult_str) {
  std::string pathname = "/home/anna/code/exercises/probability/projections/" +
                         std::to_string(_pop_size) + "/" +
                         std::to_string(days) + "_days/" + "max_dur_" +
                         std::to_string(_max_dur) + "/r0_" + r0_str + "other" +
                         mult_str + ".csv";
  std::ofstream file(pathname);
  file << "day,case,deaths" << std::endl;
  std::int32_t today = 0;
  for (; today < _CHD.size(); ++today) {
    file << today << "," << _CHD[today][0] << "," << _CHD[today][2]
         << std::endl;
  }
  for (std::int32_t d = today + 1; d < days + today + 1; ++d) {
    daily_change_MT();
    file << d << "," << _daily_change[0] << "," << _daily_change[2]
         << std::endl;
  }
  file.close();
}

std::vector<std::int32_t> Population::project_30_day_vector() {
  std::vector<std::int32_t> this_proj;
  for (std::int32_t today = 0; today < _CHD.size(); ++today) {
    this_proj.push_back(_CHD[today][0]);
  }
  for (std::int32_t i = 0; i < 30; ++i) {
    daily_change_MT();
    this_proj.push_back(_daily_change[0]);
  }
  return this_proj;
}
}  // namespace covid

struct Model_30 {
  int _day[365];
  int size;
};

void Model_to_file(float r0_num, int max_dur, float mult) {
  std::int32_t cont_dur = 7;
  util::ThreadPool pool(10);
  std::int32_t pop_size = 8399000;
  std::int32_t days = 30;
  covid::Population this_pop(pop_size, r0_num, max_dur, cont_dur, mult, &pool);
  std::string r0_num_str =
      std::to_string(static_cast<std::int32_t>(r0_num)) + "." +
      std::to_string(static_cast<std::int32_t>(
          (r0_num - static_cast<std::int32_t>(r0_num)) * 100));
  std::string mult_str = std::to_string(static_cast<std::int32_t>(mult)) + "." +
                         std::to_string(static_cast<std::int32_t>(
                             (mult - static_cast<std::int32_t>(mult)) * 100));
  this_pop.project_and_write(days, r0_num_str, mult_str);
}
void Model_30_set_test(std::vector<float> r0_vector, int max_dur, float mult) {
  std::int32_t cont_dur = 7;
  util::ThreadPool pool(10);
  std::int32_t pop_size = 8399000;
  for (const auto& r0 : r0_vector) {
    covid::Population this_pop(pop_size, r0, max_dur, cont_dur, mult, &pool);
    std::vector<int> this_proj = this_pop.project_30_day_vector();
    Model_30 this_model;
    this_model.size = this_proj.size();
    for (std::int32_t i = 0; i < this_proj.size(); ++i) {
      this_model._day[i] = this_proj[i];
    }
    for (std::int32_t i = this_proj.size(); i < 365; ++i) {
      this_model._day[i] = 0;
    }
    std::cout << "model_30_setter for r0: " << r0 << " with " << this_model.size
              << " days COMPLETE" << std::endl;
  }
}

int main(int argc, char** argv) {
  std::vector<float> r0_vector;
  for (int i = 0; i < 6; ++i) {
    r0_vector.push_back(i);
    r0_vector.push_back(float(i + .25));
    r0_vector.push_back(float(i + .5));
    r0_vector.push_back(float(i + .75));
  }
  int max_dur = 45;
  float mult = 0;
  Model_30_set_test(r0_vector, max_dur, mult);
  return 0;
}

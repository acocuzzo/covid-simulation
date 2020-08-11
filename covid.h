#ifndef COVID_H_
#define COVID_H_

#include <iostream>
#include <list>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <deque>

#include "mutex.h"
#include "threadpool.h"

namespace covid {

struct Person {
  Person() : _status(0) {};
  Person(int32_t age, bool isfemale)
      : _status(0), _age(age), _isfemale(isfemale){};
  Person(int32_t status)
      : _status(status) {};
  Person(int32_t status, int32_t age, bool isfemale,
         int32_t death_chance)
      : _status(status),
        _age(age),
        _isfemale(isfemale),
        _death_chance(death_chance){};

  std::int32_t _status;
  std::int32_t _age;
  bool _isfemale;
  std::int32_t _death_chance;
};

class Population {
 public:
  Population(std::int32_t size, float r0_num, std::int32_t max_dur, std::int32_t cont_dur, float mult,
             util::ThreadPool* pool);

  void project_and_write(int32_t days, std::string& r0_str, std::string& mult_str);
  std::vector<int> project_30_day_vector();

  inline const std::vector<std::vector<std::int32_t>>& get_CHD() const {
    return _CHD;
  }
  inline const std::vector<std::vector<float>>& get_rba() const { return _rba; }
  inline const std::vector<std::vector<float>>& get_rbs() const { return _rbs; }
  inline std::int32_t get_num_infected() const { return _i; }
  inline std::int32_t get_num_healthy() const {return _h.size();}
  inline float get_p_infected() const { return _i / _pop_size; }
  inline float get_p_healthy() const {return _h.size()/_pop_size;}
  inline std::int32_t get_num_recovered() const { return _r; }
  inline float get_p_recovered() const { return _r / _pop_size; }
  inline std::int32_t get_num_dead() const { return _d; }
  inline float get_p_dead() const { return _d / _pop_size; }

  private:
  // build population
  void add_n_people(std::int32_t n, std::int32_t status);
  std::vector<std::vector<std::string>> read_csv(const std::string& pathname);
  std::vector<std::vector<std::int32_t>> build_CHD();
  void modify_CHD(float mult);
  std::vector<std::vector<float>> build_rba();
  std::vector<std::vector<float>> build_rbs();
  std::vector<float> generate_age_odds();
  std::unordered_map<std::int32_t, std::int32_t> build_age_groups();

  // getters to build pop
  std::int32_t d_from_CHD() const;
  float d_perc() const;
  std::int32_t r_from_CHD() const;
  float r_perc() const;
  std::int32_t i_from_CHD() const;
  float i_perc() const;
  std::vector<std::int32_t> dur_from_CHD() const;
  std::vector<float> dur_perc() const;
  std::int32_t death_chance(std::int32_t age, bool isfemale) const;
 
 //daily change
  void daily_change_MT();
  void d_r_MT_all();
  void d_r_MT_part(std::int32_t start, std::int32_t end);
  std::int32_t get_contagious_MT() const;
  void infect_n_MT(std::int32_t num_to_infect);
  void change_status_MT_all();
  void change_status_MT_part(std::int32_t start, std::int32_t end);
  void increment_ref_count();
  void decrement_ref_count();
  void wait_till_ref_0();

  std::int32_t _pop_size;
  std::vector<std::unique_ptr<Person>> _population;
  std::int32_t _max_dur;
  std::int32_t _cont_dur;
  float _mult;
  std::vector<std::vector<std::int32_t>> _CHD;
  std::vector<std::vector<float>> _rba;
  std::vector<std::vector<float>> _rbs;
  std::vector<float> _age_odds;
  std::vector<Person*> _h;
  std::deque<std::vector<Person*>> _i_deque;
  std::int32_t _i;
  std::int32_t _r;
  std::int32_t _d;
  std::unordered_map<std::int32_t, std::int32_t> _age_group;
  float _r0_num;
  std::int32_t _ref;
  Mutex _mu;
  util::ThreadPool* _pool;
  std::int32_t _daily_change[3];

  //friend class testing::Projection_Test;

};

class CSVRow {
 public:
  const std::string& operator[](std::size_t index) const {
    return m_data[index];
  }
  std::size_t size() const { return m_data.size(); }
  void readNextRow(std::istream& str) {
    std::string line;
    std::getline(str, line);
    std::stringstream lineStream(line);
    std::string cell;

    m_data.clear();
    while (std::getline(lineStream, cell, ',')) {
      m_data.push_back(cell);
    }
    if (!lineStream && cell.empty()) {
      m_data.push_back("");
    }
  }

  std::vector<std::string> m_data;
};

inline std::istream& operator>>(std::istream& str, CSVRow& data) {
  data.readNextRow(str);
  return str;
};
}  // namespace covid

#endif
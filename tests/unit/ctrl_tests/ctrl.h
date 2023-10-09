
#pragma once

#include <gtest/gtest.h>
#include <fstream>

#include "test_defs.h"
#include "ctrl/commands.h"
#include "ctrl/receiver.h"
#include "ifaces/timetable_info_interface.h"

// Universal test fixture for Receiver, Sender and Service
class ctrl : public ::testing::Test {
 protected:

  // To sort jobs, nodes and partitions (by numeric id)
  template <class T>
  struct id_comparator {
    bool operator()(const T &v1, const T &v2) const {
      return v1->get_id() < v2->get_id();
    }
  };

  // To sort resources (by names)
  template <class T>
  struct name_comparator {
    bool operator()(const T &v1, const T &v2) const {
      return v1.get_name().compare(v2.get_name());
    }
  };

  // The Original implementation is located in plugin's code. So, defining our own
  class TimetableInfoForTests : public swm::TimetableInfoInterface {
   public:
    TimetableInfoForTests(const swm::SwmTimetable *table) {
      tables_.push_back(table);
    }
    TimetableInfoForTests(const std::vector<const swm::SwmTimetable *> &tables)
      : tables_(tables) { }

    virtual ~TimetableInfoForTests() { }

    virtual const std::vector<const swm::SwmTimetable *> &tables() const override {
      return tables_;
    }
    virtual bool empty() const override { return tables_.empty(); }

   private:
    std::vector<const swm::SwmTimetable *> tables_;
  };

  void SetUp() {
    std::stringstream errors;
    ASSERT_EQ(ei_init(), 0);
    ASSERT_TRUE(factory_.load_plugins(find_plugin_dir(), &errors)) << errors.str();
    ASSERT_TRUE(scanner_.scan());
  }
  void TearDown() { }

  const swm::AlgorithmFactory *factory() const { return &factory_; }
  const swm::Scanner *scanner() const { return &scanner_; }

  std::shared_ptr<swm::util::CommandContext> create_context(const SwmUID &uid) {
    return std::shared_ptr<swm::util::CommandContext>(new swm::util::CommandContext(uid));
  }

  std::shared_ptr<swm::util::CommandInterface> create_schedule_request(
                                      const SwmUID &uid,
                                      const std::vector<std::string> &algs,
                                      const std::shared_ptr<swm::SchedulingInfoInterface> &info) {
    std::vector<swm::util::ScheduleCommand::AlgorithmSpec> alg_specs;
    alg_specs.reserve(algs.size());
    for (size_t i = 0; i < algs.size(); i++) {
      alg_specs.emplace_back(swm::util::ScheduleCommand::AlgorithmSpec(algs[i]));
    }

    std::shared_ptr<swm::util::CommandInterface> res;
    res.reset(new swm::util::ScheduleCommand(create_context(uid), alg_specs, info));
    return res;
  }

  std::shared_ptr<swm::util::CommandInterface> create_interrupt_request(const SwmUID &uid,
                                                                        const SwmUID &chain) {
    std::shared_ptr<swm::util::CommandInterface> resp;
    resp.reset(new swm::util::InterruptCommand(create_context(uid), chain));
    return resp;
  }

  std::shared_ptr<swm::util::CommandInterface> create_metrics_request(const SwmUID &uid,
                                                                      const SwmUID &chain) {
    std::shared_ptr<swm::util::CommandInterface> resp;
    resp.reset(new swm::util::MetricsCommand(create_context(uid), chain));
    return resp;
  }

  std::shared_ptr<swm::util::CommandInterface> create_exchange_request(const SwmUID &uid,
                                                                       const SwmUID &chain1,
                                                                       const SwmUID &chain2) {
    std::shared_ptr<swm::util::CommandInterface> resp;
    resp.reset(new swm::util::ExchangeCommand(create_context(uid), chain1, chain2));
    return resp;
  }

  // Fills stream with ErlBIN data (created according to JSON config)
  void prepare_input_stream(const std::string &json, std::istringstream *istr, bool *failed) {
    *failed = true;
    std::string cur_dir;
    ASSERT_TRUE(my_getcwd(&cur_dir));
    ASSERT_TRUE(my_chdir(find_swm_sched_dir()));

    try {
      std::string temp_file = find_temp_dir() + "/" + "swm-sched-tests.tmp";
      std::ofstream fstr(temp_file);
      if (!fstr.good()) {
        throw std::runtime_error("failed to create temp file");
      }
      fstr << json;
      fstr.close();

      std::string output, error;
      std::stringstream args;
      args << find_converter() << " " << temp_file;
      std::string err_string = "escript: exception error";
      if (!my_exec("escript", args.str(), &output, &error) ||
        (output.size() >= err_string.size() && output.substr(0, err_string.size()) == err_string)) {
        throw std::runtime_error("Failed to launch \"" + args.str() + "\": " + error + ", " + output);
      }
      istr->str(output);
      my_chdir(cur_dir);
      *failed = false;
    }
    catch (std::exception &ex) {
      my_chdir(cur_dir);
      ASSERT_TRUE(false) << ex.what();
    }
  }

  // With provided JSON config, tries to extract only one schedule command
  void receive_single_sched_command(const std::string &json,
                                    std::shared_ptr<swm::util::CommandInterface> *cmd) {
    ASSERT_TRUE(cmd != nullptr);
    std::istringstream istr;
    bool failed;
    prepare_input_stream(json, &istr, &failed);
    ASSERT_FALSE(failed);

    swm::util::MyQueue<std::shared_ptr<swm::util::CommandInterface> > queue(2);
    swm::util::Receiver receiver;
    ASSERT_NO_THROW(receiver.init(&queue, &istr));
    while (!receiver.finished() && queue.element_count() < queue.size()) {
      std::this_thread::yield();
    }
    ASSERT_EQ(queue.element_count(), 1);
    auto res = queue.pop();
    ASSERT_EQ(res->type(), swm::util::SWM_COMMAND_SCHEDULE);
    *cmd = res;
  }

  // Creates default JSON config
  // Note: the correctness of this config will be checked by different tests
  // If you want to change it, do not forget to update the tests!
  void construct_sched_command(std::string *json_config) {
    ASSERT_TRUE(json_config != nullptr);
    *json_config = R"(

     {
       "job": [
         { "id": "10000000-0000-0000-0000-000000000000",
           "cluster_id": "1",
           "state": "Q",
           "request": {
             "resource": { "name": "node", "count": 3 },
             "resource": { "name": "mem", "count": 1073741824 },
           }
         }
       ],
       "cluster": [
         { "id": "1",
           "state": "up",
           "scheduler": 1
         }
       ],
       "partition": [
         { "id": "1",
           "state": "down",
           "jobs_per_node": 1,
         },
         { "id": "2",
           "state": "up",
           "jobs_per_node": 2,
         }
       ],
       "node": [
         { "id": "1",
           "state_power": "up",
           "state_alloc": "free",
           "resources": {
             "resource": {"name": "cpu", "count": 24},
             "resource": {"name": "mem", "count": 68719476736},
           },
         },
         { "id": "3",
           "state_power": "down",
         }
       ],
       "rh": [
         { "cluster": "1",
           "sub": [
             { "partition": "1" },
             { "partition": "2",
               "sub": [
                 {"node": "1"},
                 {"node": "3"}
               ]
             }
           ]
         }
       ],
       "scheduler": [
         { "id": 1,
           "name": "swm-fcfs",
           "state": "up"
         }
       ]
     }

    )";
  }

 private:
  swm::AlgorithmFactory factory_;
  swm::Scanner scanner_;
};

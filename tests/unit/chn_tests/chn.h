#pragma once

#include <gtest/gtest.h>

#include "test_defs.h"
#include "alg/algorithm_factory.h"
#include "chn/chain_controller.h"
#include "ctrl/scheduling_info.h"

class chn : public ::testing::Test {
protected:
  void SetUp() {
    std::stringstream errors;
    ASSERT_TRUE(factory_.load_plugins(find_plugin_dir(), &errors)) << errors.str();

    fcfs_desc_ = nullptr;
    for (const auto desc : factory_.known_algorithms()) {
      if (desc->family_id() == "swm-fcfs") {
        fcfs_desc_ = desc;
      }
      if (desc->family_id() == "swm-dummy") {
        dummy_desc_ = desc;
      }
    }
    ASSERT_NE(fcfs_desc_, nullptr);
    ASSERT_NE(dummy_desc_, nullptr);
  }
  void TearDown() { }

  bool create_dummy_algorithms(std::vector<std::shared_ptr<swm::Algorithm> > *algs, size_t count) {
    return create_algorithms(dummy_desc_, algs, count);
  }

  bool create_fcfs_algorithms(std::vector<std::shared_ptr<swm::Algorithm> > *algs, size_t count) {
    return create_algorithms(fcfs_desc_, algs, count);
  }

  swm::util::ChainController::finish_callback empty_finish_callback(volatile bool *success
                                                                                   = nullptr) {
    return [flag = success](bool success,
                            const std::shared_ptr<swm::TimetableInfoInterface> &,
                            const std::shared_ptr<swm::util::MetricsSnapshot> &) -> void {
      if (flag != nullptr) { *flag = success; }
    };
  }

private:
  bool create_algorithms(const swm::AlgorithmDescInterface *desc,
    std::vector<std::shared_ptr<swm::Algorithm> > *algs,
    size_t count) {
    algs->resize(count);
    for (size_t i = 0; i < count; i++) {
      std::shared_ptr<swm::Algorithm> alg;
      if (factory_.create(desc, &alg)) {
        (*algs)[i] = alg;
      }
      else {
        algs->clear();
        return false;
      }
    }
    return true;
  }

  swm::AlgorithmFactory factory_;
  const swm::AlgorithmDescInterface *fcfs_desc_;
  const swm::AlgorithmDescInterface *dummy_desc_;
};

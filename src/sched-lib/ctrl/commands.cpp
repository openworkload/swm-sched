
#include "commands.h"

#include <string.h>

namespace swm {
namespace util {

//--------------------------------------
//--- ScheduleCommand::AlgorithmSpec ---
//--------------------------------------

ScheduleCommand::AlgorithmSpec::AlgorithmSpec(const std::string &family_id,
                                              const std::string *version,
                                              const ComputeUnitInterface::Type *cu)
    : family_(family_id), has_version_(version != nullptr), has_cu_(cu != nullptr) {
  
  if (has_version_) {
    version_ = *version;
  }
  if (has_cu_) {
    cu_ = *cu;
  }
}

bool ScheduleCommand::AlgorithmSpec::version_specified(std::string *version) const {
  if (has_version_) {
    if (version != nullptr) {
      *version = version_;
    }
    return true;
  }
  else {
    return false;
  }
}

bool ScheduleCommand::AlgorithmSpec
                    ::compute_unit_specified(ComputeUnitInterface::Type *cu_type) const {
  if (has_cu_) {
    if (cu_type != nullptr) {
      *cu_type = cu_;
    }
    return true;
  }
  else {
    return false;
  }
}

//-----------------------
//--- ScheduleCommand ---
//-----------------------

bool ScheduleCommand::init(const std::vector<std::unique_ptr<unsigned char[]> > &data, std::stringstream *errors) {
  std::stringstream errors_;
  if (errors == nullptr) {
    errors = &errors_;
  }

  if (data.size() != DataTypeCount) {
    *errors << "not enough data slices (" << data.size() << " provided, "
            << DataTypeCount << " expected)";
    return false;
  }

  schedulers_.clear();
  sched_info_ptr_.reset(sched_info_ = new SchedulingInfo());

  for (size_t i = 0; i < data.size(); ++i) {
    if (data[i].get() == nullptr) {
      *errors << "The " << i << "-th data slice is equal to nullptr";
      return false;
    }

    const auto data_slice = data[i].get();
    auto term = erl_decode(data_slice);
    if (term == nullptr) {
      *errors << "failed to decode the slice at position " << i;
      return false;
    }
    erl_print_term(stderr, term); std::cerr << std::endl;

    switch (i) {
      case SWM_DATA_TYPE_SCHEDULERS: {
        if (!apply_schedulers(term, errors)) {
          *errors << "Could not parse schedulers information";
          return false;
        }
        break;
      };
      case SWM_DATA_TYPE_JOBS: {
        if (!apply_jobs(term, errors)) {
          *errors << "Could not parse jobs information";
          return false;
        }
        break;
      };
      case SWM_DATA_TYPE_RH: {
        if (!apply_rh(term, errors)) {
          *errors << "Could not parse RH information";
          return false;
        }
        break;
      };
      case SWM_DATA_TYPE_GRID: {
        if (!apply_grid(term, errors)) {
          *errors << "Could not parse grid information";
          return false;
        }
        break;
      };
      case SWM_DATA_TYPE_CLUSTERS: {
        if (!apply_clusters(term, errors)) {
          *errors << "Could not parse clusters information";
          return false;
        }
        break;
      };
      case SWM_DATA_TYPE_PARTITIONS: {
        if (!apply_partitions(term, errors)) {
          *errors << "Could not parse partitions information";
          return false;
        }
        break;
      };
      case SWM_DATA_TYPE_NODES: {
        if (!apply_nodes(term, errors)) {
          *errors << "Could not parse nodes information";
          return false;
        }
        break;
      };
      default: {
        *errors << "unknown data type: " << i;
        return false;
      }
    }

    erl_free_compound(term);
  }

  sched_info_->validate_references();
  return true;
}

bool ScheduleCommand::apply_schedulers(ETERM *, std::stringstream *) {
  //2Taras: read schedulers, something like that:
  size_t nalgs = 1;
  for (size_t i = 0; i < nalgs; ++i) {
    std::string family  = "swm-fcfs";
    bool has_version    = true;
    std::string version = "1.0";
    bool cu_specified   = true;
    auto cu             = ComputeUnitInterface::Cpu;
    schedulers_.emplace_back(AlgorithmSpec(family,
                                           has_version ? &version : nullptr,
                                           cu_specified ? &cu : nullptr));
  }
  return true;
}

bool ScheduleCommand::apply_jobs(ETERM* terms, std::stringstream *) {
  auto &jobs = sched_info_->jobs_vector();
  const size_t njobs = erl_length(terms);
  jobs.reserve(njobs);
  for (size_t i = 0; i < njobs; ++i) {
    ETERM* term = erl_hd(terms);
    terms = erl_tl(terms);
    jobs.emplace_back(term);
  }
  return true;
}

static inline bool apply_rh_helper(ETERM* terms, std::vector<RhItem> *rh, std::stringstream *error) {
  if (ERL_IS_LIST(terms)) {
    const auto len = erl_length(terms);
    for (int i = 0; i < len; ++i) {
      ETERM* hd = erl_hd(terms);
      if (!hd) {
        *error << "Wrong RH format (hd=nullptr)" << std::endl;
        return false;
      }
      if (!apply_rh_helper(hd, rh, error)) {
        *error << "Could not parse RH" << std::endl;
        return false;
      }
      terms = erl_tl(terms);
    }
  }
  else if (ERL_IS_TUPLE(terms)) {
    ETERM *first = erl_element(1, terms);
    if (first == nullptr) {
      *error << "Can't get first element of RH tuple" << std::endl;
      return false;
    }
    ETERM *second = erl_element(2, terms);
    if (second == nullptr) {
      *error << "Can't get second element of RH tuple" << std::endl;
      return false;
    }

    SwmTupleAtomStr tpl;
    if (eterm_to_tuple_atom_str(first, tpl)) {
      *error << "Can't parse RH tuple" << std::endl;
      return false;
    }

    std::vector<RhItem> children;
    if (!ERL_IS_EMPTY_LIST(second)) {
      if (!apply_rh_helper(second, &children, error)) {
        return false; // error already described
      }
    }
    RhItem item(tpl.x1, tpl.x2, &children);

    rh->push_back(item);
  }
  else {
    *error << "Wrong RH item ETERM" << std::endl;
    return false;
  }

  return true;
}

bool ScheduleCommand::apply_rh(ETERM* terms, std::stringstream *error) {
  std::stringstream error_;
  if (error == nullptr) {
    error = &error_;
  }
  return apply_rh_helper(terms, &sched_info_->resource_hierarchy_vector(), error);
}

bool ScheduleCommand::apply_grid(ETERM* terms, std::stringstream *) {
  const auto len = erl_size(terms);
  if (len != 0) {
    sched_info_->set_grid(SwmGrid(terms));
  }
  return true;
}

bool ScheduleCommand::apply_clusters(ETERM* terms, std::stringstream *) {
  auto &clusters = sched_info_->clusters_vector();
  const size_t nclusters = erl_length(terms);
  clusters.reserve(nclusters);
  for (size_t i = 0; i < nclusters; ++i) {
    ETERM* term = erl_hd(terms);
    clusters.emplace_back(term);
    terms = erl_tl(terms);
  }
  return true;
}

bool ScheduleCommand::apply_partitions(ETERM* terms, std::stringstream *) {
  auto &parts = sched_info_->parts_vector();
  const size_t nparts = erl_length(terms);
  parts.reserve(nparts);
  for (size_t i = 0; i < nparts; ++i) {
    ETERM* term = erl_hd(terms);
    parts.emplace_back(term);
    terms = erl_tl(terms);
  }
  return true;
}

bool ScheduleCommand::apply_nodes(ETERM* terms, std::stringstream *) {
  auto &nodes = sched_info_->nodes_vector();
  const auto nnodes = erl_length(terms);
  nodes.reserve(nnodes);
  for (int i = 0; i < nnodes; ++i) {
    ETERM* term = erl_hd(terms);
    nodes.push_back(term);
    terms = erl_tl(terms);
  }
  return true;
}

//------------------------
//--- InterruptCommand ---
//------------------------

bool InterruptCommand::init(const std::vector<std::unique_ptr<unsigned char[]> > &,
                            std::stringstream *) {
  //2Taras: read chain id
  chain_ = "42";
  return true;
}

//----------------------
//--- MetricsCommand ---
//----------------------

bool MetricsCommand::init(const std::vector<std::unique_ptr<unsigned char[]> > &,
                          std::stringstream *) {
  //2Taras: read chain id
  chain_ = "42";
  return true;
}

//-----------------------
//--- ExchangeCommand ---
//-----------------------

bool ExchangeCommand::init(const std::vector<std::unique_ptr<unsigned char[]> > &,
                           std::stringstream *) {
  //2Taras: read chain identifiers
  source_chain_ = "42";
  target_chain_ = "42";
  return true;
}

} // util
} // swm

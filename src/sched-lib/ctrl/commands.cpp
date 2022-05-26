
#include "commands.h"

#include "wm_io.h"

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

bool ScheduleCommand::init(const std::vector<std::unique_ptr<char[]>> &data, std::stringstream *errors) {
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
    char* buf = data[i].get();
    if (!buf) {
      *errors << "The " << i << "-th data slice is empty";
      return false;
    }

    int index = 0;
    int version = 0;
    if (ei_decode_version(buf, &index, &version)) {
      *errors << "Could not decode erlang binary version at position " << index << std::endl;
      return false;
    }
    if (version != ERLANG_BINARY_FORMAT_VERSION) {
      std::cerr << "Wrong erlang binary format version: " << version
                << ", expected: " << ERLANG_BINARY_FORMAT_VERSION << std::endl;
      return false;
    }

    print_ei_buf(buf, index);

    switch (i) {
      case SWM_DATA_TYPE_SCHEDULERS: {
        if (!apply_schedulers(buf, index, errors)) {
          *errors << "Could not parse schedulers information";
          return false;
        }
        break;
      };
      case SWM_DATA_TYPE_JOBS: {
        if (!apply_jobs(buf, index, errors)) {
          *errors << "Could not parse jobs information";
          return false;
        }
        break;
      };
      case SWM_DATA_TYPE_RH: {
        if (!apply_rh(buf, index, errors)) {
          *errors << "Could not parse RH information";
          return false;
        }
        break;
      };
      case SWM_DATA_TYPE_GRID: {
        if (!apply_grid(buf, index, errors)) {
          *errors << "Could not parse grid information";
          return false;
        }
        break;
      };
      case SWM_DATA_TYPE_CLUSTERS: {
        if (!apply_clusters(buf, index, errors)) {
          *errors << "Could not parse clusters information";
          return false;
        }
        break;
      };
      case SWM_DATA_TYPE_PARTITIONS: {
        if (!apply_partitions(buf, index, errors)) {
          *errors << "Could not parse partitions information";
          return false;
        }
        break;
      };
      case SWM_DATA_TYPE_NODES: {
        if (!apply_nodes(buf, index, errors)) {
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
  }

  sched_info_->validate_references();
  return true;
}

bool ScheduleCommand::apply_schedulers(char *, int &, std::stringstream *) {
  //TODO: read schedulers, something like that:
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

bool ScheduleCommand::apply_jobs(char *buf, int &index, std::stringstream *error) {
  auto &jobs = sched_info_->jobs_vector();

  int term_size = 0;
  int term_type = 0;
  if (ei_get_type(buf, &index, &term_type, &term_size)) {
    *error << "Could not get job list term type at position " << index << std::endl;
    return false;
  }
  if (term_type != ERL_LIST_EXT && term_type != ERL_NIL_EXT) {
    *error << "Could not parse jobs list term at " << index << ": not a list" << std::endl;
    return false;
  }
  int list_size = 0;
  if (ei_decode_list_header(buf, &index, &list_size)) {
    *error << "Could not decode ei list header at " << index << std::endl;
    return false;
  }
  if (list_size == 0) {
    *error << "Empty jobs array" << std::endl;
    jobs.clear();
    return true;
  }

  jobs.reserve(list_size);
  for (int i = 0; i < term_size; ++i) {
    jobs.emplace_back(buf, index);
  }
  ei_skip_term(buf, &index);  // last element of a list is empty list
  return true;
}

static inline bool apply_rh_helper(char *buf, int &index, std::vector<RhItem> *rh, std::stringstream *error) {
  int term_size = 0;
  int term_type = 0;
  if (ei_get_type(buf, &index, &term_type, &term_size)) {
    *error << "Could not get job list term type at position " << index << std::endl;
    return false;
  }

  if (term_type == ERL_LIST_EXT || term_type == ERL_NIL_EXT) {
    int list_size = 0;
    if (ei_decode_list_header(buf, &index, &list_size)) {
      *error << "Could not decode ei list header at " << index << std::endl;
      return false;
    }
    if (list_size == 0) {
      *error << "Empty RH list" << std::endl;
      return true;
    }
    for (int i = 0; i < list_size; ++i) {
      if (!apply_rh_helper(buf, index, rh, error)) {
        *error << "Could not parse RH list element number " << i << std::endl;
        return false;
      }
    }
    ei_skip_term(buf, &index);  // last element of a list is empty list

  } else if (term_type == ERL_SMALL_TUPLE_EXT || term_type == ERL_LARGE_TUPLE_EXT) {

    int arity = 0;
    if (ei_decode_tuple_header(buf, &index, &arity)) {
      *error << "Could not decode RH tuple header" << std::endl;
      return false;
    }
    if (arity != 2) {
      *error << "RH tuple arity is not 2, but " << arity << std::endl;
      return false;
    }

    SwmTupleAtomStr tpl;
    if (ei_buffer_to_tuple_atom_str(buf, index, tpl)) {
      *error << "Can't parse RH tuple element" << std::endl;
      return false;
    }

    std::vector<RhItem> children;
    if (!apply_rh_helper(buf, index, &children, error)) {
      return false;
    }
    rh->emplace_back(std::get<0>(tpl), std::get<1>(tpl), &children);

  } else {
    *error << "Wrong RH item erlang term type: " << term_type << std::endl;
    return false;
  }

  return true;
}

bool ScheduleCommand::apply_rh(char *buf, int &index, std::stringstream *error) {
  std::stringstream error_;
  if (error == nullptr) {
    error = &error_;
  }
  return apply_rh_helper(buf, index, &sched_info_->resource_hierarchy_vector(), error);
}

bool ScheduleCommand::apply_grid(char *buf, int &index, std::stringstream *) {
  int term_size = 0;
  int term_type = 0;
  if (ei_get_type(buf, &index, &term_type, &term_size)) {
    std::cerr << "Could not get grid term type at position " << index << std::endl;
    return false;
  }
  if (term_size == 0) {  // expected scenario
    if (ei_skip_term(buf, &index)) {
      std::cerr << "Could not skip empty SwmGrid term: ";
      ei_print_term(stderr, buf, &index);
      std::cerr << std::endl;
      return false;
    }
    return true;
  }

  sched_info_->set_grid(SwmGrid(buf, index));
  return true;
}

bool ScheduleCommand::apply_clusters(char *buf, int &index, std::stringstream *error) {
  auto &clusters = sched_info_->clusters_vector();

  int term_size = 0;
  int term_type = 0;
  if (ei_get_type(buf, &index, &term_type, &term_size)) {
    *error << "Could not get clusters list term type at position " << index << std::endl;
    return false;
  }
  if (term_type != ERL_LIST_EXT && term_type != ERL_NIL_EXT) {
    *error << "Clusters are not packed in a list at position " << index << std::endl;
    return false;
  }

  int list_size = 0;
  if (ei_decode_list_header(buf, &index, &list_size)) {
    *error << "Could not decode ei list header at " << index << std::endl;
    return false;
  }
  if (list_size == 0) {
    *error << "Empty clusters array" << std::endl;
    clusters.clear();
    return true;
  }

  if (list_size) {
    clusters.reserve(list_size);
    for (int i = 0; i < list_size; ++i) {
      clusters.emplace_back(buf, index);
    }
    ei_skip_term(buf, &index);  // last element of a list is empty list
  }
  return true;
}

bool ScheduleCommand::apply_partitions(char *buf, int &index, std::stringstream *error) {
  auto &parts = sched_info_->parts_vector();

  int term_size = 0;
  int term_type = 0;
  if (ei_get_type(buf, &index, &term_type, &term_size)) {
    *error << "Could not get partitions list term type at position " << index << std::endl;
    return false;
  }
  if (term_type != ERL_LIST_EXT && term_type != ERL_NIL_EXT) {
    *error << "Partitions are not packed in a list at position " << index << std::endl;
    return false;
  }

  int list_size = 0;
  if (ei_decode_list_header(buf, &index, &list_size)) {
    *error << "Could not decode ei list header at " << index << std::endl;
    return false;
  }
  if (list_size == 0) {
    *error << "Empty clusters array" << std::endl;
    parts.clear();
    return true;
  }

  if (list_size) {
    parts.reserve(list_size);
    for (int i = 0; i < list_size; ++i) {
      parts.emplace_back(buf, index);
    }
    ei_skip_term(buf, &index);  // last element of a list is empty list
  }
  return true;
}

bool ScheduleCommand::apply_nodes(char *buf, int &index, std::stringstream *error) {
  auto &nodes = sched_info_->nodes_vector();

  int term_size = 0;
  int term_type = 0;
  if (ei_get_type(buf, &index, &term_type, &term_size)) {
    *error << "Could not get nodes list term type at position " << index << std::endl;
    return false;
  }
  if (term_type != ERL_LIST_EXT && term_type != ERL_NIL_EXT) {
    *error << "Nodes are not packed in a list at position " << index << std::endl;
    return false;
  }

  int list_size = 0;
  if (ei_decode_list_header(buf, &index, &list_size)) {
    *error << "Could not decode ei list header at " << index << std::endl;
    return false;
  }
  if (list_size == 0) {
    *error << "Empty clusters array" << std::endl;
    nodes.clear();
    return true;
  }

  if (list_size) {
    nodes.reserve(list_size);
    for (int i = 0; i < list_size; ++i) {
      nodes.emplace_back(buf, index);
    }
    ei_skip_term(buf, &index);  // last element of a list is empty list
  }
  return true;
}

//------------------------
//--- InterruptCommand ---
//------------------------

bool InterruptCommand::init(const std::vector<std::unique_ptr<char[]> > &,
                            std::stringstream *) {
  //TODO: read chain id
  chain_ = "42";
  return true;
}

//----------------------
//--- MetricsCommand ---
//----------------------

bool MetricsCommand::init(const std::vector<std::unique_ptr<char[]> > &,
                          std::stringstream *) {
  //TODO: read chain id
  chain_ = "42";
  return true;
}

//-----------------------
//--- ExchangeCommand ---
//-----------------------

bool ExchangeCommand::init(const std::vector<std::unique_ptr<char[]> > &,
                           std::stringstream *) {
  //TODO: read chain identifiers
  source_chain_ = "42";
  target_chain_ = "42";
  return true;
}

} // util
} // swm

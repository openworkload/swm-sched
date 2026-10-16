
#include "commands.h"

#include "ei_bounds.h"
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
  } else {
    return false;
  }
}

bool ScheduleCommand::AlgorithmSpec ::compute_unit_specified(ComputeUnitInterface::Type *cu_type) const {
  if (has_cu_) {
    if (cu_type != nullptr) {
      *cu_type = cu_;
    }
    return true;
  } else {
    return false;
  }
}

//-----------------------
//--- ScheduleCommand ---
//-----------------------

bool ScheduleCommand::init(const std::vector<ReceiverSlice> &data, std::stringstream *errors) {
  std::stringstream errors_;
  if (errors == nullptr) {
    errors = &errors_;
  }

  if (data.size() != DataTypeCount) {
    *errors << "not enough data slices (" << data.size() << " provided, " << DataTypeCount << " expected)";
    return false;
  }

  schedulers_.clear();
  sched_info_ptr_.reset(sched_info_ = new SchedulingInfo());

  for (size_t i = 0; i < data.size(); ++i) {
    char *buf = data[i].data.get();
    const uint32_t buf_size = data[i].size;
    if (!buf || buf_size == 0) {
      *errors << "The " << i << "-th data slice is empty";
      return false;
    }

    int index = 0;
    int version = 0;
    if (ei_decode_version(buf, &index, &version)) {
      *errors << "Could not decode erlang binary version at position " << index << std::endl;
      return false;
    }
    if (!ei_require_index(index, buf_size, errors, "ei_decode_version")) {
      return false;
    }
    if (version != ERLANG_BINARY_FORMAT_VERSION) {
      *errors << "Wrong erlang binary format version: " << version << ", expected: " << ERLANG_BINARY_FORMAT_VERSION
              << std::endl;
      return false;
    }

    // Do not dump full EI entities; counts are logged after parsing.
    switch (i) {
      case SWM_DATA_TYPE_SCHEDULERS: {
        if (!apply_schedulers(buf, buf_size, index, errors)) {
          *errors << "Could not parse schedulers information";
          return false;
        }
        break;
      };
      case SWM_DATA_TYPE_JOBS: {
        if (!apply_jobs(buf, buf_size, index, errors)) {
          *errors << "Could not parse jobs information";
          return false;
        }
        break;
      };
      case SWM_DATA_TYPE_RH: {
        if (!apply_rh(buf, buf_size, index, errors)) {
          *errors << "Could not parse RH information";
          return false;
        }
        break;
      };
      case SWM_DATA_TYPE_GRID: {
        if (!apply_grid(buf, buf_size, index, errors)) {
          *errors << "Could not parse grid information";
          return false;
        }
        break;
      };
      case SWM_DATA_TYPE_CLUSTERS: {
        if (!apply_clusters(buf, buf_size, index, errors)) {
          *errors << "Could not parse clusters information";
          return false;
        }
        break;
      };
      case SWM_DATA_TYPE_PARTITIONS: {
        if (!apply_partitions(buf, buf_size, index, errors)) {
          *errors << "Could not parse partitions information";
          return false;
        }
        break;
      };
      case SWM_DATA_TYPE_NODES: {
        if (!apply_nodes(buf, buf_size, index, errors)) {
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
  swm_logi("Parsed entities: schedulers=%zu jobs=%zu nodes=%zu clusters=%zu partitions=%zu",
           schedulers_.size(),
           sched_info_->jobs().size(),
           sched_info_->nodes().size(),
           sched_info_->clusters().size(),
           sched_info_->parts().size());
  return true;
}

bool ScheduleCommand::apply_schedulers(char *, uint32_t, int &, std::stringstream *) {
  // TODO: read schedulers, something like that:
  size_t nalgs = 1;
  for (size_t i = 0; i < nalgs; ++i) {
    std::string family = "swm-fcfs";
    bool has_version = true;
    std::string version = "1.0";
    bool cu_specified = true;
    auto cu = ComputeUnitInterface::Cpu;
    schedulers_.emplace_back(AlgorithmSpec(family, has_version ? &version : nullptr, cu_specified ? &cu : nullptr));
  }
  return true;
}

namespace {

template <typename Entity>
bool decode_entity_list(
    char *buf, uint32_t buf_size, int &index, std::vector<Entity> *out, std::stringstream *error, const char *label) {
  int term_size = 0;
  int term_type = 0;
  if (ei_get_type(buf, &index, &term_type, &term_size)) {
    *error << "Could not get " << label << " list term type at position " << index << std::endl;
    return false;
  }
  if (!ei_require_index(index, buf_size, error, label)) {
    return false;
  }
  if (term_type != ERL_LIST_EXT && term_type != ERL_NIL_EXT) {
    *error << label << " are not packed in a list at position " << index << std::endl;
    return false;
  }

  int list_size = 0;
  if (ei_decode_list_header(buf, &index, &list_size)) {
    *error << "Could not decode ei list header at " << index << std::endl;
    return false;
  }
  if (!ei_require_index(index, buf_size, error, "ei_decode_list_header") ||
      !ei_require_list_size(list_size, error, label)) {
    return false;
  }
  if (list_size == 0) {
    out->clear();
    return true;
  }

  out->reserve(static_cast<size_t>(list_size));
  for (int i = 0; i < list_size; ++i) {
    const int before = index;
    try {
      out->emplace_back(buf, index);
    } catch (const std::exception &ex) {
      *error << "Failed to parse " << label << " element " << i << ": " << ex.what() << std::endl;
      return false;
    }
    if (index <= before) {
      *error << "Failed to parse " << label << " element " << i << ": index did not advance" << std::endl;
      return false;
    }
    if (!ei_require_index(index, buf_size, error, label)) {
      return false;
    }
  }
  if (ei_skip_term(buf, &index)) {
    *error << "Could not skip " << label << " list tail" << std::endl;
    return false;
  }
  return ei_require_index(index, buf_size, error, label);
}

}  // namespace

bool ScheduleCommand::apply_jobs(char *buf, uint32_t buf_size, int &index, std::stringstream *error) {
  return decode_entity_list(buf, buf_size, index, &sched_info_->jobs_vector(), error, "jobs");
}

static bool apply_rh_helper(
    char *buf, uint32_t buf_size, int &index, std::vector<RhItem> *rh, std::stringstream *error) {
  int term_size = 0;
  int term_type = 0;
  if (ei_get_type(buf, &index, &term_type, &term_size)) {
    *error << "Could not get RH term type at position " << index << std::endl;
    return false;
  }
  if (!ei_require_index(index, buf_size, error, "RH")) {
    return false;
  }

  if (term_type == ERL_LIST_EXT || term_type == ERL_NIL_EXT) {
    int list_size = 0;
    if (ei_decode_list_header(buf, &index, &list_size)) {
      *error << "Could not decode ei list header at " << index << std::endl;
      return false;
    }
    if (!ei_require_index(index, buf_size, error, "RH list") || !ei_require_list_size(list_size, error, "RH")) {
      return false;
    }
    if (list_size == 0) {
      return true;
    }
    for (int i = 0; i < list_size; ++i) {
      if (!apply_rh_helper(buf, buf_size, index, rh, error)) {
        *error << "Could not parse RH list element number " << i << std::endl;
        return false;
      }
    }
    if (ei_skip_term(buf, &index)) {
      *error << "Could not skip RH list tail" << std::endl;
      return false;
    }
    return ei_require_index(index, buf_size, error, "RH list tail");

  } else if (term_type == ERL_SMALL_TUPLE_EXT || term_type == ERL_LARGE_TUPLE_EXT) {
    int arity = 0;
    if (ei_decode_tuple_header(buf, &index, &arity)) {
      *error << "Could not decode RH tuple header" << std::endl;
      return false;
    }
    if (!ei_require_index(index, buf_size, error, "RH tuple")) {
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
    if (!ei_require_index(index, buf_size, error, "RH tuple element")) {
      return false;
    }

    std::vector<RhItem> children;
    if (!apply_rh_helper(buf, buf_size, index, &children, error)) {
      return false;
    }
    rh->emplace_back(std::get<0>(tpl), std::get<1>(tpl), &children);

  } else {
    *error << "Wrong RH item erlang term type: " << term_type << std::endl;
    return false;
  }

  return true;
}

bool ScheduleCommand::apply_rh(char *buf, uint32_t buf_size, int &index, std::stringstream *error) {
  std::stringstream error_;
  if (error == nullptr) {
    error = &error_;
  }
  return apply_rh_helper(buf, buf_size, index, &sched_info_->resource_hierarchy_vector(), error);
}

bool ScheduleCommand::apply_grid(char *buf, uint32_t buf_size, int &index, std::stringstream *error) {
  std::stringstream error_;
  if (error == nullptr) {
    error = &error_;
  }
  int term_size = 0;
  int term_type = 0;
  if (ei_get_type(buf, &index, &term_type, &term_size)) {
    *error << "Could not get grid term type at position " << index << std::endl;
    return false;
  }
  if (!ei_require_index(index, buf_size, error, "grid")) {
    return false;
  }
  if (term_size == 0) {  // expected scenario
    if (ei_skip_term(buf, &index)) {
      *error << "Could not skip empty SwmGrid term";
      return false;
    }
    return ei_require_index(index, buf_size, error, "grid skip");
  }

  const int before = index;
  try {
    sched_info_->set_grid(SwmGrid(buf, index));
  } catch (const std::exception &ex) {
    *error << "Failed to parse grid: " << ex.what();
    return false;
  }
  if (index <= before || !ei_require_index(index, buf_size, error, "grid")) {
    *error << "Failed to parse grid: index out of range";
    return false;
  }
  return true;
}

bool ScheduleCommand::apply_clusters(char *buf, uint32_t buf_size, int &index, std::stringstream *error) {
  return decode_entity_list(buf, buf_size, index, &sched_info_->clusters_vector(), error, "clusters");
}

bool ScheduleCommand::apply_partitions(char *buf, uint32_t buf_size, int &index, std::stringstream *error) {
  return decode_entity_list(buf, buf_size, index, &sched_info_->parts_vector(), error, "partitions");
}

bool ScheduleCommand::apply_nodes(char *buf, uint32_t buf_size, int &index, std::stringstream *error) {
  return decode_entity_list(buf, buf_size, index, &sched_info_->nodes_vector(), error, "nodes");
}

//------------------------
//--- InterruptCommand ---
//------------------------

bool InterruptCommand::init(const std::vector<ReceiverSlice> &, std::stringstream *) {
  // TODO: read chain id
  chain_ = "42";
  return true;
}

//----------------------
//--- MetricsCommand ---
//----------------------

bool MetricsCommand::init(const std::vector<ReceiverSlice> &, std::stringstream *) {
  // TODO: read chain id
  chain_ = "42";
  return true;
}

//-----------------------
//--- ExchangeCommand ---
//-----------------------

bool ExchangeCommand::init(const std::vector<ReceiverSlice> &, std::stringstream *) {
  // TODO: read chain identifiers
  source_chain_ = "42";
  target_chain_ = "42";
  return true;
}

}  // namespace util
}  // namespace swm

#ifndef __EXP_GET_LATENCY_H__
#define __EXP_GET_LATENCY_H__

#include "experiment.h"

#include "statistics.h"

#include <sstream>
#include <stdexcept>
#include <vector>

extern Data * g_data;
extern std::mutex g_write_lock;

class ExperimentGet : public Experiment
{ 
public:
  std::vector<double> _start_time;
  std::vector<double> _latencies;
  BinStatistics _latency_stats;

  ExperimentGet(struct ProgramOptions options): Experiment("get",  options)
    , _start_time()
    , _latencies()
    , _latency_stats()
  {
  }

  void initialize_custom(unsigned /* core */) override
  {
    _latency_stats = BinStatistics(bin_count(), bin_threshold_min(), bin_threshold_max());
  }

  bool do_work(unsigned core) override
  {
    // handle first time setup
    if(_first_iter) 
    {
      _pool_element_end = -1;
      // seed the pool with elements from _data
      _populate_pool_to_capacity(core);

      wait_for_delayed_start(core);

      PLOG("[%u] Starting Get experiment...", core);

      _first_iter = false;
    }     

    // end experiment if we've reached the total number of components
    if ( _i + 1 == pool_num_objects() )
    {
      PINF("[%u] reached total number of components. Exiting.", core);
      return false; 
    }

    // check time it takes to complete a single get operation

    try
    {
      void * pval = nullptr; /* change to get semantics requires initializaitonof pval */
      size_t pval_len;

      timer.start();
      auto rc = store()->get(pool(), g_data->key(_i), pval, pval_len);
      timer.stop();
      if ( rc != S_OK )
      {
        std::ostringstream e;
	e << "_pool_element_end = " << pool_element_end() << " get rc != S_OK: " << rc << " @ _i = " << _i;
        PERR("[%u] %s. Exiting.", core, e.str().c_str());
        throw std::runtime_error(e.str());
      }  
      store()->free_memory(pval);
    }
    catch(...)
    {
      PERR("%s", "get call threw exception! Ending experiment.");
      throw std::exception();
    }

    _update_data_process_amount(core, _i);

    double lap_time = timer.get_lap_time_in_seconds();
    double time_since_start = timer.get_time_in_seconds();

    // store the information for later use
    _latency_stats.update(lap_time);
    _start_time.push_back(time_since_start);
    _latencies.push_back(lap_time);

    ++_i;  // increment after running so all elements get used

    if (_i == unsigned(pool_element_end()) + 1)
    {
      try
      {
        _erase_pool_entries_in_range(pool_element_start(), pool_element_end());
        _populate_pool_to_capacity(core);
      }
      catch(...)
      {
        PERR("%s", "failed during erasing and repopulation");
        throw std::exception();
      }

      if (is_verbose())
      {
        std::stringstream debug_message;
        debug_message << "pool repopulated: " << _i;
        _debug_print(core, debug_message.str());
      }
    }
    return true;
  }

  void cleanup_custom(unsigned core)  
  {
    double run_time = timer.get_time_in_seconds();
    double iops = double(_i) / run_time;
    PINF("[%u] get: IOPS: %2g in %2g seconds", core, iops, run_time);
    _update_aggregate_iops(iops);
    
    double throughput = _calculate_current_throughput();
    PINF("[%u] get: THROUGHPUT: %.2f MB/s (%ld bytes over %.3f seconds)", core, throughput, total_data_processed(), run_time);

    // compute _start_time_stats pre-lock
    BinStatistics start_time_stats = _compute_bin_statistics_from_vectors(_latencies, _start_time, bin_count(), _start_time.front(), _start_time.at(_i-1), _i); 

    if ( is_json_reporting() )
    {
      std::lock_guard<std::mutex> g(g_write_lock);

      // get existing results, read to document variable
      rapidjson::Document document = _get_report_document();

      // collect latency stats
      rapidjson::Value latency_object = _add_statistics_to_report("latency", _latency_stats, document);
      rapidjson::Value timing_object = _add_statistics_to_report("start_time", start_time_stats, document);
      rapidjson::Value iops_object;
      rapidjson::Value throughput_object;

      iops_object.SetDouble(iops);
      throughput_object.SetDouble(throughput);

      // save everything
      rapidjson::Value experiment_object(rapidjson::kObjectType);

      experiment_object.AddMember("IOPS", iops_object, document.GetAllocator());
      experiment_object.AddMember("throughput (MB/s)", throughput_object, document.GetAllocator());
      experiment_object.AddMember("latency", latency_object, document.GetAllocator());
      experiment_object.AddMember("start_time", timing_object, document.GetAllocator()); 
       
      _report_document_save(document, core, experiment_object);
      _print_highest_count_bin(_latency_stats, core);
    }
  }
};


#endif //  __EXP_GET_LATENCY_H__

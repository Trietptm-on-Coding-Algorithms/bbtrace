#pragma once

#include <mutex>
#include <condition_variable>
#include <sstream>
#include <queue>

#define WITHOUT_DR
#include "datatypes.h"

#include "logparser.h"
#include "threadinfo.hpp"
#include "observer.hpp"

typedef std::map<uint, uint> map_uint_uint_t;
typedef std::map<uint, uint64> map_uint_uint64_t;
typedef std::map<app_pc, std::string> map_app_pc_string_t;

enum RunnerMessageType {
  kMsgUndefined = 0,
  kMsgCreateThread,
  kMsgResumeThread,
  kMsgThreadFinished,
  kMsgRequestStop
};

struct RunnerMessage {
  uint thread_id;
  RunnerMessageType msg_type;
  std::string data;
};

struct SyncSequence {
public:
  uint seq;
  uint64 ts;

  SyncSequence(): seq(0), ts(0) {}
};

struct ThreadStats {
public:
  uint bb_counts;
  uint64 ts;

  ThreadStats(): bb_counts(0), ts(0) {}

  void
  Apply(ThreadInfo &thread_info)
  {
    bb_counts = thread_info.bb_count;
    ts = thread_info.now_ts;
  }
};

typedef std::map<uint, SyncSequence> SyncSequenceMap;
typedef std::map<uint, ThreadInfo> ThreadInfoMap;
typedef std::map<uint, ThreadStats> ThreadStatsMap;

class LogRunnerObserver;

class LogRunner: public LogRunnerInterface
{
public:
  enum RunPhases {
    kPhaseNone = 0,
    kPhasePre,
    kPhasePost
  };

  LogRunner() {}
  static LogRunner* GetInstance();

  // Contracts
  virtual std::string GetPrefix() override;
  virtual std::string GetExecutable() override;
  virtual void RequestToStop() override;

  void AddObserver(LogRunnerObserver *observer);
  void ListObservers();
  bool Open(std::string &filename);
  void SetExecutable(std::string exename);
  void FinishThread(ThreadInfo &thread_info);

  bool Step(ThreadInfoMap::iterator &it_thread);
  bool ThreadStep(ThreadInfo &thread_info);

  bool Run(RunPhases phase = kPhaseNone);
  bool RunMT();
  static void ThreadRun(ThreadInfo &thread_info);
  void PostMessage(uint thread_id, RunnerMessageType msg_type, std::string &data);

  void DoCommand(int argc, const char* argv[]);

  void ThreadWaitCritSec(ThreadInfo &thread_info);
  void ThreadWaitEvent(ThreadInfo &thread_info);
  void ThreadWaitMutex(ThreadInfo &thread_info);
  void ThreadWaitRunning(ThreadInfo &thread_info);

  void CheckPending(ThreadInfo &thread_info)
  {
    ThreadWaitCritSec(thread_info);
    ThreadWaitEvent(thread_info);
    ThreadWaitMutex(thread_info);
    ThreadWaitRunning(thread_info);
  }

  void ApiCallRet(ThreadInfo &thread_info);

  void OnCreateThread(DataFlowApiCall &apicall, uint64 ts);
  void OnResumeThread(DataFlowApiCall &apicall, uint64 ts);

  void Summary();

  void FilterApiCall(std::string &name)
  {
    filter_apicall_names_.push_back(name);
  }

  void SaveSymbols(std::ostream &out);
  void SaveState(std::ostream &out);
  void Dump(int indent = 0);

  void RestoreSymbols(std::istream &in);
  void RestoreState(std::istream &in);

  std::mutex resume_mx_;
  std::condition_variable resume_cv_;
  ThreadInfoMap &info_threads() { return info_threads_; }

protected:
  ThreadInfoMap info_threads_;
  ThreadStatsMap stats_threads_;
  std::string filename_;
  std::string exename_;
  std::vector<uint> filter_apicall_addrs_;
  std::vector<std::string> filter_apicall_names_;

  bool request_stop_;
  bool is_multithread_;

  void DoKindBB(ThreadInfo &thread_info, mem_ref_t &buf_bb);
  void DoEndBB(ThreadInfo &thread_info /* , bb mem read/write */);
  void DoKindSymbol(ThreadInfo &thread_info, buf_symbol_t &buf_sym);
  void DoKindLibCall(ThreadInfo &thread_info, buf_lib_call_t &buf_libcall);
  void DoKindLibRet(ThreadInfo &thread_info, buf_lib_ret_t &buf_libret);
  void DoKindArgs(ThreadInfo &thread_info, buf_event_t &buf_args);
  void DoKindString(ThreadInfo &thread_info, buf_string_t &buf_str);
  void DoKindSync(ThreadInfo &thread_info, buf_event_t &buf_sync);
  void DoKindWndProc(ThreadInfo &thread_info, buf_event_t &buf_wndproc);
  void DoMemRW(ThreadInfo &thread_info, mem_ref_t &mem_rw, bool is_write);
  void DoMemLoop(ThreadInfo &thread_info, mem_ref_t &mem_loop);
  void OnApiCall(uint thread_id, DataFlowApiCall &apicall_ret);
  void OnApiUntracked(uint thread_id, DataFlowStackItem &bb_untracked_api);
  void OnBB(uint thread_id, DataFlowStackItem &last_bb, DataFlowMemAccesses &memaccesses);
  void OnThread(uint thread_id, uint handle_id, uint sp);
  void OnPush(uint thread_id, DataFlowStackItem &the_bb, DataFlowApiCall *apicall_now = nullptr);
  void OnPop(uint thread_id, DataFlowStackItem &the_bb);
  void OnStart();
  void OnFinish();

private:
  map_app_pc_string_t symbol_names_;
  SyncSequenceMap wait_seqs_; // hmutex / hevent
  SyncSequenceMap critsec_seqs_; // critsec

  std::mutex message_mu_;
  std::condition_variable message_cv_;
  std::queue<RunnerMessage> messages_;
  std::vector<LogRunnerObserver*> observers_;
};

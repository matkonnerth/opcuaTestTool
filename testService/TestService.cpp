#include "../api/Request.h"
#include "../api/Server.h"
#include "Config.h"
#include "JobScheduler.h"
#include <future>
#include <iostream>
#include <signal.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <type_traits>
#include <unistd.h>
#include "Repl.h"

namespace opctest {

using opctest::api::GetJobLogRequest;
using opctest::api::GetJobLogResponse;
using opctest::api::GetJobRequest;
using opctest::api::GetJobResponse;
using opctest::api::GetJobsRequest;
using opctest::api::GetScriptRequest;
using opctest::api::GetScriptResponse;
using opctest::api::GetScriptsResponse;
using opctest::api::NewJobRequest;
using opctest::api::NewJobResponse;
using opctest::api::Response;
using opctest::api::UpdateScriptRequest;
using opctest::api::GetTargetsRequest;
using opctest::api::GetTargetsResponse;
using opctest::api::NewLineReplRequest;
using opctest::api::CloneScriptRepoRequest;
using opctest::api::CloneScriptRepoResponse;

class TestService
{
public:
   TestService(const std::string& workingDir, const std::string& repo, std::function<void(const std::string& event, const std::string& data)> eventCallback)
   : m_repl{ workingDir, eventCallback }
   {
      auto logger = spdlog::get("TestService");
      logger->info("init testService");
      scheduler = std::make_unique<JobScheduler>(workingDir, repo);
      scheduler->setJobFinishedCallback(eventCallback);
   }

   void handleSigChld(int sig)
   {
      (void)sig;
      pid_t pid = wait(nullptr);
      scheduler->jobFinished(pid);
   }

   NewJobResponse newJob(const NewJobRequest& req)
   {

      auto id = scheduler->create(req.jsonRequest);
      NewJobResponse resp{};
      resp.ok = true;
      resp.id = id;
      auto logger = spdlog::get("TestService");
      logger->info("newJob, id: {}", id);
      return resp;
   }

   GetJobResponse getJobs(const GetJobsRequest& req)
   {
      GetJobResponse resp{};
      resp.ok = true;
      resp.data = scheduler->getFinishedJobs(req.from, req.max);
      return resp;
   }

   GetJobResponse getFinishedJob(const GetJobRequest& req)
   {
      GetJobResponse resp{};
      resp.ok = true;
      resp.data = scheduler->getFinishedJob(req.id);
      return resp;
   }

   GetScriptsResponse getScripts()
   {
      GetScriptsResponse resp{};
      resp.ok = true;
      resp.data = scheduler->getScripts();
      return resp;
   }

   GetScriptResponse getScript(const GetScriptRequest& req)
   {
      GetScriptResponse resp{};
      resp.ok = true;
      resp.content = scheduler->getScript(req.name);
      return resp;
   }

   Response updateScript(const UpdateScriptRequest& req)
   {
      Response resp{};
      resp.ok = true;
      scheduler->updateScript(req.name, req.content);
      return resp;
   }

   GetJobLogResponse getJobLog(const GetJobLogRequest& req)
   {
      GetJobLogResponse resp{};
      resp.ok = true;
      resp.data = scheduler->getJobLog(req.id);
      return resp;
   }

   GetTargetsResponse getTargets()
   {
      GetTargetsResponse resp{};
      resp.ok = true;
      resp.data = scheduler->getTargets();
      return resp;
   }

   Response newLineRepl(const NewLineReplRequest& req)
   {
      Response resp {true};
      m_repl.newLine(req.content);
      return resp;
   }

   CloneScriptRepoResponse cloneScriptRepo()
   {
      CloneScriptRepoResponse resp{true};
      scheduler->cloneScriptRepo();
      return resp;
   }

   // void setJobFinishedCallback(std::function<void(const std::string& event, const std::string& data)> cb)
   // {
   //    scheduler->setJobFinishedCallback(cb);
   // }

private:
   std::unique_ptr<JobScheduler> scheduler{ nullptr };
   Repl m_repl;
};
} // namespace opctest

void setupLogger(const std::string& workingDir)
{
   std::vector<spdlog::sink_ptr> sinks;
   sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
   sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_st>(workingDir + "/logs/TestService.log", 1048576 * 5, 10));
   auto combined_logger = std::make_shared<spdlog::logger>("TestService", begin(sinks), end(sinks));
   // register it if you need to access it globally
   spdlog::register_logger(combined_logger);
}

template <class>
inline constexpr bool always_false_v = false;

bool apiCallback(opctest::TestService& service, const opctest::api::RequestVariant& req, opctest::api::ResponseVariant& resp)
{
   std::visit(
   [&](auto&& arg) {
      using T = std::decay_t<decltype(arg)>;
      if constexpr (std::is_same_v<T, opctest::api::GetJobRequest>)
      {
         resp = service.getFinishedJob(arg);
      }
      else if constexpr (std::is_same_v<T, opctest::api::GetJobsRequest>)
      {
         resp = service.getJobs(arg);
      }
      else if constexpr (std::is_same_v<T, opctest::api::NewJobRequest>)
      {
         resp = service.newJob(arg);
      }
      else if constexpr (std::is_same_v<T, opctest::api::GetScriptsRequest>)
      {
         resp = service.getScripts();
      }
      else if constexpr (std::is_same_v<T, opctest::api::GetScriptRequest>)
      {
         resp = service.getScript(arg);
      }
      else if constexpr (std::is_same_v<T, opctest::api::UpdateScriptRequest>)
      {
         resp = service.updateScript(arg);
      }
      else if constexpr (std::is_same_v<T, opctest::api::GetJobLogRequest>)
      {
         resp = service.getJobLog(arg);
      }
      else if constexpr (std::is_same_v<T, opctest::api::GetTargetsRequest>)
      {
         resp = service.getTargets();
      }
      else if constexpr (std::is_same_v<T, opctest::api::NewLineReplRequest>)
      {
         resp = service.newLineRepl(arg);
      }
      else if constexpr (std::is_same_v<T, opctest::api::CloneScriptRepoRequest>)
      {
         resp = service.cloneScriptRepo();
      }
      else
      {
         static_assert(always_false_v<T>, "non-exhaustive visitor!");
      }
   },
   req);

   return true;
}

int main(int argc, char* argv[])
{
   std::string binaryPath = argv[0];
   auto pos = binaryPath.find_last_of('/');
   binaryPath.erase(pos);
   setupLogger(binaryPath);

   //get script repo url
   std::string scriptRepoUrl {"https://github.com/matkonnerth/opcuaTestToolScripts.git"};
   if (argc == 2)
   {
      scriptRepoUrl = argv[1];
   }
   auto logger = spdlog::get("TestService");
   logger->info("using git repo url {}", scriptRepoUrl);

   // handle signals in a dedicated thread
   sigset_t sigset;
   sigemptyset(&sigset);
   sigaddset(&sigset, SIGCHLD);
   pthread_sigmask(SIG_BLOCK, &sigset, nullptr);

   opctest::api::Server server{ "0.0.0.0", 9888, binaryPath + "/dist/testToolApp" };

   opctest::TestService service(binaryPath, scriptRepoUrl, server.getEventCallback());


   auto signalHandler = [&service, &sigset]() {
      while (true)
      {
         int signum = 0;
         // wait until a signal is delivered:
         sigwait(&sigset, &signum);
         service.handleSigChld(signum);
      }
   };

   auto ft_signal_handler = std::async(std::launch::async, signalHandler);

   auto cb = [&](const opctest::api::RequestVariant& req, opctest::api::ResponseVariant& resp) { return apiCallback(service, req, resp); };

   server.setCallback(cb);
   server.listen();
}
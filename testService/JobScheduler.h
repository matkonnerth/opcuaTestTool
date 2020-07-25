#pragma once
#include <mutex>
#include <pistache/http.h>
#include <string>
#include <unordered_map>
#include <vector>
#include "DataBase.h"
using namespace Pistache;

class JobScheduler
{
public:
   JobScheduler(const std::string& workingDir, int maxConcurrentJobs = 4);
   int create(const std::string& jsonString);
   void jobFinished(int pid);
   void getFinishedJobs(Http::ResponseStream& stream);
   std::string getFinishedJob(int jobId);

private:
   int maxConcurrentJobs{ 4 };   
   std::mutex _m;
   const std::string workingDir;
   std::unordered_map<int, int> activeJobs{};
   std::vector<int> finishedPids{};
   std::unique_ptr<DataBase> db {nullptr};
};

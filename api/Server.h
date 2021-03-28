#pragma once
#include "Request.h"
#include <httplib.h>
#include <iostream>
#include <unordered_map>


namespace opctest::api {
class Server
{
public:
   Server(std::string ip, int port)
   : ip{ std::move(ip) }
   , port{ port }
   {
      // getJobs
      srv.Get("/jobs", [&](const httplib::Request& httpReq, httplib::Response& httpRes) {
         httpRes.set_header("Access-Control-Allow-Origin", "*");

         GetJobsRequest req{};
         if(httpReq.has_param("from"))
         {
            req.from = stoi(httpReq.get_param_value("from"));
         }
         if (httpReq.has_param("max"))
         {
            req.max = stoi(httpReq.get_param_value("max"));
         }

         RequestVariant varReq{ req };
         ResponseVariant varResp{};
         callback(varReq, varResp);

         using nlohmann::json;
         json jResp = json::object();
         jResp["ok"] = std::get<GetJobResponse>(varResp).ok;
         jResp["data"] = json::parse(std::get<GetJobResponse>(varResp).data);
         httpRes.set_content(jResp.dump(), "application/json");
      });

      // newJob
      srv.Post("/jobs", [&](const httplib::Request& httpReq, httplib::Response& httpRes) {
         httpRes.set_header("Access-Control-Allow-Origin", "*");

         NewJobRequest req{};
         req.jsonRequest = httpReq.body;

         RequestVariant varReq{ req };
         ResponseVariant varResp{};
         callback(varReq, varResp);

         using nlohmann::json;
         json jResp = std::get<NewJobResponse>(varResp);
         httpRes.set_content(jResp.dump(), "application/json");
      });

      // get finished job
      srv.Get(R"(/jobs/(\d+))", [&](const httplib::Request& httpReq, httplib::Response& httpRes) {
         httpRes.set_header("Access-Control-Allow-Origin", "*");

         GetJobRequest req{};
         req.id = stoi(httpReq.matches[1]);

         RequestVariant varReq{ req };
         ResponseVariant varResp{};
         callback(varReq, varResp);

         using nlohmann::json;
         json jResp = json::object();
         jResp["ok"] = std::get<GetJobResponse>(varResp).ok;
         jResp["data"] = json::parse(std::get<GetJobResponse>(varResp).data);
         httpRes.set_content(jResp.dump(), "application/json");
      });
   }

   void listen()
   {
      srv.listen(ip.c_str(), port);
   }

   void setCallback(RequestCallback cb)
   {
      callback = cb;
   }

private:
   httplib::Server srv;
   std::string ip{ "0.0.0.0" };
   int port{ 9888 };
   RequestCallback callback{ nullptr };
};
}; // namespace opctest::api
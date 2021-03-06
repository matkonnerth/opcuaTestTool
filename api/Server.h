#pragma once
#include "Request.h"
#include <chrono>
#include <httplib.h>
#include <iostream>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <regex>


namespace opctest::api {

class EventDispatcher
{
public:
   EventDispatcher()
   {}

   void wait_event(httplib::DataSink* sink)
   {
      std::unique_lock<std::mutex> lk(m_);
      int id = id_;
      cv_.wait(lk, [&] { return cid_ == id; });
      if (sink->is_writable())
      {
         sink->write(message_.data(), message_.size());
      }
   }

   void send_event(const std::string& message)
   {
      std::lock_guard<std::mutex> lk(m_);
      message_ = "data: " + message + "\n\n";
      cid_ = id_++;
      cv_.notify_all();
   }

private:
   std::mutex m_;
   std::condition_variable cv_;
   std::atomic_int id_{ 0 };
   std::atomic_int cid_{ -1 };
   std::string message_;
};
class Server
{
public:
   Server(std::string ip, int port, const std::string& pathToWebApp)
   : m_ip{ std::move(ip) }
   , m_port{ port }
   {
      srv.Options("/(.*)", [&](const httplib::Request& /*req*/, httplib::Response& res) {
         res.set_header("Access-Control-Allow-Methods", "POST, GET, PUT, OPTIONS");
         res.set_header("Content-Type", "text/html; charset=utf-8");
         res.set_header("Access-Control-Allow-Headers", "X-Requested-With, Content-Type, Accept");
         res.set_header("Access-Control-Allow-Origin", "*");
         res.set_header("Connection", "close");
      });

      auto ret = srv.set_mount_point("/", pathToWebApp.c_str());
      if (!ret)
      {
         std::cout << "cannot mount webapp " << pathToWebApp << "\n";
      }
      // getJobs
      srv.Get("/api/jobs", [&](const httplib::Request& httpReq, httplib::Response& httpRes) {
         httpRes.set_header("Access-Control-Allow-Origin", "*");

         GetJobsRequest req{};
         if (httpReq.has_param("from"))
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
         jResp["response"] = json::parse(std::get<GetJobResponse>(varResp).data);
         httpRes.set_content(jResp.dump(), "application/json");
      });

      srv.Get("/api/scripts", [&](const httplib::Request& /*httpReq*/, httplib::Response& httpRes) {
         httpRes.set_header("Access-Control-Allow-Origin", "*");

         GetScriptsRequest req{};
         RequestVariant varReq{ req };
         ResponseVariant varResp{};
         callback(varReq, varResp);

         using nlohmann::json;
         json jResp = json::object();
         jResp["ok"] = std::get<GetScriptsResponse>(varResp).ok;
         jResp["response"] = json::parse(std::get<GetScriptsResponse>(varResp).data);
         httpRes.set_content(jResp.dump(), "application/json");
      });

      srv.Put("/api/repo/clone", [&](const httplib::Request& /*httpReq*/, httplib::Response& httpRes) {
         httpRes.set_header("Access-Control-Allow-Origin", "*");

         CloneScriptRepoRequest req{};
         RequestVariant varReq{ req };
         ResponseVariant varResp{};
         callback(varReq, varResp);

         using nlohmann::json;
         json jResp = json::object();
         jResp["ok"] = std::get<CloneScriptRepoResponse>(varResp).ok;
         httpRes.set_content(jResp.dump(), "application/json");
      });


      srv.Get(R"(/api/scripts/(.*))", [&](const httplib::Request& httpReq, httplib::Response& httpRes) {
         httpRes.set_header("Access-Control-Allow-Origin", "*");

         GetScriptRequest req{};
         req.name = httpReq.matches[1];
         RequestVariant varReq{ req };
         ResponseVariant varResp{};
         callback(varReq, varResp);
         httpRes.set_content(std::get<GetScriptResponse>(varResp).content, "text/plain");
      });

      srv.Post(R"(/api/scripts/(.*))", [&](const httplib::Request& httpReq, httplib::Response& httpRes) {
         httpRes.set_header("Access-Control-Allow-Origin", "*");

         UpdateScriptRequest req{};
         req.name = httpReq.matches[1];
         req.content = httpReq.body;
         RequestVariant varReq{ req };
         ResponseVariant varResp{};
         callback(varReq, varResp);
         using nlohmann::json;
         json jResp = json::object();
         jResp["ok"] = std::get<GetScriptsResponse>(varResp).ok;
         httpRes.set_content(jResp.dump(), "application/json");
      });

      // newJob
      srv.Post("/api/jobs", [&](const httplib::Request& httpReq, httplib::Response& httpRes) {
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
      srv.Get(R"(/api/jobs/(\d+))", [&](const httplib::Request& httpReq, httplib::Response& httpRes) {
         httpRes.set_header("Access-Control-Allow-Origin", "*");

         GetJobRequest req{};
         req.id = stoi(httpReq.matches[1]);

         RequestVariant varReq{ req };
         ResponseVariant varResp{};
         callback(varReq, varResp);

         using nlohmann::json;
         json jResp = json::object();
         jResp["statusCode"] = std::get<GetJobResponse>(varResp).ok;
         jResp["response"] = json::parse(std::get<GetJobResponse>(varResp).data);
         httpRes.set_content(jResp.dump(), "application/json");
      });

      // get finished job
      srv.Get(R"(/api/jobs/(\d+)/log)", [&](const httplib::Request& httpReq, httplib::Response& httpRes) {
         httpRes.set_header("Access-Control-Allow-Origin", "*");

         GetJobLogRequest req{};
         req.id = stoi(httpReq.matches[1]);

         RequestVariant varReq{ req };
         ResponseVariant varResp{};
         callback(varReq, varResp);

         httpRes.set_content(std::get<GetJobLogResponse>(varResp).data, "text/plain");
      });

      srv.Get("/api/events", [&](const httplib::Request& /*req*/, httplib::Response& res) {
         res.set_header("Access-Control-Allow-Origin", "*");
         res.set_chunked_content_provider("text/event-stream", [&](size_t /*offset*/, httplib::DataSink& sink) {
            ed.wait_event(&sink);
            return true;
         });
      });

      srv.Get("/api/targets", [&](const httplib::Request& /*httpReq*/, httplib::Response& httpRes) {
         httpRes.set_header("Access-Control-Allow-Origin", "*");

         GetTargetsRequest req{};
         RequestVariant varReq{ req };
         ResponseVariant varResp{};
         callback(varReq, varResp);
         httpRes.set_content(std::get<GetTargetsResponse>(varResp).data, "application/json");
      });

      //repl - new line
      srv.Post(R"(/api/repl)", [&](const httplib::Request& httpReq, httplib::Response& httpRes) {
         httpRes.set_header("Access-Control-Allow-Origin", "*");

         NewLineReplRequest req{};
         req.content = httpReq.body;
         RequestVariant varReq{ req };
         ResponseVariant varResp{};
         callback(varReq, varResp);
         using nlohmann::json;
         json jResp = json::object();
         jResp["ok"] = std::get<Response>(varResp).ok;
         httpRes.set_content(jResp.dump(), "application/json");
      });

      m_fEventCallback = [&](const std::string& name, const std::string& data1) {
         std::regex newlines_re("\n+");
         auto result = std::regex_replace(data1, newlines_re, "\\n");
         std::string message = "{\"event\": \"" + name + "\", \"data\": \"" + result + "\"}";
         ed.send_event(message);
      };
   }

   void listen()
   {
      srv.listen(m_ip.c_str(), m_port);
   }

   void setCallback(RequestCallback cb)
   {
      callback = cb;
   }

   auto getEventCallback()
   {
      return m_fEventCallback;
   }

private:
   httplib::Server srv;
   std::string m_ip{ "0.0.0.0" };
   int m_port{ 9889 };
   RequestCallback callback{ nullptr };
   EventDispatcher ed{};
   std::function<void(const std::string eventName, const std::string data)> m_fEventCallback{ nullptr };
};
} // namespace opctest::api
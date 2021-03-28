#pragma once
#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <variant>

namespace opctest::api {

struct Request
{
   std::string jsonRequest;
};

struct NewJobRequest : public Request
{};

struct GetJobRequest : public Request
{
   int id;
};

struct GetJobsRequest : public Request
{
   int from;
   int max;
};

struct Response
{
   bool ok;
};

struct NewJobResponse : public Response
{
   int id;
};

struct GetJobResponse : public Response
{
   std::string data;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(NewJobResponse, ok, id);

using RequestVariant = std::variant<NewJobRequest, GetJobRequest, GetJobsRequest>;
using ResponseVariant = std::variant<Response, NewJobResponse, GetJobResponse>;

using RequestCallback = std::function<bool(const RequestVariant&, ResponseVariant&)>;

} // namespace opctest::api
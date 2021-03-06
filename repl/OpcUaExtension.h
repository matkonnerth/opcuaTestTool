#pragma once
#include <memory>
#include <modernopc/client/Client.h>
#include <sol/sol.hpp>

class OpcUaExtension
{
public:
   void add(sol::state_view& lua);


private:
   std::unique_ptr<modernopc::Client> client;
};
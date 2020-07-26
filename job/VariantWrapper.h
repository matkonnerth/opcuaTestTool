#pragma once
#include <open62541/types_generated.h>
#include <string>

namespace tt {

class VariantWrapper
{
public:
   VariantWrapper();
   VariantWrapper(UA_Variant* var)
   : variant{ var }
   {}
   VariantWrapper(const VariantWrapper&) = delete;
   VariantWrapper& operator=(const VariantWrapper&) = delete;
   VariantWrapper(VariantWrapper&& other)
   {
      variant = other.variant;
      other.variant = nullptr;
   }
   ~VariantWrapper();
   bool isEqual(const std::string& jsonVariant) const;
   UA_Variant* getVariant()
   {
      return variant;
   }

private:
   UA_Variant* variant;
};
} // namespace tt
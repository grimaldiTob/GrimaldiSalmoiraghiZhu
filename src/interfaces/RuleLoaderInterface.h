#pragma once

#include "../../external/simdjson.h"
#include "../types/rules/BaseRule.h"
#include <memory>
#include <string>
#include <vector>

class RuleLoaderInterface {
  public:
    virtual ~RuleLoaderInterface() = default;

    /**
     * Parses a JSON file and loads rules into the provided rules list.
     * @param parser The simdjson parser instance.
     * @param filename The name of the JSON file to parse.
     * @param rules_list The list to populate with parsed rules.
     */
    virtual void
    loadRules(const std::string &filename,
              std::vector<std::shared_ptr<BaseRule>> &rules_list) = 0;
};

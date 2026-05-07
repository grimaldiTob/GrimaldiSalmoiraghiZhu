#pragma once

#include <vector>
#include <string>
#include <memory>
#include "../rules/BaseRule.h"
#include "../../external/simdjson.h"

class RuleLoaderInterface {
public:
    virtual ~RuleLoaderInterface() = default;

    /**
     * Parses a JSON file and loads rules into the provided rules list.
     * @param parser The simdjson parser instance.
     * @param filename The name of the JSON file to parse.
     * @param rules_list The list to populate with parsed rules.
     */
    virtual void loadRules(simdjson::ondemand::parser& parser, const std::string& filename, std::vector<std::shared_ptr<BaseRule>>& rules_list) = 0;
};
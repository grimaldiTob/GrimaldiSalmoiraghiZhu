#pragma once

#include <vector>
#include <string>
#include <memory>
#include "../interfaces/RuleLoaderInterface.h"

class RuleLoader : public RuleLoaderInterface {
public:
    RuleLoader() = default;
    ~RuleLoader() override = default;

    /**
     * Parses a JSON file and loads rules into the provided rules list.
     * @param parser The simdjson parser instance.
     * @param filename The name of the JSON file to parse.
     * @param rules_list The list to populate with parsed rules.
     */
    void loadRules(simdjson::ondemand::parser& parser, const std::string& filename, std::vector<std::shared_ptr<BaseRule>>& rules_list) override;
private: 
    RulePriority parsePriority(std::string_view prio_str);
};


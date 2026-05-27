#pragma once

#include "../interfaces/RuleLoaderInterface.h"
#include <memory>
#include <string>
#include <vector>

class RuleLoader : public RuleLoaderInterface {
  public:
    RuleLoader() = default;
    ~RuleLoader() override = default;

    void loadRules(const std::string &filename,
                   std::vector<std::shared_ptr<BaseRule>> &rules_list) override;

  private:
    RulePriority parsePriority(std::string_view prio_str);

    // ok now I defined it this way but thing about having the rules_list as a
    // RuleLoader attribute. I'm not sure if it is right or not since the
    // RuleLoader is just loading rules (and not storing) but might be usefull
    // to consider this.
    void sortRules(std::vector<std::shared_ptr<BaseRule>> &rules_list);

    // Subroutinf for each ruel type
    std::shared_ptr<BaseRule> parseSimpleRule(simdjson::ondemand::object &obj);
    std::shared_ptr<BaseRule>
    parseStepDifferenceRule(simdjson::ondemand::object &obj);
    std::shared_ptr<BaseRule>
    parseStatefulRule(simdjson::ondemand::object &obj);
    std::shared_ptr<BaseRule> parseLogicalCorrelationRule(
        simdjson::ondemand::object &obj,
        const std::vector<std::shared_ptr<BaseRule>> &rules_list);

    simdjson::ondemand::parser m_parser;
};

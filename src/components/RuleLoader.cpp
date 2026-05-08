#include <fstream>
#include <iostream>

#include "RuleLoader.h"
#include "../rules/SimpleRule.h"
#include "../rules/StepDifferenceRule.h"
#include "../rules/StatefulRule.h"
#include "../rules/LogicalCorrelationRule.h"

/** @brief Helper method which maps priority strings to the enum values.*/
RulePriority RuleLoader::parsePriority(std::string_view prio_str) {
    if (prio_str == "HIGH") return RulePriority::HIGH;
    if (prio_str == "MEDIUM") return RulePriority::MEDIUM;
    return RulePriority::LOW;
}

/** @brief Take the rules_list passed as an argument and sorts the value given 
 * the priority attribute of the rule.
 */
void RuleLoader::sortRules(std::vector<std::shared_ptr<BaseRule>>& rules_list) {
    std::sort(rules_list.begin(), rules_list.end(),
        [](const auto& a, const auto& b) {
            return a->getPriority() < b->getPriority();
        });
}

/** @brief parses a JSON file passed as argument. 
 * It extracts the values from the json and creates a Rule object
 * for each rule in the file. It then adds the created rule to the rules list passed as argument.
 * TODO: Is the rule vector concept fine? Rules are statically loaded at the start of the program, 
 * so we can just have a vector of rules.
 * TODO: Should we split this function into multiple subroutins (one for each rule type)? 
 * This way we ensure higher modularity and readibility. Moreover, the addition of a new rule type 
 * would just require to add a new helper function.  
 */
void RuleLoader::loadRules(simdjson::ondemand::parser& parser, const std::string& filename, std::vector<std::shared_ptr<BaseRule>>& rules_list) {
    
    simdjson::padded_string json;
    // .get() method assigns the value to the argument passed to the function 
    if(simdjson::padded_string::load(filename).get(json)) {
        std::cerr << "Cannot load the file. Check the filename." << std::endl;
        return; // skip
    }
    
    simdjson::ondemand::document doc;
    parser.iterate(json).get(doc); // parser.iterate allows to read the json string and parse the json object into the doc 
    
    for(simdjson::ondemand::object obj : doc.get_array()) {
        std::shared_ptr<BaseRule> current_rule;

        // Extract mandatory fields for all rules
        std::string_view rule_id_sv = obj["rule_id"].get_string();
        std::string_view rule_type_sv = obj["type"].get_string();

        std::string rule_id(rule_id_sv);
        std::string rule_type(rule_type_sv);

        if (rule_type == "simple" || rule_type == "step_difference") {
            std::string_view sensor_id_sv = obj["sensor_id"].get_string();
            std::string sensor_id(sensor_id_sv);

            std::string_view oprtor_sv = obj["operator"].get_string();
            std::string oprtor(oprtor_sv);
            double value = obj["value"].get_double();

            // priority gets parsed LAST --> simdjson parses reading the order the content of the .json file
            std::string_view priority_sv = obj["priority"].get_string();
            RulePriority priority = parsePriority(priority_sv);

            if (rule_type == "simple") {
                current_rule = std::make_shared<SimpleRule>(rule_id, priority, sensor_id, oprtor, value);
            } else {
                current_rule = std::make_shared<StepDifferenceRule>(rule_id, priority, sensor_id, oprtor, value);
            }
        } else if (rule_type == "stateful") {
            std::string_view sensor_id_sv = obj["sensor_id"].get_string();
            std::string sensor_id(sensor_id_sv);

            std::string_view oprtor_sv = obj["operator"].get_string();
            std::string oprtor(oprtor_sv);
            double value = obj["value"].get_double();

            double consecutive_meas = obj["consecutive_measurements"].get_double();

            std::string_view priority_sv = obj["priority"].get_string();
            RulePriority priority = parsePriority(priority_sv);
            current_rule = std::make_shared<StatefulRule>(rule_id, priority, sensor_id, oprtor, consecutive_meas, value);
        } else if (rule_type == "correlation") {
            std::string_view logic_sv = obj["logic"].get_string();
            std::string logic(logic_sv);

            std::vector<std::shared_ptr<BaseRule>> corr_rules;
            std::vector<std::string> condition_ids;
            for (auto id_val : obj["conditions"].get_array()) {
                condition_ids.emplace_back(id_val.get_string());
            }
            // this approach is not working if the correlation rules gets parsed BEFORE the simple rule.
            for (const std::string& target_id : condition_ids) {
                for (const auto& rule : rules_list) {
                    if (rule->getRuleId() == target_id) {
                        corr_rules.push_back(rule);
                        break;
                    }
                }
            }

            std::string_view priority_sv = obj["priority"].get_string();
            RulePriority priority = parsePriority(priority_sv);
            current_rule = std::make_shared<LogicalCorrelationRule>(rule_id, priority, logic, corr_rules);
        }

        if (current_rule) {
            rules_list.emplace_back(current_rule);
        }
    }
    sortRules(rules_list);
}

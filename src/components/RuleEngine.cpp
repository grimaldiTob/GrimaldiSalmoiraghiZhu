#include "RuleEngine.h"
#include <fstream>

// constructor
RuleEngine::RuleEngine(std::shared_ptr<BatchProviderInterface> provider) {}

RulePriority RuleEngine::parsePriority(std::string_view prio_str) {
    if (prio_str == "HIGH") return RulePriority::HIGH;
    if (prio_str == "MEDIUM") return RulePriority::MEDIUM;
    return RulePriority::LOW;
}

/** @brief parses a JSON file passed as argument. 
 * It extracts the values from the json and creates a Rule object (class to implement)
 * for each rule in the file.
 */
void RuleEngine::ruleParsing(simdjson::ondemand::parser& parser, const std::string& filename) {
    std::ifstream infile(filename); // just one file at a time for now
    
    // uncomment this when the Rule class is ready
    std::vector<std::shared_ptr<BaseRule>> parsed_rules;

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
        
        // define mandary fields for ALL TYPE OF RULES
        std::string_view rule_id_sv = obj["rule_id"].get_string();
        std::string_view rule_type_sv = obj["type"].get_string();

        std::string rule_id(rule_id_sv);
        std::string rule_type(rule_type_sv);

        // I dont know if this check should be performed using enums. Since we call this function just at start 
        // we can consider the string in the json file and check on that 
        if(rule_type == "simple" || rule_type == "step_difference") {
            // set values 
            std::string_view sensor_id_sv = obj["sensor_id"].get_string();
            std::string sensor_id(sensor_id_sv);

            std::string_view oprtor_sv = obj["operator"].get_string();
            std::string oprtor(oprtor_sv);
            double value = obj["value"].get_double();

            // priority gets parsed LAST --> simdjson parses reading the order the content of the .json file
            std::string_view priority_sv = obj["priority"].get_string();
            RulePriority priority = parsePriority(priority_sv);

            if(rule_type == "simple"){
                current_rule = std::make_shared<SimpleRule>(
                        rule_id, priority, sensor_id, oprtor, value
                );
            } else {
                current_rule = std::make_shared<StepDifferenceRule>(
                        rule_id, priority, sensor_id, oprtor, value
                );
            }
        }
    }
}

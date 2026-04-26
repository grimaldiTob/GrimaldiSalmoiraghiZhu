#include "RuleEngine.h"
#include <fstream>

// constructor
RuleEngine::RuleEngine(std::shared_ptr<BatchProviderInterface> provider) {}

/** @brief parses a JSON file passed as argument. 
 * It extracts the values from the json and creates a Rule object (class to implement)
 * for each rule in the file.
 */
void RuleEngine::ruleParsing(simdjson::ondemand::parser& parser, const std::string& filename) {
    std::ifstream infile(filename); // just one file at a time for now
    
    // uncomment this when the Rule class is ready
    // std::vector<Rule> rule_array;

    simdjson::padded_string json;
    // .get() method assigns the value to the argument passed to the function 
    if(simdjson::padded_string::load(filename).get(json)) {
        std::cerr << "Cannot load the file. Check the filename." << std::endl;
        return; // skip
    }

    simdjson::ondemand::document doc;
    parser.iterate(json).get(doc); // parser.iterate allows to read the json string and parse the json object into the doc 

    /*
    Commento tutto sto blocco di codice tanto non aveva molto senso implementare il parsing senza avere la classe Rule su cui basarsi
    Ad ogni modo questo parsing è più complicato visto che alcune regole hanno dei campi e altre no.
    La logica è più o meno questa ma ho poco tempo.

    doc can be iterated as an array
    for(simdjson::ondemand::object obj : doc.get_array()) {
        // Rule rule; when the rule is defined uncomment this
        
        // define mandary fields for ALL TYPE OF RULES
        std::string_view rule_id;
        std::string_view rule_type;
        std::string_view priority;

        // mandatory fields... when classes will be defined we can modify this code 
        std::string_view sensor_id;
        std::string_view oprtor;

        // for correlated rules
        std::string_view logic;
        std::vector<std::string_view> conditions;
        obj["rule_id"].get(rule_id);
        obj["type"].get(rule_type);
        obj["priority"].get(priority);

        if(std::string(rule_type) == "correlation"){
            obj["logic"].get(logic);

            simdjson::ondemand::array conditions_array;
            if (!obj["conditions"].get(conditions_array)) {
                for (std::string_view cond : conditions_array) {
                    conditions.emplace_back(std::string(cond));
                }
            }
        } else if()
    }
        */
}

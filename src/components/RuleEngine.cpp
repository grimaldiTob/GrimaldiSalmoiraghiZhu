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
    std::string line;

    // get_line takes a ifstream in input and a string and stores the content in the string until it finds the "\n" char 
    while(std::getline(infile, line)) {
        if(line.empty()) continue;

        simdjson::padded_string padded_line(line); // create a new padded line
        simdjson::ondemand::document doc;

        auto error = parser.iterate(padded_line).get(doc); // .get() method assigns the value to the argument passed to the function 
        if(error) {
            std::cerr << "Impossible JSON packet syntax" << std::endl;
            continue; // skip to the next line here
        }

        break; // implementation incomplete
    }
}

#include "RuleEngine.h"
#include <fstream>

/** @brief 
 * Reset the content of the cache erasing just the values not the key.
*/
void RuleEngine::resetCache() {
    // ho trovato questa funzione ed è troppo in stile Formaggia
    std::for_each(rules_cache.begin(), rules_cache.end(),
                  [](std::pair<std::string, std::optional<bool>>& p){
                      p.second = std::nullopt;
                  });
}

/** @brief Method which evaluates all the rules stored in the rules_list
 * For each rule checks if there is a match between the measurement in the
 * batch and evaluates the rule
 */
void RuleEngine::evaluateRules() {
    for(auto& rule : rules_list) {

        rule->evaluate(batch, rules_cache);

        // hypothetically we could think of storing result in cache here 
        // instead of the evaluate rule
        
        // also thinking about parallelization of rule evaluation
        // we cannot really parallelize the whole rules_list for two reasons:
            // 1. the cache becomes useless
            // 2. (and most important) WE HAVE PRIORITIES
        
        // so we can think of like parallelizing over 'HIGH', 'MEDIUM', 'LOW' priorities but not all of them.
    }
    accumulator.storeResultHistory(); // store the values of the accumulator in the history map
    resetCache(); // reset rules cache 
}
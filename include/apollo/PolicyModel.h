#ifndef APOLLO_POLICY_MODEL_H
#define APOLLO_POLICY_MODEL_H

#include <string>
#include <vector>

// Abstract
class PolicyModel {
    public:
        PolicyModel(int num_policies, std::string name, bool training) : 
            policy_count(num_policies),
            name(name),
            training(training)
        {};
        virtual ~PolicyModel() {}
        //
        virtual int      getIndex(std::vector<float> &features) = 0;

        virtual void    store(const std::string &filename) = 0;

        int          policy_count;
        std::string      name           = "";
        bool             training       = false;
}; //end: PolicyModel (abstract class)


#endif

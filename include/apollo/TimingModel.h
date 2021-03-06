#ifndef APOLLO_TIMING_MODEL_H
#define APOLLO_TIMING_MODEL_H

#include <string>
#include <vector>

// Abstract
class TimingModel {
    public:
        TimingModel(std::string name) : name(name) {};
        virtual ~TimingModel() {}
        virtual double getTimePrediction(std::vector<float> &features) = 0;
        virtual void store(const std::string &filename) = 0;

        std::string      name           = "";
}; //end: TimingModel (abstract class)


#endif

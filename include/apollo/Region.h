
#ifndef APOLLO_REGION_H
#define APOLLO_REGION_H

#include <vector>
#include <unordered_map>

#include "apollo/Apollo.h"
#include "apollo/ModelWrapper.h"
#include "apollo/Explorable.h"

class Apollo::Region {
    public:
        Region( Apollo       *apollo,
                const char   *regionName,
                int           numAvailablePolicies);
        ~Region();

        typedef struct {
            std::string  name;
            double      *target_var;
            double       start;
            double       step;
            double       stop;
        } Explorable;

        typedef struct {
            int       exec_count;
            double    time_total;
        } Measure;


        char    *name;
        void     begin(void);
        void     end(void);

        bool     is_timed;
        int      minimum_elements_to_evaluate_model; //deprecated

        Apollo::ModelWrapper *getModel(void);
        int                   getPolicyIndex(void);

        std::unordered_map<std::vector<Apollo::Feature>, Apollo::Region::Measure *>
            measures;

        std::vector<Apollo::Explorable> explorables;

        int            current_policy;

        void           flushMeasurements(int assign_to_step);

    private:
        //
        Apollo        *apollo;
        bool           currently_inside_region;
        //
        Apollo::ModelWrapper  *model;
        //
        double         current_step_time_begin;
        double         current_step_time_end;
        //
        uint64_t       id;
        uint64_t       parent_id;
        char           CURRENT_BINDING_GUID[256];
        //
        //
}; //end: Apollo::Region


#endif


#include <memory>
#include <mutex>
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>

#include "external/nlohmann/json.hpp"
using json = nlohmann::json;

#include "apollo/Apollo.h"
#include "apollo/ModelWrapper.h"
//
#include "apollo/models/Random.h"
#include "apollo/models/Sequential.h"
#include "apollo/models/DecisionTree.h"
#include "apollo/models/Python.h"

using namespace std;

bool
Apollo::ModelWrapper::configure(
        const char           *model_def)
{
    Apollo::Model::Type MT, model_type;

    if (model_def == NULL) {
        model_type = MT.Default;
        apollo_log(2, "Using the default model for initialization.\n");
    } else {
        // Extract the various common elements from the model definition
        // and provide them to the configure method, independent of whatever
        // further definitions are unique to that model.
        json j = model_def;

        int            m_type_idx;
        string         m_type_name;
        vector<string> m_loop_names;
        int            m_feat_count;
        vector<string> m_feat_names;
        string         m_drv_format;
        string         m_drv_rules;

        // Validate and extract model components:
        int model_errors = 0;
        if (j.find("type") == j.end()) {
            apollo_log(1, "Invalid model_def: missing [type]\n");
            model_errors++;
        } else {
            if (j["type"].find("index") == j["type"].end()) {
                apollo_log(1, "Invalid model_def: missing [type][index]\n");
                model_errors++;
            } else {
                m_type_idx = j["type"]["index"].get<int>();
            }
            if (j["type"].find("name") == j["type"].end()) {
                apollo_log(1, "Invalid model_def: missing [type][name]\n");
                model_errors++;
            } else {
                m_type_name = j["type"]["name"].get<string>();
            }
        }
        if (j.find("loop_names") == j.end()) {
            apollo_log(1, "Invalid model_def: missing [loop_names]\n");
            model_errors++;
        } else {
            m_loop_names = j["loop_names"].get<vector<string>>();
        }
        if (j.find("features") == j.end()) {
            apollo_log(1, "Invalid model_def: missing [features]\n");
            model_errors++;
        } else {
            if (j["features"].find("count") == j["features"].end()) {
                apollo_log(1, "Invalid model_def: missing [features][count]\n");
                model_errors++;
            } else {
                m_feat_count = j["features"]["count"].get<int>();
            }
            if (j["features"].find("names") == j["features"].end()) {
                apollo_log(1, "Invalid model_def: missing [features][names]\n");
                model_errors++;
            } else {
                m_feat_names = j["features"]["names"].get<vector<string>>();
            }
        }
        if (j.find("driver") == j.end()) {
            apollo_log(1, "Invalid model_def: missing [driver]\n");
            model_errors++;
        } else {
            if (j["driver"].find("format") == j["driver"].end()) {
                apollo_log(1, "Invalid model_def: missing [driver][format]\n");
                model_errors++;
            } else {
                m_drv_format = j["driver"]["format"].get<string>();
            }
            if (j["driver"].find("rules") == j["driver"].end()) {
                apollo_log(1, "Invalid model_def: missing [driver][rules]\n");
                model_errors++;
            } else {
                m_drv_rules = j["driver"]["rules"].get<string>();
            }
        }

        if (model_errors > 0) {
            fprintf(stderr, "ERROR: There were %d errors parsing"
                    " the supplied model definition.\n", model_errors);
            exit(1);
        }

        // TODO: Use the new variables above.
        cout << "m_type_idx     == " << m_type_idx << endl;
        cout << "m_type_name    == " << m_type_name << endl;
        cout << "m_loop_names   == ";
        for (auto i : m_loop_names) {
            cout << "                 " << i << endl;
        };
        cout << "m_feat_count   == " << m_feat_count << endl;
        cout << "m_feat_names   == ";
        for (auto i : m_feat_names) {
            cout << "                 " << i << endl; };
        cout << "m_drv_format   == " << m_drv_format << endl;
        cout << "m_drv_rules    == " << m_drv_rules << endl;

    }

    if (model_type == MT.Default) { model_type = APOLLO_DEFAULT_MODEL_TYPE; }
    shared_ptr<Apollo::ModelObject> nm = nullptr;

    switch (model_type) {
        //
        case MT.Random:
            nm = make_shared<Apollo::Model::Random>();
            break;
        case MT.Sequential:
            nm = make_shared<Apollo::Model::Sequential>();
            break;
        case MT.DecisionTree:
            nm = make_shared<Apollo::Model::DecisionTree>();
            break;
        case MT.Python:
            nm = make_shared<Apollo::Model::Python>();
            break;
        //
        default:
             fprintf(stderr, "WARNING: Unsupported Apollo::Model::Type"
                     " specified to Apollo::ModelWrapper::configure."
                     " Doing nothing.\n");
             return false;
    }

    Apollo::ModelObject *lnm = nm.get();


    lnm->configure(apollo, num_policies, model_def);

    model_sptr.reset(); // Release ownership of the prior model's shared ptr
    model_sptr = nm;    // Make this new model available for use.

    return true;
}


Apollo::ModelWrapper::ModelWrapper(
        Apollo      *apollo_ptr,
        int          numPolicies)
{
    apollo = apollo_ptr;
    num_policies = numPolicies;

    model_sptr = nullptr;

    return;
}

// NOTE: This is the method that RAJA loops call, they don't
//       directly call the model's getIndex() method.
int
Apollo::ModelWrapper::requestPolicyIndex(void) {
    // Claim shared ownership of the current model object.
    // NOTE: This is useful in case Apollo replaces the model with
    //       something else while we're in this [model's] method. The model
    //       we're picking up here will not be destroyed until
    //       this (and all other co-owners) are done with it,
    //       though Apollo is not prevented from setting up
    //       a new model that other threads will be getting, all
    //       without global mutex synchronization.
    shared_ptr<Apollo::ModelObject> lm_sptr = model_sptr;
    Apollo::ModelObject *model = lm_sptr.get();

    static int err_count = 0;
    if (model == nullptr) {
        err_count++;
        lm_sptr.reset();
        if (err_count < 10) {
            fprintf(stderr, "WARNING: requestPolicyIndex() called before model"
                    " has been loaded. Returning index 0 (default).\n");
            return 0;
        } else if (err_count == 10) {
            fprintf(stderr, "WARNING: requestPolicyIndex() called before model"
                    " has been loaded. Returning index 0 (default) and suppressing"
                    " additional identical error messages.\n");
            return 0;
        }
        return 0;
    }


    // Actually call the model now:
    int choice = model->getIndex();

    return choice;
}

Apollo::ModelWrapper::~ModelWrapper() {
    id = "";
    model_sptr.reset(); // Release access to the shared object.
}


// // NOTE: This is deprecated in favor all "model processing engines"
// //       being built directly into the libapollo.so
//
// #include <dlfcn.h>
// #include <string.h>
//
// bool
// Apollo::ModelWrapper::loadModel(const char *path, const char *definition) {
//     // Grab the object lock so if we're changing models
//     // in the middle of a run, the client wont segfault
//     // attempting to region->requestPolicyIndex() at the
//     // head of a loop.
//     lock_guard<mutex> lock(object_lock);
// 
//     if (object_loaded) {
//         // TODO: Clean up after prior model.
//     }
//    
//     object_loaded = false;
// 
//     // Clear any prior errors.
//     char *error_msg = NULL;
//     dlerror();
//     
//     // Load the shared object:
//     void *handle = dlopen(path, (RTLD_LAZY | RTLD_GLOBAL));
// 
//     if ((error_msg = dlerror()) != NULL) {
//         fprintf(stderr, "APOLLO: dlopen(%s, ...): %s\n", path, error_msg);
//         return false;
//     }
// 
//     // Bind to the initialization hooks:
//     create = (Apollo::Model* (*)()) dlsym(handle, "create_instance");
//     if ((error_msg = dlerror()) != NULL) {
//         fprintf(stderr, "APOLLO: dlsym(handle, \"create_instance\") in"
//                 " shared object %s failed: %s\n", path, error_msg);
//         return false;
//     }
//     
//     destroy = (void (*)(Apollo::Model*)) dlsym(handle, "destroy_instance"  );
//     if ((error_msg = dlerror()) != NULL) {
//         fprintf(stderr, "APOLLO: dlsym(handle, \"destroy_instance\") in"
//                 " shared object %s failed: %s\n", path, error_msg);
//         return false;
//     }
// 
//     model = NULL;
//     model = (Apollo::Model*) create();
//     if (model == NULL) {
//         fprintf(stderr, "APOLLO: Could not create an instance of the"
//                 " shared model object %s.\n");
//         return false;
//     }
// 
//     model->configure(apollo, num_policies, definition);
// 
//     object_loaded = true;
//     return object_loaded;
// }
// 



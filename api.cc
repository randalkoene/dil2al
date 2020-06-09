/*
api.cc
Randal A. Koene, 20200528

Provides functions to dil2al/formalizer that create an API through which client calls
can receive or interact with Formalizer data.

See the Formalizer Refactor card https://trello.com/c/rtzKmWA8

For JSON data I/O, this uses the jsoncons library from
https://github.com/danielaparker/jsoncons


*/

#include "dil2al.hh"

#ifdef INCLUDE_API

#include <jsoncons/json.hpp>
//#include <iostream>
//#include <cassert>

using namespace jsoncons; // for convenience

/*
This is really dumb, but it beats having to worry about stack objects expiring
when pointers are returned to data within the object structure...
Instead, you'll probably want to use stack objects and actually COPY the data
you intend to return, since it may be going into a shared memory block anyway.
*/
Detailed_Items_List *persistent_graph = NULL;

/**
 * Load the whole Graph and return a pointer to its data structure.
 * 
 * @return pointer to Graph object, or NULL if unsuccessful.
 */
Detailed_Items_List *get_Graph() {
    if (persistent_graph)
        return persistent_graph;

    persistent_graph = new Detailed_Items_List;

    if (persistent_graph->Get_All_DIL_ID_File_Parameters() < 0) {
        delete persistent_graph;
        return NULL;
    }

    if (!persistent_graph->list.head()) {
        delete persistent_graph;
        return NULL;
    }

    if (persistent_graph->Get_All_Topical_DIL_Parameters(true) < 0) {
        delete persistent_graph;
        return NULL;
    }

    return persistent_graph;
}

/**
 * Find a Node by DIL ID and return a pointer to its data structure.
 * 
 * @param nodeID a valid Node ID (a DILID object).
 * @return pointer to Node data object, or NULL if unsuccessful.
 */
DIL_entry * get_Node(DIL_ID nodeID) {

    if (!nodeID.valid())
        return NULL;
    
    /*
    Wow, this is a super slow approach for getting data of a single Node!
    It always loads the complete Graph, either from cache or from HTML,
    with all description text for all Nodes.
    There is no provision at all for loaded the data of only a specific Node.
    Note that Get_All_DIL_ID_File_Parameters() DOES in fact have an option to
    load data for only a specific Node. A faster implementation is therefore
    probably not so hard to make.
    */
    Detailed_Items_List *graph = get_Graph();
    
    return graph->list.head()->elbyid(nodeID);
}

/**
 * Find a Node by DIL ID and return it in JSON format.
 * 
 * Takes a DIL ID expressed as a String, loads the Graph of Nodes and tries to
 * find a corresponding Node. If found, the Node data is returned in a self-descriptive
 * JSON format.
 * The Node data in this case is the data specific to the Node, not unique to
 * the Edges connecting it with other Nodes.
 * (This is a change from the older interpretation of a DIL_entry and its data.)
 * 
 * @param str_nodeID a string with a valid Node ID (a DILID number).
 * @param json_nodedata receives the JSON formatted Node data.
 * @return true if the Node was found with valid data.
 */
bool get_Node_JSON(String str_nodeID, std::string & json_nodedata) {

    DIL_entry *node = get_Node(DIL_ID(str_nodeID));
    if (!node)
        return false;

    json nodeJSON;

    // Node data from "topics" and "content" (from DIL files)
    nodeJSON["ID"] = str_nodeID.chars();
    if (!node->parameters)
        nodeJSON["topic"] = "undefined";
    else if (!node->parameters->topics.head())
        nodeJSON["topic"] = "undefined";
    else
        nodeJSON["topic"] = node->parameters->topics.head()->dil.file.chars();
    if (node->content) {
        nodeJSON["valuation"] = node->content->valuation; // this is 'desirability' for PLAN_ENTRY_TYPE_OUTCOME
        nodeJSON["completion"] = node->content->completion;
        nodeJSON["required"] = node->content->required;
    } else {
        nodeJSON["valuation"] = "undefined";
        nodeJSON["completion"] = "undefiend";
        nodeJSON["required"] = "undefined";
    }
    if (node->Entry_Text()) {
        nodeJSON["text"] = node->Entry_Text()->chars();
    } else {
        nodeJSON["text"] = "";
    }

    // Node data from edges to Superiors (from DIL-by-ID file)
    #define SUPIDSTR al.title
    DIL_Superiors * edge = node->Projects(0);
    for (; (edge); edge = edge->Next()) {
        if (edge->targetdate() == DILSUPS_TD_UNSPECIFIED)
            nodeJSON["targetdate"] = "unspecified";
        else {
            nodeJSON["targetdate"] = time_stamp("%Y%m%d%H%M", edge->targetdate()).chars();
        }
        bool exacttd = edge->tdexact();
        bool fixedtd = false;
        if (!exacttd) fixedtd = (edge->tdproperty() & DILSUPS_TDPROP_FIXED);
        //bool variabletd = (!exacttd && !fixedtd);
        if (exacttd) nodeJSON["tdproperty"] = "exact";
        else if (fixedtd) nodeJSON["tdproperty"] = "fixed";
        else nodeJSON["tdproperty"] = "variable";
        if (exacttd) {
            bool tdisperiodic = (edge->tdproperty() & DILSUPS_TDPROP_PERIODIC);
            nodeJSON["tdperiodic"] = periodictaskstr[edge->tdperiod()];
            if (tdisperiodic) {
                nodeJSON["tdevery"] = edge->tdevery();
                nodeJSON["tdspan"] = edge->tdspan(); // 0 means unlimited
            }
        }
    }

    nodeJSON.dump(json_nodedata,indenting::indent);
    //cout << pretty_print(nodeJSON) << std::endl;
    return true;
}

/**
 * Command line intereface to JSON API. Request and return a Node in JSON format.
 * 
 * Uses the JSON API to request Node data for the Node with specified ID
 * and to return that data through the INFO stream.
 * 
 * @param str_nodeID a string with a valid Node ID (a DILID number).
 * @return true if the Node was found with valid data.
 */
bool cmd_api_JSON(String str_nodeID) {
    std::string json_nodedata;
    if (!get_Node_JSON(str_nodeID, json_nodedata)) return false;
    INFO << json_nodedata.c_str();
    return true;
}



#endif
// end of INCLUDE_API

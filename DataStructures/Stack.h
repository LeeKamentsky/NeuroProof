#include <vector>
#include <tr1/unordered_map>
#include <tr1/unordered_set>
#include "Rag.h"
#include "../Algorithms/RagAlgs.h"
#include "AffinityPair.h"
#include "../FeatureManager/FeatureManager.h"
#include "Glb.h"
#include <boost/python.hpp>
#include <boost/tuple/tuple.hpp>
#include "../Algorithms/MergePriorityFunction.h"

#ifndef STACK_H
#define STACK_H

namespace NeuroProof {

class Stack {
  public:
    Stack(Label* watershed_, int depth_, int height_, int width_, int padding_=1) : watershed(watershed_), depth(depth_), height(height_), width(width_), padding(padding_), feature_mgr(0), median_mode(false), prediction_array(0)
    {
        rag = new Rag<Label>();
    }

    void build_rag();

    void agglomerate_rag(double threshold);

    void add_prediction_channel(double * prediction_array_)
    {
        prediction_array.push_back(prediction_array_);
    
        if (feature_mgr) {
            feature_mgr->add_channel();
        }
    }

    bool is_orphan(RagNode<Label>* node)
    {
        node_properties_holder = rag->retrieve_property_list("border_node");
        bool border = false;
        try {
            border = property_list_retrieve_template_property<Label, bool>(node_properties_holder, node);
        } catch (ErrMsg& msg) {
            //
        }

        return !border;
    }
        
    Rag<Label>* get_rag()
    {
        return rag;
    }

    void determine_edge_locations()
    {
        best_edge_z.clear();
        best_edge_loc.clear();

        unsigned int plane_size = width * height;

        for (unsigned int z = 1; z < (depth-1); ++z) {
            int z_spot = z * plane_size;

            EdgeCount curr_edge_z;
            EdgeLoc curr_edge_loc;
             

            for (unsigned int y = 1; y < (height-1); ++y) {
                int y_spot = y * width;
                for (unsigned int x = 1; x < (width-1); ++x) {
                    unsigned long long curr_spot = x + y_spot + z_spot;
                    unsigned int spot0 = watershed_to_body[(watershed[curr_spot])];
                    unsigned int spot1 = watershed_to_body[(watershed[curr_spot-1])];
                    unsigned int spot2 = watershed_to_body[(watershed[curr_spot+1])];
                    unsigned int spot3 = watershed_to_body[(watershed[curr_spot-width])];
                    unsigned int spot4 = watershed_to_body[(watershed[curr_spot+width])];
                    unsigned int spot5 = watershed_to_body[(watershed[curr_spot-plane_size])];
                    unsigned int spot6 = watershed_to_body[(watershed[curr_spot+plane_size])];

                    if (spot1 && (spot0 != spot1)) {
                        RagEdge<Label>* edge = rag->find_rag_edge(spot0, spot1);
                        curr_edge_z[edge] += 1;  
                        curr_edge_loc[edge] = Location(x,y,z);  
                    }
                    if (spot2 && (spot0 != spot2)) {
                        RagEdge<Label>* edge = rag->find_rag_edge(spot0, spot2);
                        curr_edge_z[edge] += 1;  
                        curr_edge_loc[edge] = Location(x,y,z);  
                    }
                    if (spot3 && (spot0 != spot3)) {
                        RagEdge<Label>* edge = rag->find_rag_edge(spot0, spot3);
                        curr_edge_z[edge] += 1;  
                        curr_edge_loc[edge] = Location(x,y,z);  
                    }
                    if (spot4 && (spot0 != spot4)) {
                        RagEdge<Label>* edge = rag->find_rag_edge(spot0, spot4);
                        curr_edge_z[edge] += 1;  
                        curr_edge_loc[edge] = Location(x,y,z);  
                    }
                    if (spot5 && (spot0 != spot5)) {
                        RagEdge<Label>* edge = rag->find_rag_edge(spot0, spot5);
                        curr_edge_z[edge] += 1;  
                        curr_edge_loc[edge] = Location(x,y,z);  
                    }
                    if (spot6 && (spot0 != spot6)) {
                        RagEdge<Label>* edge = rag->find_rag_edge(spot0, spot6);
                        curr_edge_z[edge] += 1;  
                        curr_edge_loc[edge] = Location(x,y,z);  
                    }
                }
            }
        
            for (EdgeCount::iterator iter = curr_edge_z.begin(); iter != curr_edge_z.end(); ++iter) {
                if (iter->second > best_edge_z[iter->first]) {
                    best_edge_z[iter->first] = iter->second;
                    best_edge_loc[iter->first] = curr_edge_loc[iter->first];
                }
            }
        } 
        
        
        return;
    }

    double get_edge_weight(RagEdge<Label>* edge)
    {
        return feature_mgr->get_prob(edge);
    }

    boost::python::tuple get_edge_loc(RagEdge<Label>* edge)
    {
        if (best_edge_loc.find(edge) == best_edge_loc.end()) {
            throw ErrMsg("Edge location was not loaded!");
        }
        Location loc = best_edge_loc[edge];
        unsigned int x = boost::get<0>(loc) - padding;
        unsigned int y = boost::get<1>(loc) - padding; //height - boost::get<1>(loc) - 1 - padding;
        unsigned int z = boost::get<2>(loc) - padding;

        return boost::python::make_tuple(x, y, z); 
    }

    Label get_body_id(unsigned int x, unsigned int y, unsigned int z)
    {
        x += padding;
        //y += padding;
        y = height - y - 1 - padding;
        z += padding;
        unsigned int plane_size = width * height;
        unsigned long long curr_spot = x + y * width + z * plane_size; 
        Label body_id = watershed[curr_spot];
        if (!watershed_to_body.empty()) {
            body_id = watershed_to_body[body_id];
        }
        return body_id;
    }

    bool add_edge_constraint(boost::python::tuple loc1, boost::python::tuple loc2)
    {
        Label body1 = get_body_id(boost::python::extract<int>(loc1[0]), boost::python::extract<int>(loc1[1]),
                                boost::python::extract<int>(loc1[2]));    
        Label body2 = get_body_id(boost::python::extract<int>(loc2[0]), boost::python::extract<int>(loc2[1]),
                                boost::python::extract<int>(loc2[2]));    

        if (body1 == body2) {
            return false;
        }

        RagEdge<Label>* edge = rag->find_rag_edge(body1, body2);

        if (!edge) {
            RagNode<Label>* node1 = rag->find_rag_node(body1);
            RagNode<Label>* node2 = rag->find_rag_node(body2);
            edge = rag->insert_rag_edge(node1, node2);
            edge->set_false_edge(true);
        }
        edge->set_preserve(true);

        return true;
    }

    FeatureMgr * get_feature_mgr()
    {
        return feature_mgr;
    }
    void set_feature_mgr(FeatureMgr * feature_mgr_)
    {
        feature_mgr = feature_mgr_;
    }

    Label * get_label_volume()
    {
        Label * temp_labels = new Label[(width-(2*padding))*(height-(2*padding))*(depth-(2*padding))];
        Label * temp_labels_iter = temp_labels;
        unsigned int plane_size = width * height;

        for (int z = padding; z < (depth - padding); ++z) {
            int z_spot = z * plane_size;
            for (int y = padding; y < (height - padding); ++y) {
                int y_spot = y * width;
                for (int x = padding; x < (width - padding); ++x) {
                    unsigned long long curr_spot = x+y_spot+z_spot;
                    Label sv_id = watershed[curr_spot];
                    Label body_id;
                    if (!watershed_to_body.empty()) {
                        body_id = watershed_to_body[sv_id];
                    } else {
                        body_id = sv_id;
                    }
                    *temp_labels_iter = body_id;
                    ++temp_labels_iter;
                }
            }
        }
    
        return temp_labels;
    }

    int get_num_bodies()
    {
        return rag->get_num_regions();        
    }

    int get_width() const
    {
        return width;
    }

    int get_height() const
    {
        return height;
    }

    int get_depth() const
    {
        return depth;
    }

    int remove_inclusions();

    ~Stack()
    {
        delete rag;
        if (!prediction_array.empty()) {
            for (unsigned int i = 0; i < prediction_array.size(); ++i) {
                delete prediction_array[i];
            }
        }

        delete watershed;
        if (feature_mgr) {
            delete feature_mgr;
        }
    }
    struct DFSStack {
        Label previous;
        RagNode<Label>* rag_node;  
        int count;
        int start_pos;
    };

  private:
    void biconnected_dfs(std::vector<DFSStack>& dfs_stack);
    
    Rag<Label> * rag;
    Label* watershed;
    std::vector<double*> prediction_array;
    std::tr1::unordered_map<Label, Label> watershed_to_body; 
    std::tr1::unordered_map<Label, std::vector<Label> > merge_history; 
    typedef boost::tuple<unsigned int, unsigned int, unsigned int> Location;
    typedef std::tr1::unordered_map<RagEdge<Label>*, unsigned long long> EdgeCount; 
    typedef std::tr1::unordered_map<RagEdge<Label>*, Location> EdgeLoc; 
    
    int depth, height, width;
    int padding;

    boost::shared_ptr<PropertyList<Label> > node_properties_holder;
    std::tr1::unordered_set<Label> visited;
    std::tr1::unordered_map<Label, int> node_depth;
    std::tr1::unordered_map<Label, int> low_count;
    std::tr1::unordered_map<Label, Label> prev_id;
    std::vector<std::vector<OrderedPair<Label> > > biconnected_components; 
    
    std::vector<OrderedPair<Label> > stack;
   
    EdgeCount best_edge_z;
    EdgeLoc best_edge_loc;

    FeatureMgr * feature_mgr;
    bool median_mode;
};


void Stack::build_rag()
{
    if (feature_mgr && (feature_mgr->get_num_features() == 0)) {
        feature_mgr->add_median_feature();
        median_mode = true; 
    } 
    
    unsigned int plane_size = width * height;
    std::vector<double> predictions(prediction_array.size(), 0.0);

    for (unsigned int z = 1; z < (depth-1); ++z) {
        int z_spot = z * plane_size;
        for (unsigned int y = 1; y < (height-1); ++y) {
            int y_spot = y * width;
            for (unsigned int x = 1; x < (width-1); ++x) {
                unsigned long long curr_spot = x + y_spot + z_spot;
                unsigned int spot0 = watershed[curr_spot];
                unsigned int spot1 = watershed[curr_spot-1];
                unsigned int spot2 = watershed[curr_spot+1];
                unsigned int spot3 = watershed[curr_spot-width];
                unsigned int spot4 = watershed[curr_spot+width];
                unsigned int spot5 = watershed[curr_spot-plane_size];
                unsigned int spot6 = watershed[curr_spot+plane_size];

                for (unsigned int i = 0; i < prediction_array.size(); ++i) {
                    predictions[i] = prediction_array[i][curr_spot];
                }

                if (feature_mgr && !median_mode) {
                    RagNode<Label> * node = rag->find_rag_node(spot0);
                    if (!node) {
                        node = rag->insert_rag_node(spot0);
                    }
                    node->incr_size();

                    feature_mgr->add_val(predictions, node);
                }

                if (spot1 && (spot0 != spot1)) {
                    rag_add_edge(rag, spot0, spot1, predictions, feature_mgr);
                }
                if (spot2 && (spot0 != spot2)) {
                    rag_add_edge(rag, spot0, spot2, predictions, feature_mgr);
                }
                if (spot3 && (spot0 != spot3)) {
                    rag_add_edge(rag, spot0, spot3, predictions, feature_mgr);
                }
                if (spot4 && (spot0 != spot4)) {
                    rag_add_edge(rag, spot0, spot4, predictions, feature_mgr);
                }
                if (spot5 && (spot0 != spot5)) {
                    rag_add_edge(rag, spot0, spot5, predictions, feature_mgr);
                }
                if (spot6 && (spot0 != spot6)) {
                    rag_add_edge(rag, spot0, spot6, predictions, feature_mgr);
                }
            }
        }
    } 

    boost::shared_ptr<PropertyList<Label> > node_list = NodePropertyList<Label>::create_node_list();
    rag->bind_property_list("border_node", node_list);

    for (unsigned int z = 1; z < (depth-1); z+=(depth-3)) {
        int z_spot = z * plane_size;
        for (unsigned int y = 1; y < (height-1); ++y) {
            int y_spot = y * width;
            for (unsigned int x = 1; x < (width-1); ++x) {
                unsigned long long curr_spot = x + y_spot + z_spot;
                property_list_add_template_property(node_list, rag->find_rag_node(watershed[curr_spot]), true);
            }
        }
    }
    for (unsigned int z = 1; z < (depth-1); ++z) {
        int z_spot = z * plane_size;
        for (unsigned int y = 1; y < (height-1); y+=(height-3)) {
            int y_spot = y * width;
            for (unsigned int x = 1; x < (width-1); ++x) {
                unsigned long long curr_spot = x + y_spot + z_spot;
                property_list_add_template_property(node_list, rag->find_rag_node(watershed[curr_spot]), true);
            }
        }
    }
    for (unsigned int z = 1; z < (depth-1); ++z) {
        int z_spot = z * plane_size;
        for (unsigned int y = 1; y < (height-1); ++y) {
            int y_spot = y * width;
            for (unsigned int x = 1; x < (width-1); x+=(width-3)) {
                unsigned long long curr_spot = x + y_spot + z_spot;
                property_list_add_template_property(node_list, rag->find_rag_node(watershed[curr_spot]), true);
            }
        }
    }   

    watershed_to_body[0] = 0;
    for (Rag<Label>::nodes_iterator iter = rag->nodes_begin(); iter != rag->nodes_end(); ++iter) {
        Label id = (*iter)->get_node_id();
        watershed_to_body[id] = id;
    }
/*
    for (Rag<Label>::edges_iterator iter = rag->edges_begin(); iter != rag->edges_end(); ++iter) {
        std::cout << (*iter)->get_node1()->get_node_id() << " " << (*iter)->get_node2()->get_node_id() << " " << feature_mgr->get_prob(*iter) << std::endl; 
    }*/
}



void Stack::agglomerate_rag(double threshold)
{
    MergePriority* priority = new ProbPriority(feature_mgr, rag);
    priority->initialize_priority(threshold);

    boost::shared_ptr<PropertyList<Label> > node_properties = rag->retrieve_property_list("border_node");

    while (!(priority->empty())) {

        RagEdge<Label>* rag_edge = priority->get_top_edge();

        if (!rag_edge) {
            continue;
        }

        RagNode<Label>* rag_node1 = rag_edge->get_node1();
        RagNode<Label>* rag_node2 = rag_edge->get_node2();
        Label node1 = rag_node1->get_node_id(); 
        Label node2 = rag_node2->get_node_id(); 
        rag_merge_edge_median(*rag, rag_edge, rag_node1, node_properties, priority, feature_mgr);
        watershed_to_body[node2] = node1;
        for (std::vector<Label>::iterator iter = merge_history[node2].begin(); iter != merge_history[node2].end(); ++iter) {
            watershed_to_body[*iter] = node1;
        }
        merge_history[node1].push_back(node2);
        merge_history[node1].insert(merge_history[node1].end(), merge_history[node2].begin(), merge_history[node2].end());
        merge_history.erase(node2);
    }
}

int Stack::remove_inclusions()
{
    int num_removed = 0;

    node_properties_holder = rag->retrieve_property_list("border_node");
    visited.clear();
    node_depth.clear();
    low_count.clear();
    prev_id.clear();
    biconnected_components.clear();
    stack.clear();

    RagNode<Label>* rag_node = 0;
    for (Rag<Label>::nodes_iterator iter = rag->nodes_begin(); iter != rag->nodes_end(); ++iter) {
        bool border = false;
        try {
            border = property_list_retrieve_template_property<Label, bool>(node_properties_holder, *iter);
        } catch (ErrMsg& msg) {
            //
        }

        if (border) {
            rag_node = *iter;
            break;
        }
    }
    assert(rag_node);

    DFSStack temp;
    temp.previous = 0;
    temp.rag_node = rag_node;
    temp.count = 1;
    temp.start_pos = 0;

    std::vector<DFSStack> dfs_stack;
    dfs_stack.push_back(temp);
    biconnected_dfs(dfs_stack);
    stack.clear();
    std::tr1::unordered_map<Label, Label> body_to_body; 
    std::tr1::unordered_map<Label, std::vector<Label> > merge_history2; 

    // merge nodes in biconnected_components (ignore components with '0' node)
    for (int i = 0; i < biconnected_components.size(); ++i) {
        bool found_zero = false;
        std::tr1::unordered_set<Label> merge_nodes;
        for (int j = 0; j < biconnected_components[i].size()-1; ++j) {
            Label region1 = biconnected_components[i][j].region1;     
            Label region2 = biconnected_components[i][j].region2;
            if (!region1 || !region2) {
                found_zero = true;
            }
            
            if (body_to_body.find(region1) != body_to_body.end()) {
                region1 = body_to_body[region1];
            }
            if (body_to_body.find(region2) != body_to_body.end()) {
                region2 = body_to_body[region2];
            }

            assert(region1 != region2);

            merge_nodes.insert(region1);
            merge_nodes.insert(region2);
        }
        if (!found_zero) {
            Label articulation_region = biconnected_components[i][biconnected_components[i].size()-1].region1;
            unsigned long long total_size = 0;
            RagNode<Label>* articulation_node = rag->find_rag_node(articulation_region);
            for (std::tr1::unordered_set<Label>::iterator iter = merge_nodes.begin(); iter != merge_nodes.end(); ++iter) {
                Label region1 = *iter;
                RagNode<Label>* rag_node = rag->find_rag_node(region1);
                total_size += rag_node->get_size();

            }
          
            bool found_preserve = false; 
            for (std::tr1::unordered_set<Label>::iterator iter = merge_nodes.begin(); iter != merge_nodes.end(); ++iter) {
                Label region2 = *iter;
                if (body_to_body.find(region2) != body_to_body.end()) {
                    region2 = body_to_body[region2];
                }

                RagNode<Label>* rag_node = rag->find_rag_node(region2);
                assert(rag_node);
                if (articulation_node != rag_node) {
                    for (RagNode<Label>::edge_iterator edge_iter = rag_node->edge_begin();
                        edge_iter != rag_node->edge_end(); ++edge_iter) {
                        if ((*edge_iter)->is_preserve()) {
                            found_preserve = true;
                            break;
                        }
                    } 
                }

                if (found_preserve) {
                    break;
                }
            }

            if (found_preserve) {
                continue;
            }
            
            articulation_node->set_size(total_size);
            for (std::tr1::unordered_set<Label>::iterator iter = merge_nodes.begin(); iter != merge_nodes.end(); ++iter) {
                Label region2 = *iter;
                if (body_to_body.find(region2) != body_to_body.end()) {
                    region2 = body_to_body[region2];
                }

                RagNode<Label>* rag_node = rag->find_rag_node(region2);
                assert(rag_node);
                if (articulation_node != rag_node) {
                    feature_mgr->merge_features(articulation_node, rag_node); 
                    rag->remove_rag_node(rag_node); 

                    watershed_to_body[region2] = articulation_region;
                    for (std::vector<Label>::iterator iter2 = merge_history[region2].begin(); iter2 != merge_history[region2].end(); ++iter2) {
                        watershed_to_body[*iter2] = articulation_region;
                    }

                    body_to_body[region2] = articulation_region;
                    for (std::vector<Label>::iterator iter2 = merge_history2[region2].begin(); iter2 != merge_history2[region2].end(); ++iter2) {
                        body_to_body[*iter2] = articulation_region;
                    }

                    merge_history[articulation_region].push_back(region2);
                    merge_history[articulation_region].insert(merge_history[articulation_region].end(), merge_history[region2].begin(), merge_history[region2].end());
                    merge_history.erase(region2);
                   
                    merge_history2[articulation_region].push_back(region2); 
                    merge_history2[articulation_region].insert(merge_history2[articulation_region].end(), merge_history2[region2].begin(), merge_history2[region2].end());
                    merge_history2.erase(region2);
                }
            }
        }
    }

    
    visited.clear();
    node_depth.clear();
    low_count.clear();
    prev_id.clear();
    biconnected_components.clear();
    stack.clear();
    return num_removed;
}

// return low point (0 is default edge)
void Stack::biconnected_dfs(std::vector<DFSStack>& dfs_stack)
{
    while (!dfs_stack.empty()) {
        DFSStack entry = dfs_stack.back();
        RagNode<Label>* rag_node = entry.rag_node;
        Label previous = entry.previous;
        int count = entry.count;
        dfs_stack.pop_back();

        if (visited.find(rag_node->get_node_id()) == visited.end()) {
            visited.insert(rag_node->get_node_id());
            node_depth[rag_node->get_node_id()] = count;
            low_count[rag_node->get_node_id()] = count;
            prev_id[rag_node->get_node_id()] = previous;
        }

        bool skip = false;
        int curr_pos = 0;
        for (RagNode<Label>::node_iterator iter = rag_node->node_begin(); iter != rag_node->node_end(); ++iter) {
            RagEdge<Label>* rag_edge = rag->find_rag_edge(rag_node, *iter);
            if (rag_edge->is_false_edge()) {
                continue;
            }
            
            if (curr_pos < entry.start_pos) {
                ++curr_pos;
                continue;
            }
            if (prev_id[(*iter)->get_node_id()] == rag_node->get_node_id()) {
                OrderedPair<Label> current_edge(rag_node->get_node_id(), (*iter)->get_node_id());
                int temp_low = low_count[(*iter)->get_node_id()];
                low_count[rag_node->get_node_id()] = std::min(low_count[rag_node->get_node_id()], temp_low);

                if (temp_low >= count) {
                    OrderedPair<Label> popped_edge;
                    biconnected_components.push_back(std::vector<OrderedPair<Label> >());
                    do {
                        popped_edge = stack.back();
                        stack.pop_back();
                        biconnected_components[biconnected_components.size()-1].push_back(popped_edge);
                    } while (!(popped_edge == current_edge));
                    OrderedPair<Label> articulation_pair(rag_node->get_node_id(), rag_node->get_node_id());
                    biconnected_components[biconnected_components.size()-1].push_back(articulation_pair);
                } 
            } else if (visited.find((*iter)->get_node_id()) == visited.end()) {
                OrderedPair<Label> current_edge(rag_node->get_node_id(), (*iter)->get_node_id());
                stack.push_back(current_edge);

                DFSStack temp;
                temp.previous = rag_node->get_node_id();
                temp.rag_node = (*iter);
                temp.count = count+1;
                temp.start_pos = 0;
                entry.start_pos = curr_pos;
                dfs_stack.push_back(entry);
                dfs_stack.push_back(temp);
                skip = true;
                break;
            } else if ((*iter)->get_node_id() != previous) {
                low_count[rag_node->get_node_id()] = std::min(low_count[rag_node->get_node_id()], node_depth[(*iter)->get_node_id()]);
                if (count > node_depth[(*iter)->get_node_id()]) {
                    stack.push_back(OrderedPair<Label>(rag_node->get_node_id(), (*iter)->get_node_id()));
                }
            }
            ++curr_pos;
        }

        if (skip) {
            continue;
        }

        bool border = false;
        try {
            border = property_list_retrieve_template_property<Label, bool>(node_properties_holder, rag_node);
        } catch (ErrMsg& msg) {
            //
        }
        if (previous && border) {
            low_count[rag_node->get_node_id()] = 0;
            stack.push_back(OrderedPair<Label>(0, rag_node->get_node_id()));
        }
    }
}


}

#endif

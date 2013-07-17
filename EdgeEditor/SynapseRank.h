#ifndef SYNAPSERANK_H
#define SYNAPSERANK_H

#include "NodeEdgeRank.h"

#include <string>

namespace NeuroProof {

class SynapseRank : public NodeEdgeRank {
  public:
    SynapseRank(Rag_uit* rag_) : NodeEdgeRank(rag_), ignore_size(0.1),
            voi_change_thres(0.0), volume_size(0),
            SynapseStr("synapse_weight") {}
  
    void initialize(double ignore_size);

    RagNode_uit* find_most_uncertain_node(RagNode_uit* head_node);
    void insert_node(Node_uit node);
    void update_neighboring_nodes(Node_uit keep_node);

  private:
    double ignore_size;
    double voi_change_thres;
    unsigned long long volume_size;
    const std::string SynapseStr;
};

}

#endif
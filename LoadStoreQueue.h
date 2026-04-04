#pragma once
#include <iostream>
#include <vector>
#include <string>
#include "Basics.h"

class LoadStoreQueue {
public:
    // LSQ reservation station
    int latency;

    std::vector<RSEntry> rs;
    
    std::vector<ExecutionResult> completed_results;

    void capture(int tag, int val);
    void executeCycle(std::vector<int>& Memory, std::vector<ROBEntry>& ROB, int rob_head);
};
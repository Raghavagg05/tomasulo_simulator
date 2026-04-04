#pragma once
#include <iostream>
#include <vector>
#include <string>
#include "Basics.h"

class ExecutionUnit {
public:
    // per-unit reservation station
    UnitType name;
    int latency;

    std::vector<RSEntry> rs;
    // Vector to hold completed results waiting to be broadcast
    std::vector<ExecutionResult> completed_results;
    
    void capture(int tag, int val);
    void executeCycle();
};
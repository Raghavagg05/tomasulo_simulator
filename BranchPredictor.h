#pragma once
#include "Basics.h"
#include <iostream>
#include <map>

class BranchPredictor {
public:
    int total_branches = 0;
    int correct_predictions = 0;

    // Using a map guarantees we never go out of bounds, no matter how big the program gets
    std::map<int, int> states;

    int predict(int current_pc, int imm, OpCode op) {
        // If we've never seen this branch before, initialize it to State 0
        if (states.find(current_pc) == states.end()) {
            states[current_pc] = 0;
        }
        
        int state = states[current_pc];
        
        // State 0, 1: Predict Taken. State 2, 3: Predict Not Taken.
        if (state == 0 || state == 1) {
            return current_pc + imm;
        } else {
            return current_pc + 1;
        }
    }

    void update(int pc, int actual_target, bool taken, bool was_correct) {
        total_branches++;
        if (was_correct) {
            correct_predictions++;
        }

        if (states.find(pc) == states.end()) {
            states[pc] = 0;
        }

        // 2-Bit Saturating Counter Logic
        if (taken) {
            if (states[pc] > 0) states[pc]--; // Move towards strongly taken
        } else {
            if (states[pc] < 3) states[pc]++; // Move towards strongly not taken
        }
    }
};
#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <iomanip>
#include "Basics.h"
#include "BranchPredictor.h"
#include "ExecutionUnit.h"
#include "LoadStoreQueue.h"

class Processor {
public:
    int pc;
    int clock_cycle;

    // pipeline registers


    Instruction fetch_buffer;
    // checks if there is something in the buffer
    bool fetch_valid = false; 
    bool fetch_stalled = false;
    int predicted_pc = 0;

    std::vector<Instruction> inst_memory;

    // architectural state (do not change)
    std::vector<int> ARF; // regFile
    std::vector<int> Memory; // Memory
    bool exception = false; // exception bit

    // register alias table / reorder buffer
    std::vector<ROBEntry> ROB;
    int rob_head, rob_tail, rob_count;
    std::vector<int> RAT;

    std::vector<ExecutionUnit> units;
    std::map<UnitType,int> exe_units_order;
    LoadStoreQueue* lsq;
    BranchPredictor bp;

    
    // Snapshots taken at the START of each cycle (before any stage runs).
    // stageDecode uses these instead of live values so that RS/ROB slots
    // freed by Execute or Commit in the same cycle are NOT visible to Decode
    int rob_count_snapshot;
    std::vector<int> rs_free_slot_snapshot; // index of first free RS slot
    int lsq_free_slot_snapshot;             // index of first free LSQ slot 

    Processor(ProcessorConfig& config);

    std::vector<std::string> helper_split(std::string s);

    int extract_reg_num(std::string word);

    void loadProgram(const std::string& filename);


    void flush();

    void broadcastOnCDB() {};

    void stageFetch();

    UnitType getUnitType(OpCode op);

    int check_rs_free_slot(int unit_index);

    void stageDecode();

    void stageExecuteAndBroadcast();

    void stageCommit();

    bool step();

    void dumpArchitecturalState() {
        std::cout << "\n=== ARCHITECTURAL STATE (CYCLE " << clock_cycle << ") ===\n";
        for (size_t i = 0; i < ARF.size(); i++) {
            std::cout << "x" << i << ": " << std::setw(4) << ARF[i] << " | ";
            if ((i+1) % 8 == 0) std::cout << std::endl;
        }
        if (exception) {
            std::cout << "EXCEPTION raised by instruction " << pc + 1 << std::endl;
        }
        std::cout << "Branch Predictor Stats: " << bp.correct_predictions << "/" << bp.total_branches << " correct.\n";
    };
};
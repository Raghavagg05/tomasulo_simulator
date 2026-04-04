#include "LoadStoreQueue.h"

void LoadStoreQueue::capture(int tag, int val) {
    for(size_t i = 0; i < rs.size(); i++){
        if(!rs[i].valid_flag) continue;
        
        if(!rs[i].inp1_ready && rs[i].inp1_tag == tag){
            rs[i].inp1_value = val;
            rs[i].inp1_ready = true;
        }
        if(!rs[i].inp2_ready && rs[i].inp2_tag == tag){
            rs[i].inp2_value = val;
            rs[i].inp2_ready = true;
        }
    }
}

void LoadStoreQueue::executeCycle(std::vector<int>& Memory, std::vector<ROBEntry>& ROB, int rob_head) {
    completed_results.clear();

    // Issue to pipeline
    int oldest_idx = -1;
    int min_seq = 1e9; 
    
    // Find the absolute oldest instruction that hasn't started yet
    for(size_t i = 0; i < rs.size(); i++){
        if(rs[i].valid_flag == true && rs[i].is_executing == false){
            if(rs[i].sequence_number < min_seq){
                min_seq = rs[i].sequence_number;
                oldest_idx = i;
            }
        }
    }

    // If we found one, we check if it's ready. 
    // If it's not ready, we don't skip it. We just stall.
    if(oldest_idx != -1){
        if(rs[oldest_idx].inp1_ready && rs[oldest_idx].inp2_ready){
            rs[oldest_idx].is_executing = true;
            rs[oldest_idx].rem_cycles = latency;
        }
    }

    // Advance executing instructions
    for(size_t i = 0; i < rs.size(); i++){
        if(rs[i].valid_flag && rs[i].is_executing){
            rs[i].rem_cycles--;
            
            if(rs[i].rem_cycles == 0){
                int res = 0;
                bool exc = false;
                int st_data = 0;
                
                // Address = Base Reg (inp1) + Offset (immediate)
                int addr = rs[i].inp1_value + rs[i].immediate_value;

                // Out of bounds check
                if(addr < 0 || addr >= (int)Memory.size()){
                    exc = true;
                } else {
                    if(rs[i].op_code == OpCode::LW){
                        // STORE-TO-LOAD FORWARDING LOGIC
                        int forwarded_data = Memory[addr]; // Default to actual memory
                        int curr = rob_head;
                        int my_rob_tag = rs[i].destination_tag;
                        
                        // Search ROB from oldest up to this load instruction
                        while (curr != my_rob_tag) {
                            if (ROB[curr].valid_flag && ROB[curr].op_code == OpCode::SW && ROB[curr].memory_address == addr) {
                                // Overwrite with the uncommitted store's data
                                forwarded_data = ROB[curr].store_value; 
                            }
                            curr = (curr + 1) % ROB.size();
                        }
                        res = forwarded_data;
                    } else if(rs[i].op_code == OpCode::SW){
                        st_data = rs[i].inp2_value; // The data we want to store
                    }
                }

                completed_results.push_back({rs[i].destination_tag, res, exc, addr, st_data});
                
                rs[i].valid_flag = false;
                rs[i].is_executing = false;
            }
        }
    }

}
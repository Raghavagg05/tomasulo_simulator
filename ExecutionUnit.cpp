#include "ExecutionUnit.h"

void ExecutionUnit::capture(int tag, int val) {
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

void ExecutionUnit::executeCycle() {
    // Clear the bus output from the previous cycle
    completed_results.clear();

    // Issue a new instruction into pipeline (oldest ready first)
    int start_idx = -1;
    int min_seq = 1e9; 
    
    for(size_t i = 0; i < rs.size(); i++){
        if(rs[i].valid_flag == true && rs[i].is_executing == false && rs[i].inp1_ready == true && rs[i].inp2_ready == true){
            if(rs[i].sequence_number < min_seq){
                min_seq = rs[i].sequence_number;
                start_idx = i;
            }
        }
    }

    if(start_idx != -1){
        rs[start_idx].is_executing = true;
        rs[start_idx].rem_cycles = latency;
    }
    
    // Advance executing instructions
    for(size_t i = 0; i < rs.size(); i++){
        if(rs[i].valid_flag && rs[i].is_executing){
            rs[i].rem_cycles--;
            
            if(rs[i].rem_cycles == 0){
                int res = 0;
                bool exc = false;
                OpCode op = rs[i].op_code;
                
                // Do the arithematic
                if(op == OpCode::ADD || op == OpCode::ADDI) res = rs[i].inp1_value + rs[i].inp2_value;
                else if(op == OpCode::SUB) res = rs[i].inp1_value - rs[i].inp2_value;
                else if(op == OpCode::MUL) res = rs[i].inp1_value * rs[i].inp2_value;
                else if(op == OpCode::DIV){
                    if(rs[i].inp2_value == 0) exc = true;
                    else res = rs[i].inp1_value / rs[i].inp2_value;
                }
                else if(op == OpCode::REM){
                    if(rs[i].inp2_value == 0) exc = true;
                    else res = rs[i].inp1_value % rs[i].inp2_value;
                }
                else if(op == OpCode::AND || op == OpCode::ANDI) res = rs[i].inp1_value & rs[i].inp2_value;
                else if(op == OpCode::OR || op == OpCode::ORI) res = rs[i].inp1_value | rs[i].inp2_value;
                else if(op == OpCode::XOR || op == OpCode::XORI) res = rs[i].inp1_value ^ rs[i].inp2_value;
                else if(op == OpCode::SLT || op == OpCode::SLTI) res = (rs[i].inp1_value < rs[i].inp2_value) ? 1 : 0;

                else if(op == OpCode::BEQ) res = (rs[i].inp1_value == rs[i].inp2_value) ? 1 : 0;
                else if(op == OpCode::BNE) res = (rs[i].inp1_value != rs[i].inp2_value) ? 1 : 0;
                else if(op == OpCode::BLT) res = (rs[i].inp1_value < rs[i].inp2_value) ? 1 : 0;
                else if(op == OpCode::BLE) res = (rs[i].inp1_value <= rs[i].inp2_value) ? 1 : 0;
                // Push the complete package to the vector
                completed_results.push_back({rs[i].destination_tag, res, exc});
                
                // Free the RS
                rs[i].valid_flag = false;
                rs[i].is_executing = false;
            }
        }
    }

}
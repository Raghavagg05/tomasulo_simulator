#pragma once
#include <iostream>
#include <fstream>
#include <vector>
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
    LoadStoreQueue* lsq;
    BranchPredictor bp;

    Processor(ProcessorConfig& config) {
        pc = 0;
        clock_cycle = 0;
        ARF.resize(config.num_regs, 0);
        Memory.resize(config.mem_size);

        // Instantiate Hardware Units

        ExecutionUnit adder;
        adder.name = UnitType::ADDER;
        adder.latency = config.add_lat;
        adder.rs.resize(config.adder_rs_size);
        units.push_back(adder);

        ExecutionUnit multiplier;
        multiplier.name = UnitType::MULTIPLIER;
        multiplier.latency = config.mul_lat;
        multiplier.rs.resize(config.mult_rs_size);
        units.push_back(multiplier);

        ExecutionUnit divider;
        divider.name = UnitType::DIVIDER;
        divider.latency = config.div_lat;
        divider.rs.resize(config.div_rs_size);
        units.push_back(divider);

        ExecutionUnit branch;
        branch.name = UnitType::BRANCH;
        branch.latency = config.logic_lat;
        branch.rs.resize(config.br_rs_size);
        units.push_back(branch);
        
        ExecutionUnit logic;
        logic.name = UnitType::LOGIC;
        logic.latency = config.logic_lat;
        logic.rs.resize(config.logic_rs_size);
        units.push_back(logic);

        lsq = new LoadStoreQueue();
        lsq->latency = config.mem_lat;
        lsq->rs.resize(config.lsq_rs_size);

        ROB.resize(config.rob_size);
        rob_head = 0;
        rob_tail = 0;
        rob_count = 0;
        RAT.resize(config.num_regs, -1);
    }

    std::vector<std::string> helper_split(std::string s){
        std::vector<std::string> words;
        std::string cur_word = "";
        int i=0;
        int n=s.size();
        while(i<n){
            while(i<n && (s[i]==' ' || s[i]==','))i++;
            while(i<n && (s[i]!=' ' && s[i]!=',')){
                cur_word+=s[i];
                i++;
            }
            if (cur_word.size()) {
                words.push_back(cur_word);
                cur_word = "";
            }
        }
        return words;
    }

    int extract_reg_num(std::string word){
        std::string reg_num = "";
        for(int j=1;j<word.size();j++)reg_num+=word[j];
        return stoi(reg_num);
    }

    void loadProgram(const std::string& filename) {
        std::ifstream file(filename);
        std::string line;
        while (std::getline(file, line)) {
            std::vector<std::string> words = helper_split(line);
            if (words.empty()) continue;
            //memory values initialisation
            if(words[0][0]=='.'){
                for(int i=1;i<words.size();i++)Memory[i-1] = stoi(words[i]);
                continue;
            }
            Instruction cur_instruction;
            //Memory Instructions
            if(words[0]=="lw" || words[0]=="sw"){
                if(words[0]=="lw")cur_instruction.op = OpCode::LW;
                else cur_instruction.op = OpCode::SW;
                std::string immediate = "";
                if(words[0]=="lw")cur_instruction.dest = extract_reg_num(words[1]);
                else cur_instruction.src2 = extract_reg_num(words[1]);
                for(int j=0;j<words[2].size();j++){
                    if(words[2][j]=='(')break;
                    immediate += words[2][j];
                }
                cur_instruction.imm = stoi(immediate);
                std::string reg_num = "";
                for(int j=words[2].size()-2;j>=0;j--){
                    if(words[2][j]=='x')break;
                    reg_num = words[2][j] + reg_num;
                }
                cur_instruction.src1 = stoi(reg_num);
            }
            //Jump instructions
            else if(words[0]=="j"){
                cur_instruction.op = OpCode::J;
                cur_instruction.imm = stoi(words[1]);
            }
            //Branch instructions
            else if(words[0][0]=='b'){
                if(words[0]=="beq")cur_instruction.op = OpCode::BEQ;
                else if(words[0]=="bne")cur_instruction.op = OpCode::BNE;
                else if(words[0]=="blt")cur_instruction.op = OpCode::BLT;
                else if(words[0]=="ble")cur_instruction.op = OpCode::BLE;
                cur_instruction.src1 = extract_reg_num(words[1]);
                cur_instruction.src2 = extract_reg_num(words[2]);
                cur_instruction.imm = stoi(words[3]);
            }
            //R-type instructions
            else if(words[0].back()!='i'){
                if (words[0] == "add") cur_instruction.op = OpCode::ADD;
                else if (words[0] == "sub") cur_instruction.op = OpCode::SUB;
                else if (words[0] == "mul") cur_instruction.op = OpCode::MUL;
                else if (words[0] == "div") cur_instruction.op = OpCode::DIV;
                else if (words[0] == "rem") cur_instruction.op = OpCode::REM;
                else if (words[0] == "slt") cur_instruction.op = OpCode::SLT;
                else if (words[0] == "and") cur_instruction.op = OpCode::AND;
                else if (words[0] == "or")  cur_instruction.op = OpCode::OR;
                else if (words[0] == "xor") cur_instruction.op = OpCode::XOR;
                cur_instruction.dest = extract_reg_num(words[1]);
                cur_instruction.src1 = extract_reg_num(words[2]);
                cur_instruction.src2 = extract_reg_num(words[3]);
            }
            //I-type instructions
            else if(words[0].back()=='i'){
                if (words[0] == "addi") cur_instruction.op = OpCode::ADDI;
                else if (words[0] == "slti") cur_instruction.op = OpCode::SLTI;
                else if (words[0] == "andi") cur_instruction.op = OpCode::ANDI;
                else if (words[0] == "ori")  cur_instruction.op = OpCode::ORI;
                else if (words[0] == "xori") cur_instruction.op = OpCode::XORI;
                cur_instruction.dest = extract_reg_num(words[1]);
                cur_instruction.src1 = extract_reg_num(words[2]);
                cur_instruction.imm = stoi(words[3]);
            }

            cur_instruction.pc = inst_memory.size();
            inst_memory.push_back(cur_instruction);
        }
    }


    void flush() {};

    void broadcastOnCDB() {};

    void stageFetch() {};

    void stageDecode() {};

    void stageExecuteAndBroadcast() {};

    void stageCommit() {};

    bool step() {
        clock_cycle++;
        return true; // return false if CPU has no more to do after this cycle
    }

    void dumpArchitecturalState() {
        std::cout << "\n=== ARCHITECTURAL STATE (CYCLE " << clock_cycle << ") ===\n";
        for (int i = 0; i < ARF.size(); i++) {
            std::cout << "x" << i << ": " << std::setw(4) << ARF[i] << " | ";
            if ((i+1) % 8 == 0) std::cout << std::endl;
        }
        if (exception) {
            std::cout << "EXCEPTION raised by instruction " << pc + 1 << std::endl;
        }
        std::cout << "Branch Predictor Stats: " << bp.correct_predictions << "/" << bp.total_branches << " correct.\n";
    }
};
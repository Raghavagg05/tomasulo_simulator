#pragma once
#include <string>

enum class OpCode { ADD, SUB, ADDI, MUL, DIV, REM, LW, SW, BEQ, BNE, BLT, BLE, J, SLT, SLTI, AND, OR, XOR, ANDI, ORI, XORI };
enum class UnitType { ADDER, MULTIPLIER, DIVIDER, LOADSTORE, BRANCH, LOGIC };

struct Instruction {
    OpCode op = OpCode::ADD;
    int dest = 0;
    int src1 = 0;
    int src2 = 0;
    int imm = 0;
    int pc = 0;
};

struct ProcessorConfig {
    int num_regs = 32;
    int rob_size = 64;
    int mem_size = 1024;

    int logic_lat = 1;
    int add_lat = 2;
    int mul_lat = 4;
    int div_lat = 5;
    int mem_lat = 4;

    int logic_rs_size = 4;
    int adder_rs_size = 4;
    int mult_rs_size = 2;
    int div_rs_size = 2;
    int br_rs_size = 2;
    int lsq_rs_size = 32;
};

struct ROBEntry {
    bool valid_flag = false;
    bool ready_flag = false;
    OpCode op_code = OpCode::ADD;
    int instruction_pc = 0;
    int destination_register = -1;
    int output_value = 0;
    bool exception_flag = false;
    int predicted_address = -1;
    int next_address = 0;
    int memory_address = -1;
    int store_value = 0;
};

struct RSEntry {
    bool valid_flag = false;
    OpCode op_code = OpCode::ADD;
    int inp1_value = 0;
    int inp1_tag = -1;
    bool inp1_ready = false;
    int inp2_value = 0;
    int inp2_tag = -1;
    bool inp2_ready = false;
    int destination_tag = -1;
    int immediate_value = 0;
    int sequence_number = 0;
    int pc_address = 0;
    bool is_executing = false;
    int rem_cycles = 0;
};

struct ExecutionResult {
    int tag;
    int value;
    bool exception;
    int mem_address = -1; // Added for LSQ
    int store_data = 0;   // Added for LSQ
};
#include "Processor.h"

Processor::Processor(ProcessorConfig& config) {
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
    exe_units_order[UnitType::ADDER] = 0;

    ExecutionUnit multiplier;
    multiplier.name = UnitType::MULTIPLIER;
    multiplier.latency = config.mul_lat;
    multiplier.rs.resize(config.mult_rs_size);
    units.push_back(multiplier);
    exe_units_order[UnitType::MULTIPLIER] = 1;

    ExecutionUnit divider;
    divider.name = UnitType::DIVIDER;
    divider.latency = config.div_lat;
    divider.rs.resize(config.div_rs_size);
    units.push_back(divider);
    exe_units_order[UnitType::DIVIDER] = 2;

    ExecutionUnit branch;
    branch.name = UnitType::BRANCH;
    branch.latency = config.add_lat;
    branch.rs.resize(config.br_rs_size);
    units.push_back(branch);
    exe_units_order[UnitType::BRANCH] = 3;
    
    ExecutionUnit logic;
    logic.name = UnitType::LOGIC;
    logic.latency = config.logic_lat;
    logic.rs.resize(config.logic_rs_size);
    units.push_back(logic);
    exe_units_order[UnitType::LOGIC] = 4;

    lsq = new LoadStoreQueue();
    lsq->latency = config.mem_lat;
    lsq->rs.resize(config.lsq_rs_size);

    ROB.resize(config.rob_size);
    rob_head = 0;
    rob_tail = 0;
    rob_count = 0;
    RAT.resize(config.num_regs, -1);
}

std::vector<std::string> Processor::helper_split(std::string s) {
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

int Processor::extract_reg_num(std::string word){
    std::string reg_num = "";
    for(size_t j=1;j<word.size();j++)reg_num+=word[j];
    return stoi(reg_num);
}

void Processor::loadProgram(const std::string& filename) {
    std::ifstream file(filename);
    std::string line;
    while (std::getline(file, line)) {
        std::vector<std::string> words = helper_split(line);
        if (words.empty()) continue;
        //memory values initialisation
        if(words[0][0]=='.'){
            for(size_t i=1;i<words.size();i++)Memory[i-1] = stoi(words[i]);
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
            for(size_t j=0;j<words[2].size();j++){
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

void Processor::stageFetch() {
    // If stalled or buffer is full, do nothing
    if (fetch_stalled || fetch_valid) return;

    if (pc < 0 || pc >= (int)inst_memory.size()) return;

    Instruction current = inst_memory[pc];
    
    if (current.op == OpCode::J) {
        predicted_pc = pc + current.imm;
    } 
    else if (current.op == OpCode::BEQ || current.op == OpCode::BNE || current.op == OpCode::BLT || current.op == OpCode::BLE) {
        predicted_pc = bp.predict(pc, current.imm, current.op);
    } 
    else {
        predicted_pc = pc + 1;
    }
    
    pc = predicted_pc;
    fetch_buffer = current;
    fetch_valid = true;
}

UnitType Processor::getUnitType(OpCode op) {
    if(op == OpCode::ADD || op == OpCode::SUB || op == OpCode::ADDI || op == OpCode::SLT || op == OpCode::SLTI) return UnitType::ADDER;
    else if(op == OpCode::MUL) return UnitType::MULTIPLIER;
    else if(op == OpCode::DIV || op == OpCode::REM) return UnitType::DIVIDER;
    else if(op == OpCode::AND || op == OpCode::OR || op == OpCode::XOR || op == OpCode::ANDI || op == OpCode::ORI || op == OpCode::XORI) return UnitType::LOGIC;
    else if(op == OpCode::BEQ || op == OpCode::BNE || op == OpCode::BLT || op == OpCode::BLE) return UnitType::BRANCH;
    else if(op == OpCode::LW || op == OpCode::SW) return UnitType::LOADSTORE;
    return UnitType::ADDER;
}

int Processor::check_rs_free_slot(int unit_index){
    int free_rs = -1;
    for (size_t i = 0; i < units[unit_index].rs.size(); i++) {
        if (!units[unit_index].rs[i].valid_flag) {
            free_rs = i;
            break;
        }
    }
    return free_rs;
}

void Processor::stageDecode() {
    if(!fetch_valid || rob_count>= (int)ROB.size())return;
    
    int free_rs = -1;
    int rs_ind = -1;

    if (fetch_buffer.op == OpCode::J) {}
    else if (fetch_buffer.op == OpCode::LW || fetch_buffer.op == OpCode::SW) {
        for (int i = 0; i < (int)lsq->rs.size(); i++) {
            if (!lsq->rs[i].valid_flag) { free_rs = i; break; }
        }
        if (free_rs == -1) { fetch_stalled = true; return; }
    } else {
        rs_ind = exe_units_order[getUnitType(fetch_buffer.op)];
        free_rs = check_rs_free_slot(rs_ind);
        if (free_rs == -1) { fetch_stalled = true; return; }
    }

    int rob_index = rob_tail;
    ROB[rob_index].ready_flag = false;
    ROB[rob_index].exception_flag = false;
    ROB[rob_index].memory_address = -1;
    ROB[rob_index].store_value = 0;
    ROB[rob_index].output_value = 0;
    ROB[rob_index].valid_flag = true;
    ROB[rob_index].op_code = fetch_buffer.op;
    ROB[rob_index].instruction_pc = fetch_buffer.pc;
    ROB[rob_index].destination_register = fetch_buffer.dest;
    ROB[rob_index].predicted_address = predicted_pc; 

    rob_tail = (rob_tail + 1) % ROB.size();
    rob_count++;

    if(fetch_buffer.op==OpCode::J){
        ROB[rob_index].ready_flag = true;
        fetch_valid = false;
        fetch_stalled = false;
        return;
    }

    RSEntry* rs_entry;

    if(fetch_buffer.op==OpCode::LW || fetch_buffer.op==OpCode::SW)rs_entry = &lsq->rs[free_rs];
    else rs_entry = &units[rs_ind].rs[free_rs];

    rs_entry->valid_flag = true;
    rs_entry->op_code = fetch_buffer.op;
    rs_entry->destination_tag = rob_index;
    rs_entry->immediate_value = fetch_buffer.imm;
    rs_entry->sequence_number = clock_cycle;  
    rs_entry->pc_address = fetch_buffer.pc;

    int src1 = fetch_buffer.src1;
    if(RAT[src1] == -1){
        rs_entry->inp1_value = ARF[src1];
        rs_entry->inp1_ready = true;
    }
    else{
        int tag = RAT[src1];
        if(ROB[tag].ready_flag == true){
            rs_entry->inp1_value = ROB[tag].output_value;
            rs_entry->inp1_ready = true;
        }
        else{
            rs_entry->inp1_tag = tag;
            rs_entry->inp1_ready = false;
        }
    }

    if(fetch_buffer.op==OpCode::ADDI || fetch_buffer.op==OpCode::SLTI || fetch_buffer.op==OpCode::ANDI || fetch_buffer.op==OpCode::ORI || fetch_buffer.op==OpCode::XORI || fetch_buffer.op==OpCode::LW){
        rs_entry->inp2_value = fetch_buffer.imm;
        rs_entry->inp2_ready = true;
    }
    else{
        int src2 = fetch_buffer.src2;
        if(RAT[src2] == -1){
            rs_entry->inp2_value = ARF[src2];
            rs_entry->inp2_ready = true;
        }
        else{
            int tag = RAT[src2];
            if(ROB[tag].ready_flag == true){
                rs_entry->inp2_value = ROB[tag].output_value;
                rs_entry->inp2_ready = true;
            }
            else{
                rs_entry->inp2_tag = tag;
                rs_entry->inp2_ready = false;
            }
        }
    }
    if(fetch_buffer.dest>0)RAT[fetch_buffer.dest] = rob_index;
    fetch_valid = false;
    fetch_stalled = false;
}

void Processor::stageExecuteAndBroadcast() {
    // A temporary buffer to represent the Common Data Bus (CDB) this cycle
    std::vector<ExecutionResult> cdb;

    // tick all ALU units and collect their results
    for (size_t i = 0; i < units.size(); i++) {
        units[i].executeCycle();
        for (auto res : units[i].completed_results) {
            cdb.push_back(res);
        }
    }

    //tick the LoadStoreQueue and collect its results
    lsq->executeCycle(Memory, ROB, rob_head);
    for (auto res : lsq->completed_results) {
        cdb.push_back(res);
    }

    // Broadcast everything on the CDB
    for (auto res : cdb) {
        int tag = res.tag;
        int val = res.value;

        // A. Update the Reorder Buffer (ROB)
        ROB[tag].ready_flag = true;
        ROB[tag].output_value = val;
        ROB[tag].exception_flag = res.exception;
        
        // If it was a memory instruction, save the address and store data
        if (res.mem_address != -1) {
            ROB[tag].memory_address = res.mem_address;
            ROB[tag].store_value = res.store_data;
        }

        // Scan the bus: Tell all Reservation Stations to check for this tag
        for (size_t i = 0; i < units.size(); i++) {
            units[i].capture(tag, val);
        }
        lsq->capture(tag, val);
    }
}

void Processor::stageCommit() {
    if (rob_count == 0) {
        return;
    }
    int h = rob_head;

    // Instruction must be valid and finished computing to commit
    if (ROB[h].valid_flag == false || ROB[h].ready_flag == false) {
        return;
    }

    // Exception Handling
    if (ROB[h].exception_flag == true) {
        pc = ROB[h].instruction_pc; // Point PC to the culprit
        exception = true;
        flush();
        return; // Halt immediately
    }

    OpCode op = ROB[h].op_code;

    // Commit to Memory (Stores)
    if (op == OpCode::SW) {
        Memory[ROB[h].memory_address] = ROB[h].store_value;
    }

    // Commit to Architectural Register File (ARF)
    int dest = ROB[h].destination_register;
    if (dest > 0) { // Remember: RISC-V x0 is hardwired to 0
        ARF[dest] = ROB[h].output_value;

        // If the RAT is still pointing to this ROB entry, clear it
        if (RAT[dest] == h) {
            RAT[dest] = -1;
        }
    }

    // Branch Resolution & Flush
    if (op == OpCode::BEQ || op == OpCode::BNE || op == OpCode::BLT || op == OpCode::BLE) {
        Instruction current_inst = inst_memory[ROB[h].instruction_pc];

        int correct_pc = 0;
        bool taken = false;
        
        if (ROB[h].output_value == 1) {
            taken = true;
            correct_pc = current_inst.pc + current_inst.imm;
        } else {
            correct_pc = current_inst.pc + 1;
        }

        bool was_correct = (correct_pc == ROB[h].predicted_address);

        // Send everything to the predictor to update stats and the state machine
        bp.update(ROB[h].instruction_pc, correct_pc, taken, was_correct);

        if (!was_correct) {
            pc = correct_pc;
            flush();
            return; 
        }
    }

    // Normal Deallocation
    ROB[h].valid_flag = false;
    rob_head = (rob_head + 1) % ROB.size();
    rob_count--;
}

void Processor::flush() {
    // Stall the Fetch stage and clear the buffer
    fetch_valid = false;
    fetch_stalled = false;

    // Wipe the Register Alias Table (redirecting everything back to ARF)
    for (size_t i = 0; i < RAT.size(); i++) {
        RAT[i] = -1;
    }

    // Clear the Reorder Buffer
    for (size_t i = 0; i < ROB.size(); i++) {
        ROB[i].valid_flag = false;
    }
    rob_head = 0;
    rob_tail = 0;
    rob_count = 0;

    // End all running instructions in the Execution Units
    for (size_t i = 0; i < units.size(); i++) {
        for (size_t j = 0; j < units[i].rs.size(); j++) {
            units[i].rs[j].valid_flag = false;
            units[i].rs[j].is_executing = false;
        }
    }

    // End all running instructions in the Load/Store Queue
    for (size_t i = 0; i < lsq->rs.size(); i++) {
        lsq->rs[i].valid_flag = false;
        lsq->rs[i].is_executing = false;
    }
}

bool Processor::step() {
    // Record the PC before commit to detect if a flush happens
    int pc_before_commit = pc;
    // Run pipeline stages in reverse to simulate cycle-boundary latching
    stageCommit();
    stageExecuteAndBroadcast();
    stageDecode();
    if (pc == pc_before_commit) {
        stageFetch();
    }

    clock_cycle++;

    //  Termination Condition A: An exception was raised
    if (exception) {
        return false;
    }

    // Termination Condition B: Program is completely finished
    // (No more instructions to fetch AND nothing waiting in the fetch buffer AND the ROB is totally empty)
    if (pc >= (int)inst_memory.size() && fetch_valid == false && rob_count == 0) {
        return false; 
    }

    return true;
}
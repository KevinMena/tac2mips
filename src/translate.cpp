#include "translate.hpp"

Translator::Translator()
{
    // Add MIPS registers
    //insertRegister("$v0");
    insertRegister("$v1", m_registers);

    int i = 0;
    while (i < 3)
    {   
        insertRegister("$a" + to_string(i), m_registers);
        i++;
    }

    i = 0;
    
    while (i < 10)
    {   
        insertRegister("$t" + to_string(i), m_registers);
        i++;
    }

    i = 0;
    
    while (i < 8)
    {   
        insertRegister("$s" + to_string(i), m_registers);
        i++;
    }

    // Add the float registers
    i = 0;
    
    while (i < 32)
    {   
        insertRegister("$f" + to_string(i), m_float_registers);
        i++;
    }

    // Oh no...
    m_text.emplace_back("li  $sp, 0x7fc00000");
    m_text.emplace_back("li  $fp, 0x7fc00000");
}

void Translator::insertInstruction(T_Instruction* instruction)
{
    m_data_instructions.push_back(instruction);
}

bool Translator::insertRegister(const string& id, unordered_map<string, vector<string>>& descriptors)
{
    auto found = descriptors.find(id);

    if(found != descriptors.end()) 
        return false;
    
    descriptors[id] = vector<string>();
    return true;
}

bool Translator::insertVariable(const string& id)
{
    auto found = m_variables.find(id);

    if(found != m_variables.end()) 
        return false;
    
    // Insert descriptor
    vector<string> locations { id };
    m_variables[id] = locations;
    return true;
}

void Translator::loadTemporal(const string& id, const string& register_id, bool maintain_descriptor)
{
    // Temporal information
    uint64_t size = m_graph->temps_size[id];
    uint64_t offset = m_graph->temps_offset[id];
    unordered_map<string, vector<string>>* descriptors = &m_registers;
    string load_id = size == 1 ? "loadb" : "load";
    string location = id;

    // Check if is a float
    if(id.front() == 'f' || id.front() == 'F')
    {
        descriptors = &m_float_registers;
        load_id = "fload";
    }

    // Calculate position to store in the stack, load immediate or load the address
    if(is_number(id))
        load_id = "loadi";
    else if(is_static(id))
        load_id = "loada";
    else if(id.front() == 'f' || id.front() == 'F')
    {
        descriptors = &m_float_registers;
        load_id = "fload";
        m_text.emplace_back(mips_instructions.at("load") + space + "$v0" + sep + "BASE");
        location = to_string(offset) + "($v0)";
    }
    else if(!is_global(id))
    {
        m_text.emplace_back(mips_instructions.at("load") + space + register_id + sep + "BASE");
        location = to_string(offset) + "(" + register_id + ")";
    }
    
    m_text.emplace_back(mips_instructions.at(load_id) + space + register_id + sep + location);

    if(!maintain_descriptor)
        return;

    // Maintain descriptors
    assignment(register_id, id, *descriptors, true);
    availability(id, register_id);
}

void Translator::storeTemporal(const string& id, const string& register_id, bool replace)
{
    // Temporal information
    uint64_t size = m_graph->temps_size[id];
    uint64_t offset = m_graph->temps_offset[id];
    string store_id = size == 1 ? "storeb" : "store";

    if(id.front() == 'f' || id.front() == 'F')
        store_id = "fstore";

    if(!is_global(id))
    {
        m_text.emplace_back(mips_instructions.at("load") + space + "$v0" + sep + "BASE");    
        m_text.emplace_back(mips_instructions.at(store_id) + space + register_id + sep + to_string(offset) + "($v0)");
    }
    else
        m_text.emplace_back(mips_instructions.at(store_id) + space + register_id + sep + id);
    
    // Maintain descriptors
    availability(id, id, replace);
}

void Translator::insertFlowGraph(FlowGraph* graph)
{
    m_graph = graph;
}

void Translator::print()
{
    cout << ".data" << endl;
    for(string inst : m_data)
    {
        cout << inst << endl;
    }

    cout << "\n.text" << endl;
    for(string inst : m_text)
    {
        cout << inst << endl;
    }

}

bool Translator::insertElementToDescriptor(unordered_map<string, vector<string>> &descriptors, 
                                        const string& key, const string& element, bool replace)
{
    auto found = descriptors.find(key);

    if(found == descriptors.end()) 
        return false;
        
    if(find(descriptors[key].begin(), descriptors[key].end(), element) != descriptors[key].end())
        return false;
    
    if(replace)
        descriptors[key].clear();
    
    descriptors[key].push_back(element);
    return true;
}

void Translator::removeElementFromDescriptors(unordered_map<string, vector<string>> &descriptors, 
                                            const string& element, const string& current_container)
{   
    for (pair<string, vector<string>> current_descriptor : descriptors) 
    {
        if(current_descriptor.first == current_container)
            continue;

        descriptors[current_descriptor.first].erase(
            remove(descriptors[current_descriptor.first].begin(), descriptors[current_descriptor.first].end(), element),
            descriptors[current_descriptor.first].end());
    }
}

void Translator::cleanRegistersDescriptor()
{
    for(auto current_register : m_registers)
    {
        m_registers[current_register.first].clear();
    }
}

bool Translator::assignment(const string& register_id, const string& variable_id, 
                            unordered_map<string, vector<string>>& curr_registers, bool replace)
{
    return insertElementToDescriptor(curr_registers, register_id, variable_id, replace);
}

bool Translator::availability(const string& variable_id, const string& location, bool replace)
{
    return insertElementToDescriptor(m_variables, variable_id, location, replace);
}

vector<string> Translator::getRegisterDescriptor(const string& id, unordered_map<string, vector<string>>& curr_registers)
{
    return curr_registers[id];
}

vector<string> Translator::getVariableDescriptor(const string& id)
{
    return m_variables[id];
}

string Translator::findOptimalLocation(string id)
{    
    //vector<string> descriptor = getVariableDescriptor(id);
    // For now just return itself
    return id;
}

string Translator::findElementInDescriptors(unordered_map<string, vector<string>> &descriptors, const string& element)
{   
    for (pair<string, vector<string>> current_descriptor : descriptors) 
    {
        vector<string> current_elements = current_descriptor.second;
        if( find(current_elements.begin(), current_elements.end(), element) != current_elements.end())
            return current_descriptor.first;
    }

    return "";
}

vector<string> Translator::findFreeRegister(unordered_map<string, vector<string>>& curr_registers)
{
    vector<string> regs;
    for (pair<string, vector<string>> current_register : curr_registers) 
    {
        if(current_register.second.empty())
            regs.push_back(current_register.first);
    }

    return regs;
}

string Translator::recycleRegister(T_Instruction instruction, unordered_map<string, vector<string>>& descriptors, 
                                    vector<string> &regs)
{
    // Traverse all the register and check which one is the most viable
    // for recycling and using it

    string best_reg = "";

    // Counter of spills
    map<string, int> spills;
    unordered_map<string, vector<string>> spills_emit;
    
    for (pair<string, vector<string>> current_register : descriptors) 
    {
        // If the register is already one we chose, not spill it
        if(find(regs.begin(), regs.end(), current_register.first) != regs.end())
            continue;
        
        //bool isSafe = false;
        int current_spills = 0;

        vector<string> descriptor = current_register.second;

        for(string element : descriptor)
        {
            if(is_number(element))
                continue;
            
            // First, verify if the current element is store in another place
            if(m_variables[element].size() > 1)
                // The register is safe to use
                continue;
            
            // Verify if the current element is the result and if it is then check that
            // is not any of the operands
            bool found = false;
            for(size_t i = 0; i < instruction.operands.size(); i++)
            {
                if(instruction.operands[i].name == element)
                {
                    found = true;
                    break;
                }
            }
            if(element == instruction.result.name && found)
                // The register is safe
                continue;

            // Check that the current element doesn't have later uses

            // If the register is still not safe, spill
            spills_emit[current_register.first].push_back(element);
            
            current_spills += 1;
        }

        spills[current_register.first] = current_spills;
    }

    // Now select the register with the least spills
    int min_spill = INT32_MAX;
    
    for (pair<string, int> spill : spills) 
    {
        if(spill.second < min_spill)
            best_reg = spill.first;
    }

    for(auto element : spills_emit[best_reg])
    {
        // If static variable ignore
        if(!is_static(element))
            storeTemporal(element, best_reg, false);
        else
            availability(element, element);
    }

    descriptors[best_reg].clear();
    removeElementFromDescriptors(m_variables, best_reg, "");

    return best_reg;
}

void Translator::selectRegister(const string& operand, T_Instruction instruction, unordered_map<string, vector<string>>& descriptors,
                                vector<string> &regs, vector<string> &free_regs)
{
    // First, verify is the operand is store in another register
    string reg = findElementInDescriptors(descriptors, operand);

    if(!reg.empty())
    {
        regs.push_back(reg);
        return;
    }
    
    // If not, then check if we have any free registers
    if(free_regs.size() > 0)
    {
        regs.push_back(free_regs.front());
        free_regs.erase(free_regs.begin());
        return;
    }
    
    // If any of the simple cases are not meet, the we move to the complex ones
    // No free registers and the operand is not store in any
    reg = recycleRegister(instruction, descriptors, regs);
    regs.push_back(reg);
}

vector<string> Translator::getReg(T_Instruction instruction, bool is_copy)
{
    vector<string> registers;

    // Look for the registers that are available, depending if float registers or normal
    vector<string> free_r = findFreeRegister(m_registers);
    vector<string> free_fr = findFreeRegister(m_float_registers);

    // References necessaries depending if the operand is a float
    unordered_map<string, vector<string>>* curr_desc = &m_registers;
    vector<string>* free_regs = &free_r; 

    // Choose register for every operand
    for (T_Variable current_operand : instruction.operands)
    {
        if(current_operand.name.empty())
            continue;
        
        // If the operand is a float change the references
        if(current_operand.name.front() == 'f' || current_operand.name.front() == 'F')
        {
            free_regs = &free_fr;
            curr_desc = &m_float_registers;
        }
        else
        {
            free_regs = &free_r;
            curr_desc = &m_registers;
        }

        selectRegister(current_operand.name, instruction, *curr_desc, registers, *free_regs);
    }
    
    // If a jump instruction then is just necessary the register for the operand
    if(instruction.id.find("go") != string::npos)
        return registers;

    // Choose the register for the result
    if(is_copy && !instruction.result.is_acc && !instruction.operands[0].is_acc)
    {
        string result_reg = registers[0];
        registers.insert(registers.begin(), result_reg);
    }
    else
    {
        // Check if the result is a float to look in the correct descriptors
        if(instruction.result.name.front() == 'f' || instruction.result.name.front() == 'F')
        {
            free_regs = &free_fr;
            curr_desc = &m_float_registers;
        }
        else
        {
            free_regs = &free_r;
            curr_desc = &m_registers;
        }    

        // Look for a register that ONLY has the result
        string reg = findElementInDescriptors(*curr_desc, instruction.result.name);
        if(!reg.empty() && getRegisterDescriptor(reg, *curr_desc).size() < 2)
        {
            registers.insert(registers.begin(), reg);
        }
        // TODO: If the operands don't have later uses, we choose any of these registers
        else
        {
            // Otherwise the same as one of the operands
            selectRegister(instruction.result.name, instruction, *curr_desc, registers, *free_regs);
            string reg = registers.back();
            registers.pop_back();
            registers.insert(registers.begin(), reg);
        }
    }
    
    return registers;
}

void Translator::translate()
{
    // Translate first the instructions for the .data
    for(T_Instruction* currentInstr : m_data_instructions)
    {
        translateInstruction(*currentInstr);
    }

    // Add global temporals
    for(string temporal : m_graph->globals)
    {
        if(data_statics.find(temporal) != data_statics.end())
            continue;
        
        insertVariable(temporal);
        string temp_type = m_graph->temps_size[temporal] == 1 ? "byte" : "word";
        m_data.emplace_back(temporal + decl + mips_instructions.at(temp_type) + space + "1");
    }

    vector<FlowNode*> nodes = m_graph->getOrderedBlocks();

    // Then tranlate the code
    for(FlowNode* currentNode : nodes)
    {
        // Get block size
        current_size = currentNode->function_size;

        // If the functions section starts
        if(currentNode->is_function && !function_section)
        {
            function_section = true;
            m_text.emplace_back("\n# *===== Functions Section =====*\n");
        }
        
        // Name of the block
        m_text.emplace_back(currentNode->getName() + decl);

        // Foreword
        if(currentNode->is_function)
        {
            m_text.emplace_back("# ===== Foreword =====");

            m_text.emplace_back(mips_instructions.at("store") + space + "$fp" + sep + "0($sp)");
            m_text.emplace_back(mips_instructions.at("store") + space + "$ra" + sep + "8($sp)");
            m_text.emplace_back("move  $fp, $sp");
            
            // Save the call parameters
            uint64_t size = current_size + 12;
            m_text.emplace_back("addi  $sp, $sp, " + to_string(size));

            // Update BASE and STACK
            m_text.emplace_back("addi  $a0, $fp, 12");
            m_text.emplace_back(mips_instructions.at("store") + space + "$a0" + sep + "BASE");
            m_text.emplace_back(mips_instructions.at("store") + space + "$sp" + sep + "STACK");
            
            m_text.emplace_back("# ====================");
        }

        // Translate instructions
        for(T_Instruction current_inst : currentNode->block)
        {
            m_text.push_back("# " + to_string(current_inst));
            translateInstruction(current_inst);
        }

        string lastInstr = "";
            
        // If the last instruction is a jump, update before jumping
        string lastInstrId = currentNode->block.back().id;
        if(lastInstrId == "goto" || lastInstrId== "goif" ||
            lastInstrId == "goifnot" || lastInstrId == "call" ||
            lastInstrId== "return")
        {
            lastInstr = m_text.back();
            m_text.pop_back();
        }

        // At the end of every basic block, update the temporals
        bool aliveVars = false;

        for(auto current_variable : m_variables)
        {
            string var_id = current_variable.first;
            vector<string> var_descriptor = current_variable.second;
            
            if ( find(var_descriptor.begin(), var_descriptor.end(), var_id) == var_descriptor.end() )
            {
                if(!aliveVars)
                {
                    aliveVars = true;
                    m_text.emplace_back("# ===== Updating temporals =====");
                }
                
                if(!is_static(var_id))
                    storeTemporal(var_id, var_descriptor[0], true);
            }
            
            m_variables[var_id].clear();
            m_variables[var_id].push_back(var_id);
        }

        if(aliveVars)
            m_text.emplace_back("# ==============================");

        if(!lastInstr.empty())
        {
            m_text.push_back(lastInstr);
        }

        // Clean the registers of values
        cleanRegistersDescriptor();
    }
}

void Translator::translateInstruction(T_Instruction instruction)
{
    // Check if the instruction is a meta instruction to process
    if(instruction.id[0] == '@')
    {
        translateMetaIntruction(instruction);
        return;
    }

    // Finish the program
    if(instruction.id == "exit")
    {
        loadTemporal(instruction.result.name, "$a0");
        m_text.emplace_back(mips_instructions.at(instruction.id));
        return;
    }

    // I/O Instructions
    if(instruction.id.find("print") != string::npos || instruction.id.find("read") != string::npos)
    {
        translateIOIntruction(instruction);
        return;
    }

    // Memory management instructions
    if(instruction.id == "malloc")
    {
        insertVariable(instruction.result.name);
        vector<string> op_registers = getReg(instruction);

        vector<string> reg_descriptor = getRegisterDescriptor(op_registers[1], m_registers);
        if ( find(reg_descriptor.begin(), reg_descriptor.end(), instruction.operands[0].name) == reg_descriptor.end() )
        {
            loadTemporal(instruction.operands[0].name, op_registers[1]);
        }

        // The size of the memory needed to be allocated should be in $a0
        // First, save whatever value is in the register
        vector<string> regDescriptor = getRegisterDescriptor("$a0", m_registers);
        for(string currentVar : regDescriptor)
        {
            if(is_number(currentVar))
                continue;

            if(!is_static(currentVar))
                storeTemporal(currentVar, "$a0");
            else
                availability(currentVar, currentVar);
        }

        m_registers["$a0"].clear();
        removeElementFromDescriptors(m_variables, "$a0", "");

        // Move the value to $a0
        m_text.emplace_back(mips_instructions.at("assign") + space + "$a0" + sep + op_registers[1]);

        // Call the syscall
        m_text.emplace_back(mips_instructions.at(instruction.id));

        // Save the direction in the temporal
        m_text.emplace_back(mips_instructions.at("assign") + space + op_registers[0] + sep + "$v0");

        // Maintain descriptors
        assignment(op_registers[0], instruction.result.name, m_registers);
        availability(instruction.result.name, op_registers[0], true);
        return;
    }

    if(instruction.id == "memcpy")
    {
        vector<string> op_registers = getReg(instruction);

        // We need some temporal registers to use, so we store some
        vector<string> regDescriptor = getRegisterDescriptor("$v1", m_registers);
        for(string currentVar : regDescriptor)
        {
            if(is_number(currentVar))
                continue;
            
            if(!is_static(currentVar))
                storeTemporal(currentVar, "$v1");
            else
                availability(currentVar, currentVar);
        }

        m_registers["$v1"].clear();
        removeElementFromDescriptors(m_variables, "$v1", "");

        // Load temporals if necessary
        vector<string> reg_descriptor = getRegisterDescriptor(op_registers[0], m_registers);
        if ( find(reg_descriptor.begin(), reg_descriptor.end(), instruction.result.name) == reg_descriptor.end() )
        {
            loadTemporal(instruction.result.name, op_registers[0]);
        }

        for(size_t i = 1; i < op_registers.size(); i++)
        {
            vector<string> reg_descriptor = getRegisterDescriptor(op_registers[i], m_registers);
            if ( find(reg_descriptor.begin(), reg_descriptor.end(), instruction.operands[i-1].name) == reg_descriptor.end() )
            {
                loadTemporal(instruction.operands[i-1].name, op_registers[i]);
            }
        }

        // Generate the new labels
        string label_init = "MC" + to_string(last_label_id);
        string label_end = "MC" + to_string(last_label_id) + "_END";
        m_text.emplace_back(label_init + decl);

        // Loop byte by byte, copying from source to destination
        m_text.emplace_back(mips_instructions.at("goifnot") + space + op_registers[2] + sep + label_end);
        m_text.emplace_back(mips_instructions.at("addi") + space + op_registers[2] + sep + op_registers[2] + sep + "-1");
        m_text.emplace_back(mips_instructions.at("add") + space + "$v0" + sep + op_registers[1] + sep + op_registers[2]);
        m_text.emplace_back(mips_instructions.at("loadb") + space + "$v0" + sep + "0($v0)");
        m_text.emplace_back(mips_instructions.at("add") + space + "$v1" + sep + op_registers[0] + sep + op_registers[2]);
        m_text.emplace_back(mips_instructions.at("storeb") + space + "$v0" + sep + "0($v1)");
        m_text.emplace_back(mips_instructions.at("goto") + space + label_init);
        
        m_text.emplace_back(label_end + decl);
        last_label_id++;

        return;
    }

    if(instruction.id == "free")
    {
        // BIG DOUBT ABOUT THIS ONE

        vector<string> op_registers = getReg(instruction);

        // The size of the memory needed to be allocated should be in $a0
        // First, save whatever value is in the register
        vector<string> regDescriptor = getRegisterDescriptor("$a0", m_registers);
        for(string currentVar : regDescriptor)
        {
            if(is_number(currentVar))
                continue;
            
            if(!is_static(currentVar))
                storeTemporal(currentVar, "$a0");
            else
                availability(currentVar, currentVar);
        }

        m_registers["$a0"].clear();
        removeElementFromDescriptors(m_variables, "$a0", "");

        // Move the value to shrink to $a0
        m_text.emplace_back(mips_instructions.at("assign") + space + "$a0" + sep + op_registers[0]);
        m_text.emplace_back(mips_instructions.at("minus") + space + "$a0" + sep + "$a0");

        // Call the syscall
        m_text.emplace_back(mips_instructions.at(instruction.id));
        return;
    }

    // Branching instructions
    if(instruction.id.find("go") != string::npos)
    {
        if(instruction.id == "goto")
        {
            if(instruction.id.find("_out") != std::string::npos)
                return;
            
            m_text.emplace_back(mips_instructions.at(instruction.id) + space + instruction.result.name);
        }
        else
        {
            vector<string> reg = getReg(instruction);
            unordered_map<string, vector<string>>* curr_desc = &m_registers;

            if(instruction.operands[0].name.front() == 'f' || instruction.operands[0].name.front() == 'F')
                curr_desc = &m_float_registers;

            vector<string> reg_descriptor = getRegisterDescriptor(reg[0], *curr_desc);
            if ( find(reg_descriptor.begin(), reg_descriptor.end(), instruction.operands[0].name) == reg_descriptor.end() )
            {
                loadTemporal(instruction.operands[0].name, reg[0]);
            }

            m_text.emplace_back(mips_instructions.at(instruction.id) + space + reg[0] + sep + instruction.result.name);
        }
        return;
    }

    // Function calls instructions
    if(instruction.id == "param")
    {
        m_text.emplace_back("# ===== Parameter =====");

        // Create param variable
        insertVariable(instruction.result.name);

        // Get the register
        vector<string> reg = getReg(instruction);

        // References
        unordered_map<string, vector<string>>* curr_desc = &m_registers;

        // If the operand is a float change the references
        if(instruction.result.name.front() == 'f' || instruction.result.name.front() == 'F')
            curr_desc = &m_float_registers;

        // Save where the parameter is going to be
        int jump_size = stoi(instruction.operands[0].name) + 12;
        m_text.emplace_back(mips_instructions.at("loada") + space + reg[0] + sep + to_string(jump_size) +"($sp)");
        m_text.emplace_back("# =====================");

        // Maintain descriptors
        assignment(reg[0], instruction.result.name, *curr_desc);
        availability(instruction.result.name, reg[0], true);


        return;
    }

    if(instruction.id == "call")
    {   
        // Create the variable where the return value is going to be store
        insertVariable(instruction.result.name);

        // Get register
        vector<string> regs = getReg(instruction);

        // Save all the temporals that were used
        for(auto current_variable : m_variables)
        {
            string var_id = current_variable.first;
            vector<string> var_descriptor = current_variable.second;
            
            if ( find(var_descriptor.begin(), var_descriptor.end(), var_id) == var_descriptor.end() )
            {
                if(!is_static(var_id))
                    storeTemporal(var_id, var_descriptor[0], true);
                else
                    availability(var_id, var_id);
                
                removeElementFromDescriptors(m_registers, var_id, "");
            }
        }

        // Jump to the function
        m_text.emplace_back(mips_instructions.at(instruction.id) + space + instruction.operands[0].name);

        // Save return value
        m_text.emplace_back(mips_instructions.at("load") + space + regs[0] + sep + "4($sp)");
        storeTemporal(instruction.result.name, regs[0]);

        return;
    }

    if(instruction.id == "return")
    {
        // Get the return register
        vector<string> reg = getReg(instruction);

        // References
        unordered_map<string, vector<string>>* curr_desc = &m_registers;
        string store_id = m_graph->temps_size[instruction.result.name] == 1 ? "storeb" : "store";

        // If the operand is a float change the references
        if(instruction.result.name.front() == 'f' || instruction.result.name.front() == 'F')
        {
            curr_desc = &m_float_registers;
            store_id = "fstore";
        }

        // If the element is not on the register load it
        vector<string> reg_descriptor = getRegisterDescriptor(reg[0], *curr_desc);
        if ( find(reg_descriptor.begin(), reg_descriptor.end(), instruction.result.name) == reg_descriptor.end() )
        {            
            loadTemporal(instruction.result.name, reg[0]);
        }

        m_text.emplace_back("# ===== Epilogue =====");

        m_text.emplace_back(mips_instructions.at("assign") + space + "$sp" + sep + "$fp");
        m_text.emplace_back(mips_instructions.at("load") + space + "$ra" + sep + "8($fp)");

        // Store the return value
        m_text.emplace_back(mips_instructions.at(store_id) + space + reg[0] + sep + "4($fp)");
        
        // Restore the old frame and the return address
        m_text.emplace_back(mips_instructions.at("load") + space + "$fp" + sep + "0($fp)");

        // Update BASE and STACK
        m_text.emplace_back("addi  $a0, $fp, 12");
        m_text.emplace_back(mips_instructions.at("store") + space + "$a0" + sep + "BASE");
        m_text.emplace_back(mips_instructions.at("store") + space + "$sp" + sep + "STACK");

        // Jump back to the caller
        m_text.emplace_back(mips_instructions.at(instruction.id) + space + "$ra");
        m_text.emplace_back("# ====================");
        m_text.emplace_back(""); // just to fix some alignments

        return;
    }

    // Operations instructions
    bool is_copy = instruction.id.find("assign") != string::npos ? true : false;
    translateOperationInstruction(instruction, is_copy);
}

void Translator::translateOperationInstruction(T_Instruction instruction, bool is_copy)
{
    // Try to create the variable descriptor
    insertVariable(instruction.result.name);

    // Choose the registers to use
    vector<string> op_registers = getReg(instruction, is_copy);
    int op_index = 1;
    
    // References needed
    unordered_map<string, vector<string>>* regs_to_find = &m_registers;

    for (T_Variable current_operand : instruction.operands)
    {
        if(current_operand.name.empty())
            continue;
        
        // Check if the operand is in the registers, if not then load it
        string current_reg = op_registers[op_index];

        if(current_operand.name.front() == 'f' || current_operand.name.front() == 'F')
            regs_to_find = &m_float_registers;
        else
            regs_to_find = &m_registers;

        vector<string> reg_descriptor = getRegisterDescriptor(current_reg, *regs_to_find);
        if ( find(reg_descriptor.begin(), reg_descriptor.end(), current_operand.name) == reg_descriptor.end() )
        {
            loadTemporal(current_operand.name, current_reg);
        }

        if(is_copy && !instruction.result.is_acc && !current_operand.is_acc)
        {
            assignment(current_reg, instruction.result.name, *regs_to_find);
            availability(instruction.result.name, current_reg, true);
        }

        op_index++; 
    }
    
    // Reset descriptors reference
    regs_to_find = &m_registers;

    if(is_copy)
    {
        // Take into account indirections
        if(instruction.operands[0].is_acc)
        {
            string load_result = instruction.id.back() == 'b' ? "loadb" : "load";

            // Check if is a float
            if(instruction.result.name.front() == 'f' || instruction.result.name.front() == 'F')
            {
                regs_to_find = &m_float_registers;

                if(!is_number(instruction.operands[0].acc))
                {
                    loadTemporal(instruction.operands[0].acc, "$v0", false);
                    m_text.emplace_back(mips_instructions.at("add") + space + "$v0" + sep + op_registers[1] + sep + "$v0");
                    m_text.emplace_back(mips_instructions.at("fload") + space + op_registers[0] + sep + "($v0)");
                }
                else
                {
                    string op = instruction.operands[0].acc + "(" + op_registers[1] + ")";
                    m_text.emplace_back(mips_instructions.at("fload") + space + op_registers[0] + sep + op);
                }
            }
            else
            {
                if(!is_number(instruction.operands[0].acc))
                {
                    string acc_reg = findElementInDescriptors(m_registers, instruction.operands[0].acc);
                    if(acc_reg.empty())
                    {
                        loadTemporal(instruction.operands[0].acc, op_registers[0], false);
                        acc_reg = op_registers[0];
                    }

                    m_text.emplace_back(mips_instructions.at("add") + space + op_registers[0] + sep + acc_reg + sep + op_registers[1]);

                    m_text.emplace_back(mips_instructions.at(load_result) + space + op_registers[0] + sep + "0(" + op_registers[0] + ")");
                }
                else
                {
                    string op = instruction.operands[0].acc + "(" + op_registers[1] + ")";
                    m_text.emplace_back(mips_instructions.at(load_result) + space + op_registers[0] + sep + op);
                }
            }

            // Maintain descriptors
            assignment(op_registers[0], instruction.result.name, *regs_to_find);
            availability(instruction.result.name, op_registers[0], true);
        }
        else if(instruction.result.is_acc)
        {
            string store_id = instruction.id.back() == 'b' ? "storeb" : "store";

            // Check if result register is a float
            if(instruction.result.name.front() == 'f' || instruction.result.name.front() == 'F')
            {
                regs_to_find = &m_float_registers;
                store_id = "fstore";
            }

            // Load the base direction of what we want to make an indirection
            vector<string> reg_descriptor = getRegisterDescriptor(op_registers[0], *regs_to_find);
            if ( find(reg_descriptor.begin(), reg_descriptor.end(), instruction.result.name) == reg_descriptor.end() )
            {
                loadTemporal(instruction.result.name, op_registers[0], false);
            }

            // Maintain descriptors
            assignment(op_registers[0], instruction.result.name, *regs_to_find);
            availability(instruction.result.name, op_registers[0], true);
            
            string op = instruction.result.acc + "(" + op_registers[0] + ")";

            if(!is_number(instruction.result.acc))
            {
                string acc_reg = findElementInDescriptors(m_registers, instruction.operands[0].acc);
                if(acc_reg.empty())
                {
                    loadTemporal(instruction.result.acc, "$v0", false);
                    acc_reg = "$v0";
                }
                m_text.emplace_back(mips_instructions.at("add") + space + "$v0" + sep + op_registers[0] + sep + acc_reg);
                op = "0($v0)";
            }

            m_text.emplace_back(mips_instructions.at(store_id) + space + op_registers[1] + sep + op);
        }
        return;
    }

    // Emit code depending on the operator
    int i = 0;
    if(instruction.id == "div" || instruction.id == "mod")
        i = 1;
    
    // Check if result register is a float
    if(instruction.result.name.front() == 'f' || instruction.result.name.front() == 'F')
    {
        regs_to_find = &m_float_registers;
        instruction.id = "f" + instruction.id;
    }
    
    string emit = mips_instructions.at(instruction.id) + space;
    for (;i < (int) op_registers.size(); i++)
    {
        if(i == (int) op_registers.size() - 1)
        {
            emit += op_registers[i];
            continue;
        }

        emit += op_registers[i] + sep;
    }
    m_text.emplace_back(emit);

    // If is div or mod add the special MIPS instructions
    if(instruction.id == "div")
        m_text.emplace_back(mips_instructions.at("low") + space + op_registers[0]);
    else if(instruction.id == "mod")
        m_text.emplace_back(mips_instructions.at("high") + space + op_registers[0]);

    // Maintain descriptor
    assignment(op_registers[0], instruction.result.name, *regs_to_find, true);
    availability(instruction.result.name, op_registers[0], true);
    removeElementFromDescriptors(m_variables, op_registers[0], instruction.result.name);
    removeElementFromDescriptors(*regs_to_find, instruction.result.name, op_registers[0]);

    // cout << "== " << instruction.id << " " << instruction.result.name << endl;
    // printVariablesDescriptors();
}

void Translator::translateMetaIntruction(T_Instruction instruction)
{
    if(instruction.id == "@string")
    {
        insertVariable(instruction.result.name);
        data_statics.emplace(instruction.result.name);
        m_data.emplace_back(".align 2");
        m_data.emplace_back(instruction.result.name + decl + mips_instructions.at(instruction.id) + space + instruction.operands[0].name);
        return;
    }
    
    if(instruction.id == "@staticv")
    {
        insertVariable(instruction.result.name);
        data_statics.emplace(instruction.result.name);
        m_data.emplace_back(instruction.result.name + decl + mips_instructions.at("word") + space + instruction.operands[0].name);
        return;
    }
}

void Translator::translateIOIntruction(T_Instruction instruction)
{
    if(instruction.id.find("print") != string::npos)
    {
        const char* arg_register = "$a0";
        if(instruction.id.back() != 'f')
        {
            // Is need to have the element to print in $a0, store the elements if needed
            vector<string> reg_descriptor = getRegisterDescriptor(arg_register, m_registers);
            for(string currentVar : reg_descriptor)
            {
                if(is_number(currentVar))
                    continue;
                if(!is_static(currentVar))
                    storeTemporal(currentVar, arg_register);
                else
                    availability(currentVar, currentVar);
            }
            m_registers[arg_register].clear();
            removeElementFromDescriptors(m_variables, arg_register, "");

            string curr_reg = findElementInDescriptors(m_registers, instruction.result.name);

            // Move the element to $a0
            if(!curr_reg.empty())
                m_text.emplace_back(mips_instructions.at("assign") + space + arg_register + sep + curr_reg);
            else
                loadTemporal(instruction.result.name, arg_register);
            
            // Load the correct syscall
            m_text.emplace_back(mips_instructions.at(instruction.id));
        }
        else
        {
            arg_register = "$f12";
            // Is need to have the element to print in $f12, store the elements if needed
            vector<string> regDescriptor = getRegisterDescriptor(arg_register, m_float_registers);
            for(string currentVar : regDescriptor)
            {
                if(is_number(currentVar))
                    continue;
                
                if(!is_static(currentVar))
                    storeTemporal(currentVar, arg_register);
                else
                    availability(currentVar, currentVar);
            }
            m_float_registers[arg_register].clear();
            removeElementFromDescriptors(m_variables, arg_register, "");

            // Move the element to $f12
            loadTemporal(instruction.result.name, arg_register);
            // Load the correct syscall
            m_text.emplace_back(mips_instructions.at(instruction.id));
        }
    }
    else    
    {
        if(instruction.id.back() == 'i' || instruction.id.back() == 'c')
        {
            // Create temporal where is going to be stored
            insertVariable(instruction.result.name);

            // Get register
            vector<string> regs = getReg(instruction);

            // Load correct syscall
            m_text.emplace_back(mips_instructions.at(instruction.id));

            // Store read value
            m_text.emplace_back(mips_instructions.at("assign") + space + regs[0] + sep + "$v0");
            storeTemporal(instruction.result.name, regs[0], true);
        }
        else if(instruction.id.back() == 'f')
        {
            // Create temporal where is going to be stored
            insertVariable(instruction.result.name);

            // Result is going to be store in $v0
            vector<string> regDescriptor = getRegisterDescriptor("$f12", m_float_registers);
            for(string currentVar : regDescriptor)
            {
                if(is_number(currentVar))
                    continue;
                
                if(!is_static(currentVar))
                    storeTemporal(currentVar, "f12");
                else
                    availability(currentVar, currentVar);
            }
            m_float_registers["$f12"].clear();
            removeElementFromDescriptors(m_variables, "$f12", "");

            // Load correct syscall
            m_text.emplace_back(mips_instructions.at(instruction.id));

            // Store read value
            storeTemporal(instruction.result.name, "$f12");
        }
        else
        {
            const char* addr_register = "$a0";
            const char* size_register = "$a1";

            // Is neccesary the buffer where the string is going to be loaded in $a0
            vector<string> regDescriptor = getRegisterDescriptor(addr_register, m_registers);
            for(string currentVar : regDescriptor)
            {
                if(is_number(currentVar))
                    continue;
                
                if(!is_static(currentVar))
                    storeTemporal(currentVar, addr_register);
                else
                    availability(currentVar, currentVar);
            }
            m_registers[addr_register].clear();
            removeElementFromDescriptors(m_variables, addr_register, "");

            // Max size of the string in $a1
            regDescriptor = getRegisterDescriptor(size_register, m_registers);
            for(string currentVar : regDescriptor)
            {
                if(is_number(currentVar))
                    continue;
                
                if(!is_static(currentVar))
                    storeTemporal(currentVar, size_register);
                else
                    availability(currentVar, currentVar);
            }
            m_registers[size_register].clear();
            removeElementFromDescriptors(m_variables, size_register, "");

            // Move buffer to $a0
            vector<string> regs = getReg(instruction);

            vector<string> reg_descriptor = getRegisterDescriptor(regs[0], m_registers);
            if ( find(reg_descriptor.begin(), reg_descriptor.end(), instruction.result.name) == reg_descriptor.end() )
            {
                loadTemporal(instruction.result.name, regs[0]);
            }

            m_text.emplace_back(mips_instructions.at("assign") + space + addr_register + sep + regs[0]);
            // Add size of what is going to be read
            m_text.emplace_back(mips_instructions.at("loadi") + space + size_register + sep + "99999");

            // Load correct syscall
            m_text.emplace_back(mips_instructions.at(instruction.id));
        }
    }
}

void Translator::printVariablesDescriptors()
{
    for(auto var_data : m_variables)
    {
        cout << var_data.first << " => [";
        for(auto element : var_data.second)
        {
            cout << element << " ";
        }
        cout<< "]" << endl;
    }
}

bool Translator::is_number(const string& str)
{
    for(size_t i = 0; i < str.size(); i++)
    {
        if(str[i] == '-' || str[i] == '.')
            continue;
        
        if(!isdigit(str[i]))
            return false;
    }

    return true;
}

bool Translator::is_static(const string& id)
{
    if(data_statics.find(id) != data_statics.end())
        return true;
    
    return false;
}

bool Translator::is_global(const string& id)
{
    if(m_graph->globals.find(id) != m_graph->globals.end())
        return true;
    
    return false;
}
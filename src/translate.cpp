#include "translate.hpp"

Translator::Translator()
{
    // Add MIPS registers
    //insertRegister("$v0");
    insertRegister("$v1");

    int i = 0;
    while (i < 3)
    {   
        insertRegister("$a" + to_string(i));
        i++;
    }

    i = 0;
    
    while (i < 10)
    {   
        insertRegister("$t" + to_string(i));
        i++;
    }

    i = 0;
    
    while (i < 8)
    {   
        insertRegister("$s" + to_string(i));
        i++;
    }

    // Add the float registers
    i = 0;
    
    while (i < 32)
    {   
        insertFloatRegister("$f" + to_string(i));
        i++;
    }

    // Add constant variables
    insertVariable("BASE", 0);
    insertVariable("STACK", 0);

    // Oh no...
    text.emplace_back("li  $sp, 0x7fc00000");
    text.emplace_back("li  $fp, 0x7fc00000");
}

void Translator::insertInstruction(T_Instruction* instruction)
{
    m_data_instructions.push_back(instruction);
}

bool Translator::insertRegister(string id)
{
    auto found = this->m_registers.find(id);

    if(found != this->m_registers.end()) 
        return false;
    
    this->m_registers[id] = vector<string>();
    return true;
}

bool Translator::insertFloatRegister(string id)
{
    auto found = this->m_float_registers.find(id);

    if(found != this->m_float_registers.end()) 
        return false;
    
    this->m_float_registers[id] = vector<string>();
    return true;
}

bool Translator::insertVariable(string id, uint32_t type, string value)
{
    auto found = this->m_variables.find(id);

    if(found != this->m_variables.end()) 
        return false;
    
    // Add the variable to the .data
    string s_type = "word";

    switch (type)
    {
    case 1:
        s_type = "byte";
        break;
    case 3:
        s_type = "@string";
        break;
    default:
        break;
    }

    if(id.front() == 'f' or id.front() == 'F')
    {
        s_type = "float";
        type = 2;
    }

    data.emplace_back(id + decl + mips_instructions.at(s_type) + space + value);

    // Insert tag
    this->m_tags[id] = type;
    
    // Insert descriptor
    vector<string> locations { id };
    this->m_variables[id] = locations;
    return true;
}

void Translator::insertFlowGraph(FlowGraph* graph)
{
    m_graph = graph;
}

void Translator::print()
{
    cout << ".text" << endl;
    for(string inst : text)
    {
        cout << inst << endl;
    }

    // if(functions.size() > 0)
    // {
    //     cout << "\n# ===== Functions Section ====="<< endl;
    //     for(string inst : functions)
    //     {
    //         cout << inst << endl;
    //     }
    // }

    cout << "\n.data" << endl;
    for(string inst : data)
    {
        cout << inst << endl;
    }
}

bool Translator::insertElementToDescriptor(unordered_map<string, vector<string>> &descriptors, string key, string element, bool replace)
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

void Translator::removeElementFromDescriptors(unordered_map<string, vector<string>> &descriptors, string element, string current_container)
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
    for(auto current_register : this->m_registers)
    {
        this->m_registers[current_register.first].clear();
    }
}

bool Translator::assignment(string register_id, string variable_id, unordered_map<string, vector<string>>& curr_registers, 
                            bool replace)
{
    return insertElementToDescriptor(curr_registers, register_id, variable_id, replace);
}

bool Translator::availability(string variable_id, string location, bool replace)
{
    return insertElementToDescriptor(this->m_variables, variable_id, location, replace);
}

vector<string> Translator::getRegisterDescriptor(string id, unordered_map<string, vector<string>>& curr_registers)
{
    return curr_registers[id];
}

vector<string> Translator::getVariableDescriptor(string id)
{
    return this->m_variables[id];
}

string Translator::findOptimalLocation(string id)
{    
    //vector<string> descriptor = getVariableDescriptor(id);
    // For now just return itself
    return id;
}

string Translator::findElementInDescriptors(unordered_map<string, vector<string>> &descriptors, string element)
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
                                    vector<string>& section, vector<string> &regs)
{
    // Traverse all the register and check which one is the most viable
    // for recycling and using it

    string best_reg = "";

    // Counter of spills
    map<string, int> spills;
    unordered_map<string, vector<pair<string, string>>> spills_emit;
    
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
            if(this->m_variables[element].size() > 1)
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
            string store_id = m_tags[element] == 1 ? "storeb" : "store";

            if(m_tags[element] == 2)
                store_id = "fstore";

            // Add emit for the possible spill
            spills_emit[current_register.first].push_back(make_pair(store_id, element));
            
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

    for(auto reg_info : spills_emit[best_reg])
    {
        string store_id = reg_info.first;
        string element = reg_info.second;

        // If static variable ignore
        if(m_tags[element] != 3 || m_tags[element] != 4)
            section.emplace_back(mips_instructions.at(store_id) + space + best_reg + sep + element);

        // Maintain the descriptors
        availability(element, element);
    }

    getRegisterDescriptor(best_reg, descriptors).clear();
    removeElementFromDescriptors(this->m_variables, best_reg, "");

    return best_reg;
}

void Translator::selectRegister(string operand, T_Instruction instruction, unordered_map<string, vector<string>>& descriptors,
                                vector<string> &regs, vector<string> &free_regs, vector<string>& section)
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
    reg = recycleRegister(instruction, descriptors, section, regs);
    regs.push_back(reg);
}

vector<string> Translator::getReg(T_Instruction instruction, vector<string>& section, bool is_copy)
{
    vector<string> registers;

    // Look for the registers that are available, depending if float registers or normal
    vector<string> free_r = findFreeRegister(this->m_registers);
    vector<string> free_fr = findFreeRegister(this->m_float_registers);

    // References necessaries depending if the operand is a float
    unordered_map<string, vector<string>>* curr_desc = &this->m_registers;
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
            curr_desc = &this->m_float_registers;
        }
        else
        {
            free_regs = &free_r;
            curr_desc = &this->m_registers;
        }

        selectRegister(current_operand.name, instruction, *curr_desc, registers, *free_regs, section);
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
            curr_desc = &this->m_float_registers;
        }
        else
        {
            free_regs = &free_r;
            curr_desc = &this->m_registers;
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
            selectRegister(instruction.result.name, instruction, *curr_desc, registers, *free_regs, section);
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
        translateInstruction(*currentInstr, text);
    }

    vector<FlowNode*> nodes = m_graph->getOrderedBlocks();

    // Then tranlate the code
    for(FlowNode* currentNode : nodes)
    {
        // Get block size
        current_size = currentNode->function_size;

        // Choose in which section to put the current instruction
        vector<string>& section = text;

        // If the functions section starts
        if(currentNode->is_function && !function_section)
        {
            function_section = true;
            section.emplace_back("\n# *===== Functions Section =====*\n");
        }
        
        // Name of the block
        section.emplace_back(currentNode->getName() + decl);

        // Foreword
        if(currentNode->is_function)
        {
            section.emplace_back("# ===== Foreword =====");

            section.emplace_back(mips_instructions.at("store") + space + "$fp" + sep + "0($sp)");
            section.emplace_back(mips_instructions.at("store") + space + "$ra" + sep + "8($sp)");
            section.emplace_back("move  $fp, $sp");
            // section.emplace_back("addi  $sp, $sp, 12");
            
            // Save the call parameters
            uint64_t size = current_size + 12;
            section.emplace_back("addi  $sp, $sp, " + to_string(size));

            // Update BASE and STACK
            section.emplace_back("addi  $a0, $fp, 12");
            section.emplace_back(mips_instructions.at("store") + space + "$a0" + sep + "BASE");
            section.emplace_back(mips_instructions.at("store") + space + "$sp" + sep + "STACK");
            
            section.emplace_back("# ====================");
        }

        // Translate instructions
        for(T_Instruction current_inst : currentNode->block)
        {
            text.push_back("# " + to_string(current_inst));
            translateInstruction(current_inst, section);
        }

        vector<string> aliveVars;

        // At the end of every basic block, update the temporals
        for(auto current_variable : this->m_variables)
        {
            string var_id = current_variable.first;
            vector<string> var_descriptor = current_variable.second;
            
            string store_id = m_tags[var_id] == 1 ? "storeb" : "store";

            if(m_tags[var_id] == 2)
                store_id = "fstore";
            
            if ( find(var_descriptor.begin(), var_descriptor.end(), var_id) == var_descriptor.end() )
            {
                // If static variable ignore
                if(m_tags[var_id] != 3 && m_tags[var_id] != 4)
                    aliveVars.emplace_back(mips_instructions.at(store_id) + space + var_descriptor[0] + sep + var_id);
                availability(var_id, var_id, true);
            }
        }

        if(aliveVars.size() > 0)
        {
            string lastInstr = "";
            
            // If the last instruction is a jump, update before jumping
            string lastInstrId = currentNode->block.back().id;
            if(lastInstrId == "goto" || lastInstrId== "goif" ||
                lastInstrId == "goifnot" || lastInstrId == "call" ||
                lastInstrId== "return")
            {
                lastInstr = section.back();
                section.pop_back();
            }

            section.emplace_back("# ===== Updating temporals =====");

            for(string line : aliveVars)
            {
                section.push_back(line);
            }
            
            section.emplace_back("# ==============================");

            if(!lastInstr.empty())
            {
                section.push_back(lastInstr);
            }
        }

        // Clean the registers of values
        cleanRegistersDescriptor();
    }
}

void Translator::translateInstruction(T_Instruction instruction, vector<string>& section)
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
        string load_id = is_number(instruction.result.name) ? "loadi" : "load";

        section.emplace_back(mips_instructions.at(load_id) + space + "$a0" + sep + instruction.result.name);
        section.emplace_back(mips_instructions.at(instruction.id));
        return;
    }

    // I/O Instructions
    if(instruction.id.find("print") != string::npos || instruction.id.find("read") != string::npos)
    {
        translateIOIntruction(instruction, section);
        return;
    }

    // Memory management instructions
    if(instruction.id == "malloc")
    {
        insertVariable(instruction.result.name, 0);
        vector<string> op_registers = getReg(instruction, section);

        string load_id = is_number(instruction.operands[0].name) ? "loadi" : "load";

        vector<string> reg_descriptor = getRegisterDescriptor(op_registers[1], this->m_registers);
        if ( find(reg_descriptor.begin(), reg_descriptor.end(), instruction.operands[0].name) == reg_descriptor.end() )
        {
            string best_location = findOptimalLocation(instruction.operands[0].name);
            section.emplace_back(mips_instructions.at(load_id) + space + op_registers[1] + sep + best_location);
        }

        // The size of the memory needed to be allocated should be in $a0
        // First, save whatever value is in the register
        vector<string> regDescriptor = getRegisterDescriptor("$a0", this->m_registers);
        for(string currentVar : regDescriptor)
        {
            if(is_number(currentVar))
                continue;
            
            // If static variable ignore
            if(m_tags[currentVar] != 3 && m_tags[currentVar] != 4)
                section.emplace_back(mips_instructions.at("store") + space + "$a0" + sep + currentVar);
            
            availability(currentVar, currentVar);
        }

        regDescriptor.clear();
        removeElementFromDescriptors(this->m_variables, "$a0", "");

        // Move the value to $a0
        section.emplace_back(mips_instructions.at("assign") + space + "$a0" + sep + op_registers[1]);

        // Call the syscall
        section.emplace_back(mips_instructions.at(instruction.id));

        // Save the direction in the temporal
        section.emplace_back(mips_instructions.at("assign") + space + op_registers[0] + sep + "$v0");

        // Maintain descriptors
        assignment(op_registers[0], instruction.result.name, this->m_registers);
        availability(instruction.result.name, op_registers[0], true);
        return;
    }

    if(instruction.id == "memcpy")
    {
        vector<string> op_registers = getReg(instruction, section, false);

        // We need some temporal registers to use, so we store some
        vector<string> regDescriptor = getRegisterDescriptor("$v1", this->m_registers);
        for(string currentVar : regDescriptor)
        {
            if(is_number(currentVar))
                continue;
            
            // If static variable ignore
            if(m_tags[currentVar] != 3 && m_tags[currentVar] != 4)
                section.emplace_back(mips_instructions.at("store") + space + "$v1" + sep + currentVar);
            
            availability(currentVar, currentVar);
        }

        regDescriptor.clear();
        removeElementFromDescriptors(this->m_variables, "$v1", "");

        // Load temporals if necessary
        vector<string> reg_descriptor = getRegisterDescriptor(op_registers[0], this->m_registers);
        if ( find(reg_descriptor.begin(), reg_descriptor.end(), instruction.result.name) == reg_descriptor.end() )
        {
            string best_location = findOptimalLocation(instruction.result.name);
            section.emplace_back(mips_instructions.at("load") + space + op_registers[0] + sep + best_location);
        }

        for(size_t i = 1; i < op_registers.size(); i++)
        {
            string load_id = "load";
            vector<string> reg_descriptor = getRegisterDescriptor(op_registers[i], this->m_registers);
            if ( find(reg_descriptor.begin(), reg_descriptor.end(), instruction.operands[i-1].name) == reg_descriptor.end() )
            {
                string best_location = findOptimalLocation(instruction.operands[i-1].name);

                if(is_number(best_location))
                    load_id = "loadi";

                section.emplace_back(mips_instructions.at(load_id) + space + op_registers[i] + sep + best_location);
            }
        }

        // Generate the new labels
        string label_init = "MC" + to_string(last_label_id);
        string label_end = "MC" + to_string(last_label_id) + "_END";
        section.emplace_back(label_init + decl);

        // Loop byte by byte, copying from source to destination
        section.emplace_back(mips_instructions.at("goifnot") + space + op_registers[2] + sep + label_end);
        section.emplace_back(mips_instructions.at("addi") + space + op_registers[2] + sep + op_registers[2] + sep + "-1");
        section.emplace_back(mips_instructions.at("add") + space + "$v0" + sep + op_registers[1] + sep + op_registers[2]);
        section.emplace_back(mips_instructions.at("loadb") + space + "$v0" + sep + "0($v0)");
        section.emplace_back(mips_instructions.at("add") + space + "$v1" + sep + op_registers[0] + sep + op_registers[2]);
        section.emplace_back(mips_instructions.at("storeb") + space + "$v0" + sep + "0($v1)");
        section.emplace_back(mips_instructions.at("goto") + space + label_init);
        
        section.emplace_back(label_end + decl);
        last_label_id++;

        return;
    }

    if(instruction.id == "free")
    {
        // BIG DOUBT ABOUT THIS ONE

        vector<string> op_registers = getReg(instruction, section, false);

        // The size of the memory needed to be allocated should be in $a0
        // First, save whatever value is in the register
        vector<string> regDescriptor = getRegisterDescriptor("$a0", this->m_registers);
        for(string currentVar : regDescriptor)
        {
            if(is_number(currentVar))
                continue;
            
            // If static variable ignore
            if(m_tags[currentVar] != 3 && m_tags[currentVar] != 4)
                section.emplace_back(mips_instructions.at("store") + space + "$a0" + sep + currentVar);
            
            availability(currentVar, currentVar);
        }

        regDescriptor.clear();
        removeElementFromDescriptors(this->m_variables, "$a0", "");

        // Move the value to shrink to $a0
        section.emplace_back(mips_instructions.at("assign") + space + "$a0" + sep + op_registers[0]);
        section.emplace_back(mips_instructions.at("minus") + space + "$a0" + sep + "$a0");

        // Call the syscall
        section.emplace_back(mips_instructions.at(instruction.id));
        return;
    }

    // Branching instructions
    if(instruction.id.find("go") != string::npos)
    {
        if(instruction.id == "goto")
        {
            if(instruction.id.find("_out") != std::string::npos)
                return;
            
            section.emplace_back(mips_instructions.at(instruction.id) + space + instruction.result.name);
        }
        else
        {
            vector<string> reg = getReg(instruction, section);
            unordered_map<string, vector<string>>* curr_desc = &this->m_registers;

            string load_id = this->m_tags[instruction.operands[0].name] == 1 ? "loadb" : "load";

            if(instruction.operands[0].name.front() == 'f' || instruction.operands[0].name.front() == 'F')
            {
                curr_desc = &this->m_float_registers;
                load_id = "fload";
            }

            vector<string> reg_descriptor = getRegisterDescriptor(reg[0], *curr_desc);
            if ( find(reg_descriptor.begin(), reg_descriptor.end(), instruction.operands[0].name) == reg_descriptor.end() )
            {
                string best_location = findOptimalLocation(instruction.operands[0].name);

                if(is_number(best_location))
                    load_id = "loadi";

                section.emplace_back(mips_instructions.at(load_id) + space + reg[0] + sep + best_location);
            }

            section.emplace_back(mips_instructions.at(instruction.id) + space + reg[0] + sep + instruction.result.name);
        }
        return;
    }

    // Function calls instructions
    if(instruction.id == "param")
    {
        section.emplace_back("# ===== Parameter =====");

        // Create param variable
        insertVariable(instruction.result.name, 0);

        // Get the register
        vector<string> reg = getReg(instruction, section);

        // References
        unordered_map<string, vector<string>>* curr_desc = &this->m_registers;
        string load_id = "loada";

        // If the operand is a float change the references
        if(instruction.result.name.front() == 'f' || instruction.result.name.front() == 'F')
        {
            curr_desc = &this->m_float_registers;
            load_id = "fload";
        }

        // If the element is not on the register load it
        vector<string> reg_descriptor = getRegisterDescriptor(reg[0], *curr_desc);
        if ( find(reg_descriptor.begin(), reg_descriptor.end(), instruction.result.name) == reg_descriptor.end() )
        {
            string best_location = findOptimalLocation(instruction.result.name);

            section.emplace_back(mips_instructions.at(load_id) + space + reg[0] + sep + best_location);
        }

        // Save where the parameter is going to be
        int jump_size = stoi(instruction.operands[0].name) + 12;
        section.emplace_back(mips_instructions.at(load_id) + space + "$v0" + sep + to_string(jump_size) +"($sp)");

        section.emplace_back(mips_instructions.at("store") + space + "$v0" + sep + "0(" + reg[0] + ")");

        section.emplace_back("# =====================");

        return;
    }

    if(instruction.id == "call")
    {   
        // Create the variable where the return value is going to be store
        insertVariable(instruction.result.name, 0);

        // Jump to the function
        section.emplace_back(mips_instructions.at(instruction.id) + space + instruction.operands[0].name);

        // Save return value
        section.emplace_back(mips_instructions.at("load") + space + "$v0" + sep + "4($sp)");
        section.emplace_back(mips_instructions.at("store") + space + "$v0" + sep + instruction.result.name);

        return;
    }

    if(instruction.id == "return")
    {
        // Get the return register
        vector<string> reg = getReg(instruction, section);

        // References
        unordered_map<string, vector<string>>* curr_desc = &this->m_registers;
        string load_id = this->m_tags[instruction.result.name] == 1 ? "loadb" : "load";
        string store_id = this->m_tags[instruction.result.name] == 1 ? "storeb" : "store";

        // If the operand is a float change the references
        if(instruction.result.name.front() == 'f' || instruction.result.name.front() == 'F')
        {
            curr_desc = &this->m_float_registers;
            load_id = "fload";
            store_id = "fstore";
        }

        // If the element is not on the register load it
        vector<string> reg_descriptor = getRegisterDescriptor(reg[0], *curr_desc);
        if ( find(reg_descriptor.begin(), reg_descriptor.end(), instruction.result.name) == reg_descriptor.end() )
        {
            string best_location = findOptimalLocation(instruction.result.name);

            if(is_number(instruction.result.name))
                load_id = "loadi";

            section.emplace_back(mips_instructions.at(load_id) + space + reg[0] + sep + best_location);
        }

        section.emplace_back("# ===== Epilogue =====");

        section.emplace_back(mips_instructions.at("assign") + space + "$sp" + sep + "$fp");
        section.emplace_back(mips_instructions.at("load") + space + "$ra" + sep + "8($fp)");

        // Store the return value
        section.emplace_back(mips_instructions.at(store_id) + space + reg[0] + sep + "4($fp)");
        
        // Restore the old frame and the return address
        section.emplace_back(mips_instructions.at("load") + space + "$fp" + sep + "0($fp)");

        // uint64_t size = current_size + 12;
        // section.emplace_back("addi  $sp, $sp, -" + to_string(size));

        // Update BASE and STACK
        section.emplace_back("addi  $a0, $fp, 12");
        section.emplace_back(mips_instructions.at("store") + space + "$a0" + sep + "BASE");
        section.emplace_back(mips_instructions.at("store") + space + "$sp" + sep + "STACK");

        // Jump back to the caller
        section.emplace_back(mips_instructions.at(instruction.id) + space + "$ra");
        section.emplace_back("# ====================");
        section.emplace_back(""); // just to fix some alignments

        return;
    }

    // TODO: Use function size

    // Operations instructions
    bool is_copy = false;
    int type = 1;

    if(instruction.id.find("assign") != string::npos)
    {
        is_copy = true;

        if(instruction.id.back() == 'w')
        {
            type = 0;
        }
    }
    else if(instruction.id == "add" || 
            instruction.id == "sub" ||
            instruction.id == "mult" ||
            instruction.id == "div" ||
            instruction.id == "mod" ||
            instruction.id == "minus"
        )
    {
        type = 0;
    }

    translateOperationInstruction(instruction, section, is_copy, type);
}

void Translator::translateOperationInstruction(T_Instruction instruction, vector<string>& section, bool is_copy, uint32_t type)
{
    // Try to create the variable descriptor
    insertVariable(instruction.result.name, type);

    // Choose the registers to use
    vector<string> op_registers = getReg(instruction, section, is_copy);
    int op_index = 1;
    
    // References needed
    unordered_map<string, vector<string>>* regs_to_find = &this->m_registers;
    string load_id = "load";

    for (T_Variable current_operand : instruction.operands)
    {
        if(current_operand.name.empty())
            continue;
        
        // Check if the operand is in the registers, if not then load it
        string current_reg = op_registers[op_index];

        if(current_operand.name.front() == 'f' || current_operand.name.front() == 'F')
        {
            regs_to_find = &this->m_float_registers;
            load_id = "fload";
        }
        else
        {
            regs_to_find = &this->m_registers;
            load_id = "load";
        }

        vector<string> reg_descriptor = getRegisterDescriptor(current_reg, *regs_to_find);
        if ( find(reg_descriptor.begin(), reg_descriptor.end(), current_operand.name) == reg_descriptor.end() )
        {
            string best_location = findOptimalLocation(current_operand.name);

            if(is_number(best_location))
                load_id = "loadi";
            else if(this->m_tags[current_operand.name] == 3 || this->m_tags[current_operand.name] == 4)
                load_id = "loada";

            section.emplace_back(mips_instructions.at(load_id) + space + current_reg + sep + best_location);
        }

        // Maintain descriptors
        assignment(current_reg, current_operand.name, *regs_to_find, true);
        availability(current_operand.name, current_reg);

        if(is_copy && !instruction.result.is_acc && !current_operand.is_acc)
        {
            assignment(current_reg, instruction.result.name, *regs_to_find);
            availability(instruction.result.name, current_reg, true);
        }

        op_index++; 
    }
    
    // Reset descriptors reference
    regs_to_find = &this->m_registers;

    if(is_copy)
    {
        // Take into account indirections
        if(instruction.operands[0].is_acc)
        {
            string load_result = instruction.id.back() == 'b' ? "loadb" : "load";
            string load_id = "load";

            if(this->m_tags[instruction.operands[0].name] == 3 || this->m_tags[instruction.operands[0].name] == 4)
                load_id = "loada";

            // Check if is a float
            if(instruction.result.name.front() == 'f' || instruction.result.name.front() == 'F')
            {
                regs_to_find = &this->m_float_registers;

                if(!is_number(instruction.operands[0].acc))
                {
                    section.emplace_back(mips_instructions.at("load") + space + "$v0" + sep + instruction.operands[0].acc);
                    section.emplace_back(mips_instructions.at("add") + space + "$v0" + sep + op_registers[1] + sep + "$v0");
                    section.emplace_back(mips_instructions.at("fload") + space + op_registers[0] + sep + "($v0)");
                }
                else
                {
                    string op = instruction.operands[0].acc + "(" + op_registers[1] + ")";
                    section.emplace_back(mips_instructions.at("fload") + space + op_registers[0] + sep + op);
                }
            }
            else
            {
                if(!is_number(instruction.operands[0].acc))
                {
                    section.emplace_back(mips_instructions.at(load_id) + space + op_registers[0] + sep + instruction.operands[0].acc);
                    section.emplace_back(mips_instructions.at("add") + space + op_registers[0] + sep + op_registers[0] + sep + op_registers[1]);
                    section.emplace_back(mips_instructions.at(load_result) + space + op_registers[0] + sep + "0(" + op_registers[0] + ")");
                }
                else
                {
                    string op = instruction.operands[0].acc + "(" + op_registers[1] + ")";
                    section.emplace_back(mips_instructions.at(load_result) + space + op_registers[0] + sep + op);
                }
            }

            // Maintain descriptors
            assignment(op_registers[0], instruction.result.name, *regs_to_find);
            availability(instruction.result.name, op_registers[0], true);
        }
        else if(instruction.result.is_acc)
        {
            string store_id = instruction.id.back() == 'b' ? "storeb" : "store";
            string load_id =  "load";

            if(this->m_tags[instruction.result.name] == 3 || this->m_tags[instruction.result.name] == 4)
                load_id = "loada";

            // Check if result register is a float
            if(instruction.result.name.front() == 'f' || instruction.result.name.front() == 'F')
            {
                regs_to_find = &this->m_float_registers;
                store_id = "fstore";
            }


            // Load the base direction of what we want to make an indirection
            vector<string> reg_descriptor = getRegisterDescriptor(op_registers[0], *regs_to_find);
            if ( find(reg_descriptor.begin(), reg_descriptor.end(), instruction.result.name) == reg_descriptor.end() )
            {
                string best_location = findOptimalLocation(instruction.result.name);
                section.emplace_back(mips_instructions.at(load_id) + space + op_registers[0] + sep + best_location);
            }

            // Maintain descriptors
            assignment(op_registers[0], instruction.result.name, *regs_to_find);
            availability(instruction.result.name, op_registers[0], true);
            
            string op = instruction.result.acc + "(" + op_registers[0] + ")";

            if(!is_number(instruction.result.acc))
            {
                section.emplace_back(mips_instructions.at("load") + space + "$v0" + sep + instruction.result.acc);
                section.emplace_back(mips_instructions.at("add") + space + "$v0" + sep + op_registers[0] + sep + "$v0");
                op = "0($v0)";
            }

            section.emplace_back(mips_instructions.at(store_id) + space + op_registers[1] + sep + op);
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
        regs_to_find = &this->m_float_registers;
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
    section.emplace_back(emit);

    // If is div or mod add the special MIPS instructions
    if(instruction.id == "div")
        section.emplace_back(mips_instructions.at("low") + space + op_registers[0]);
    else if(instruction.id == "mod")
        section.emplace_back(mips_instructions.at("high") + space + op_registers[0]);
    

    // Maintain descriptor
    assignment(op_registers[0], instruction.result.name, *regs_to_find, true);
    availability(instruction.result.name, op_registers[0], true);
    removeElementFromDescriptors(this->m_variables, op_registers[0], instruction.result.name);
    removeElementFromDescriptors(*regs_to_find, instruction.result.name, op_registers[0]);
}

void Translator::translateMetaIntruction(T_Instruction instruction)
{
    if(instruction.id == "@string")
    {
        data.emplace_back(".align 2");
        insertVariable(instruction.result.name, 3, instruction.operands[0].name);
        return;
    }
    
    if(instruction.id == "@staticv")
    {
        insertVariable(instruction.result.name, 4, instruction.operands[0].name);
        return;
    }
}

void Translator::translateIOIntruction(T_Instruction instruction, vector<string>& section)
{
    if(instruction.id.find("print") != string::npos)
    {
        const char* arg_register = "$a0";
        if(instruction.id.back() != 'f')
        {
            // Is need to have the element to print in $a0, store the elements if needed
            vector<string> reg_descriptor = getRegisterDescriptor(arg_register, this->m_registers);
            for(string currentVar : reg_descriptor)
            {
                if(is_number(currentVar))
                    continue;
                
                // If static variable ignore
                if(m_tags[currentVar] != 3 && m_tags[currentVar] != 4)
                    section.emplace_back(mips_instructions.at("store") + space + arg_register + sep + currentVar);

                availability(currentVar, currentVar);
            }
            reg_descriptor.clear();
            removeElementFromDescriptors(this->m_variables, arg_register, "");

            string curr_reg = findElementInDescriptors(this->m_registers, instruction.result.name);

            // Move the element to $a0
            if(!curr_reg.empty())
            {
                section.emplace_back(mips_instructions.at("assign") + space + arg_register + sep + curr_reg);
            }
            else
            {
                section.emplace_back(mips_instructions.at("load") + space + arg_register + sep + instruction.result.name);
            }
            
            // Load the correct syscall
            section.emplace_back(mips_instructions.at(instruction.id));
        }
        else
        {
            arg_register = "$f12";
            // Is need to have the element to print in $f12, store the elements if needed
            vector<string> regDescriptor = getRegisterDescriptor(arg_register, this->m_float_registers);
            for(string currentVar : regDescriptor)
            {
                if(is_number(currentVar))
                    continue;
                
                // If static variable ignore
                if(m_tags[currentVar] != 3 && m_tags[currentVar] != 4)
                    section.emplace_back(mips_instructions.at("store") + space + arg_register + sep + currentVar);

                availability(currentVar, currentVar);
            }
            regDescriptor.clear();
            removeElementFromDescriptors(this->m_variables, arg_register, "");

            // Move the element to $a0
            section.emplace_back(mips_instructions.at("fload") + space + arg_register + sep + instruction.result.name);
            // Load the correct syscall
            section.emplace_back(mips_instructions.at(instruction.id));
        }
    }
    else    
    {
        if(instruction.id.back() == 'i' || instruction.id.back() == 'c')
        {
            // Create temporal where is going to be stored
            insertVariable(instruction.result.name, 0);

            // Load correct syscall
            section.emplace_back(mips_instructions.at(instruction.id));

            // Store read value
            section.emplace_back(mips_instructions.at("store") + space + "$v0" + sep + instruction.result.name);

        }
        else if(instruction.id.back() == 'f')
        {
            // Create temporal where is going to be stored
            insertVariable(instruction.result.name, 0);

            // Result is going to be store in $v0
            vector<string> regDescriptor = getRegisterDescriptor("$f12", this->m_float_registers);
            for(string currentVar : regDescriptor)
            {
                if(is_number(currentVar))
                    continue;
                
                // If static variable ignore
                if(m_tags[currentVar] != 3 && m_tags[currentVar] != 4)
                    section.emplace_back(mips_instructions.at("store") + space + "$f12" + sep + currentVar);
                
                availability(currentVar, currentVar);
            }
            regDescriptor.clear();
            removeElementFromDescriptors(this->m_variables, "$f12", "");

            // Load correct syscall
            section.emplace_back(mips_instructions.at(instruction.id));

            // Store read value
            section.emplace_back(mips_instructions.at("store") + space + "$f12" + sep + instruction.result.name);
        }
        else
        {
            const char* addr_register = "$a0";
            const char* size_register = "$a1";

            // Is neccesary the buffer where the string is going to be loaded in $a0
            vector<string> regDescriptor = getRegisterDescriptor(addr_register, this->m_registers);
            for(string currentVar : regDescriptor)
            {
                if(is_number(currentVar))
                    continue;
                
                // If static variable ignore
                if(m_tags[currentVar] != 3 && m_tags[currentVar] != 4)
                    section.emplace_back(mips_instructions.at("store") + space + addr_register + sep + currentVar);

                availability(currentVar, currentVar);
            }
            regDescriptor.clear();
            removeElementFromDescriptors(this->m_variables, addr_register, "");

            // Max size of the string in $a1
            regDescriptor = getRegisterDescriptor(size_register, this->m_registers);
            for(string currentVar : regDescriptor)
            {
                if(is_number(currentVar))
                    continue;
                
                // If static variable ignore
                if(m_tags[currentVar] != 3 && m_tags[currentVar] != 4)
                    section.emplace_back(mips_instructions.at("store") + space + size_register + sep + currentVar);

                availability(currentVar, currentVar);
            }
            regDescriptor.clear();
            removeElementFromDescriptors(this->m_variables, size_register, "");

            // Move buffer to $a0
            section.emplace_back(mips_instructions.at("assign") + space + addr_register + sep + instruction.result.name);
            // TODO: Add size of what is going to be read
            // Load correct syscall
            section.emplace_back(mips_instructions.at(instruction.id));
        }
    }
}

void Translator::printVariablesDescriptors()
{
    for(auto vari : this->m_registers)
    {
        cout << "== " << vari.first << " ==" << endl;
        for(auto element : vari.second)
        {
            cout << element << " ";
        }
        cout << endl;
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
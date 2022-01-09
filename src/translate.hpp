#pragma once

#include <map>
#include <string>
#include <stack>
#include <vector>
#include <ctype.h>
#include <iostream>
#include <algorithm>
#include <unordered_map>

#include "FlowGraph.hpp"

using namespace std;

const string space = "  ";
const string sep = ", ";
const string decl = ": ";

const unordered_map<string, string> mips_instructions ({
    // Aritmethic operations
    {"add", "add"},
    {"addi", "addi"},
    {"sub", "sub"},
    {"mult", "mul"},
    {"div", "div"}, // Is in low
    {"mod", "div"}, // Is in high
    {"minus", "negu"},

    // Logic operations
    {"not", "not"},
    {"and", "and"},
    {"or", "or"},
    {"eq", "seq"},
    {"neq", "sne"},
    {"geq", "sge"},
    {"gt", "sgt"},
    {"leq", "sle"},
    {"lt", "slt"},

    // Jumps and memory access
    {"goto", "j"},
    {"assign", "move"},
    {"goif", "bnez"},
    {"goifnot", "beqz"},
    {"param", "sw"},
    {"call", "jal"},
    {"return", "jr"},
    {"low", "mflo"},
    {"high", "mfhi"},
    {"load", "lw"},
    {"loadi", "li"},
    {"loadb", "lb"},
    {"loada", "la"},
    {"store", "sw"},
    {"storeb", "sb"},

    // Special registers
    {"ra", "$ra"},
    {"sp", "$sp"},
    {"zero", "$zero"},

    // .Data
    {"@string", ".asciiz"},
    {"word", ".word"},
    {"byte", ".byte"},
    {"float", ".float"},

    // Syscalls
    {"exit", "li  $v0, 17 \nsyscall"},
    {"printi", "li  $v0, 1 \nsyscall"},
    {"printf", "li  $v0, 2 \nsyscall"},
    {"printc", "li  $v0, 11 \nsyscall"},
    {"print", "li  $v0, 4 \nsyscall"},
    {"readi", "li  $v0, 5 \nsyscall"},
    {"readf", "li  $v0, 6 \nsyscall"},
    {"readc", "li  $v0, 12 \nsyscall"},
    {"read", "li  $v0, 8 \nsyscall"},

    // Memory management
    // TODO
    {"malloc", "li  $v0, 9 \nsyscall"},
    {"memcpy", ""}, // memcpy ID ID int
    {"free", "li  $v0, 9 \nsyscall"},

    /******* Float Instructions *******/
    // Aritmethic operations
    {"fadd", "add.s"},
    {"fsub", "sub.s"},
    {"fmult", "mul.s"},
    {"fdiv", "div.s"},
    {"fmod", "div.s"},
    {"fminus", "negu.s"},

    // Jumps and memory access
    {"fload", "l.s"},
    {"fstore", "s.s"},

    // Logic operations
    {"feq", "c.eq.s"},
    {"fleq", "c.le.s"},
    {"flt", "c.lt.s"},
});

class Translator
{
private:
    vector<T_Instruction*> m_data_instructions;
    FlowGraph* m_graph;
    unordered_map<string, vector<string>> m_registers;
    unordered_map<string, vector<string>> m_float_registers;
    unordered_map<string, vector<string>> m_variables;
    unordered_map<string, uint32_t> m_tags;

    // Mips data
    vector<string> m_data;
    vector<string> m_text;
    set<string> data_statics;
    bool function_section = false;

    uint64_t current_size;
    uint64_t last_label_id;
    
    // Descriptors management
    bool insertElementToDescriptor(unordered_map<string, vector<string>> &descriptors, const string& key, const string& element, bool replace = false);
    void removeElementFromDescriptors(unordered_map<string, vector<string>> &descriptors, const string& element, const string& current_container);
    string findElementInDescriptors(unordered_map<string, vector<string>> &descriptors, const string& element);
    string findOptimalLocation(string id);
    
    // Registers management
    vector<string> getReg(T_Instruction instruction, bool is_copy = false);
    vector<string> findFreeRegister(unordered_map<string, vector<string>>& curr_registers);
    string recycleRegister(T_Instruction instruction, unordered_map<string, vector<string>>& descriptors, vector<string> &regs);
    void selectRegister(const string& operand, T_Instruction instruction, unordered_map<string, vector<string>>& descriptors, vector<string> &regs, vector<string> &free_regs);
    void cleanRegistersDescriptor();
    
    // Updating descriptors
    bool assignment(const string& register_id, const string& variable_id, unordered_map<string, vector<string>>& curr_registers, bool replace = false);
    bool availability(const string& variable_id, const string& location, bool replace = false);

    // Instructions tranlations
    void translateInstruction(T_Instruction instruction);
    void translateMetaIntruction(T_Instruction instruction);
    void translateOperationInstruction(T_Instruction instruction, bool is_copy = false);
    void translateIOIntruction(T_Instruction instruction);

    // Setters
    bool insertRegister(const string& id, unordered_map<string, vector<string>>& descriptors);
    bool insertVariable(const string& id);

    // Getters
    vector<string> getRegisterDescriptor(const string& id, unordered_map<string, vector<string>>& curr_registers);
    vector<string> getVariableDescriptor(const string& id);

    // Utilities
    void loadTemporal(const string& id, const string& register_id, bool maintain_descriptor = true);
    void storeTemporal(const string& id, const string& register_id, bool replace = false);
    bool is_number(const string& str);
    bool is_static(const string& id);
    bool is_global(const string& id);


public:
    Translator();
    void translate();
    void insertInstruction(T_Instruction* instruction);
    void insertFlowGraph(FlowGraph* graph);
    void print();
    void printVariablesDescriptors();
};

#include "FlowGraph.hpp"

/*
 * Inicializa los conjuntos IN y OUT de cada bloque en vacio.
 */
map<uint64_t, vector<set<string>>> liveVariables_init(FlowGraph* fg) {
    map<uint64_t, vector<set<string>>> sets;
    for (pair<uint64_t, FlowNode*> n : fg->V) sets[n.first] = {{}, {}};
    return sets;
}

/*
 * Inicializa el OUT de ENTRY en vacio.
 */
set<string> liveVariables_initEntryOut(FlowGraph* fg) { return {}; }

/*
 * Inicializa el IN de EXIT en vacio.
 */
set<string> liveVariables_initExitIn(FlowGraph* fg) { return {}; }

// Funciones de transicion de cada instruccion

set<string> liveVariables_assign(set<string> out, T_Instruction instr) {
    char c;
    if (instr.result.is_acc) {
        out.insert(instr.result.name);

        c = instr.result.acc[0];
        if (('A' <= c && c <= 'z') || c == '_') {
            out.insert(instr.result.acc);
        }

        c = instr.operands[0].name[0];
        if (('A' <= c && c <= 'z') || c == '_') {
            out.insert(instr.operands[0].name);
        }
    }
    else if (instr.operands[0].is_acc){
        out.erase(instr.result.name);
        out.insert(instr.operands[0].name);

        c = instr.operands[0].acc[0];
        if (('A' <= c && c <= 'z') || c == '_') {
            out.insert(instr.operands[0].acc);
        }
    } 
    else {
        out.erase(instr.result.name);

        c = instr.operands[0].name[0];
        if (('A' <= c && c <= 'z') || c == '_') {
            out.insert(instr.operands[0].name);
        }
    }

    return out;
}

set<string> liveVariables_f3(set<string> out, T_Instruction instr) {
    char c;
    out.erase(instr.result.name);

    c = instr.operands[0].name[0];
    if (('A' <= c && c <= 'z') || c == '_') {
        out.insert(instr.operands[0].name);
    }

    c = instr.operands[1].name[0];
    if (('A' <= c && c <= 'z') || c == '_') {
        out.insert(instr.operands[1].name);
    }

    return out;
}

set<string> liveVariables_f2(set<string> out, T_Instruction instr) {
    char c;

    out.erase(instr.result.name);

    c = instr.operands[0].name[0];
    if (('A' <= c && c <= 'z') || c == '_') {
        out.insert(instr.operands[0].name);
    }

    return out;
}

set<string> liveVariables_f1(set<string> out, T_Instruction instr) {
    char c;

    c = instr.result.name[0];
    if (('A' <= c && c <= 'z') || c == '_') {
        out.insert(instr.result.name);
    }

    return out;
}

set<string> liveVariables_condGo(set<string> out, T_Instruction instr) {
    char c = instr.operands[0].name[0];
    if (('A' <= c && c <= 'z') || c == '_') {
        out.insert(instr.operands[0].name);
    }
    
    return out;
}

set<string> liveVariables_memcpy(set<string> out, T_Instruction instr) {
    char c;
    
    out.insert(instr.result.name);
    out.insert(instr.operands[0].name);

    c = instr.operands[1].name[0];
    if (('A' <= c && c <= 'z') || c == '_') {
        out.insert(instr.operands[01].name);
    }
    
    return out;
}

set<string> liveVariables_assignf1(set<string> out, T_Instruction instr) {
    out.erase(instr.result.name);
    return out;
}

set<string> liveVariables_f(set<string> out, T_Instruction instr) {
    return out;
}

map<string, set<string> (*) (set<string>, T_Instruction)> functions = {
    {"assignw", &liveVariables_assign},
    {"assignb", &liveVariables_assign},
    {"add"    , &liveVariables_f3},
    {"sub"    , &liveVariables_f3},
    {"mult"   , &liveVariables_f3},
    {"div"    , &liveVariables_f3},
    {"mod"    , &liveVariables_f3},
    {"minus"  , &liveVariables_f2},
    {"ftoi"   , &liveVariables_f2},
    {"itof"   , &liveVariables_f2},
    {"eq"     , &liveVariables_f3},
    {"neq"    , &liveVariables_f3},
    {"lt"     , &liveVariables_f3},
    {"leq"    , &liveVariables_f3},
    {"gt"     , &liveVariables_f3},
    {"geq"    , &liveVariables_f3},
    {"or"     , &liveVariables_f3},
    {"and"    , &liveVariables_f3},
    {"goto"   , &liveVariables_f},
    {"goif"   , &liveVariables_condGo},
    {"goifnot", &liveVariables_condGo},
    {"malloc" , &liveVariables_f2},
    {"memcpy" , &liveVariables_memcpy},
    {"free"   , &liveVariables_f1},
    {"exit"   , &liveVariables_f1},
    {"param"  , &liveVariables_f2},
    {"return" , &liveVariables_f1},
    {"call"   , &liveVariables_assignf1},
    {"printc" , &liveVariables_f1},
    {"printi" , &liveVariables_f1},
    {"printf" , &liveVariables_f1},
    {"print"  , &liveVariables_f1},
    {"readc"  , &liveVariables_assignf1},
    {"readi"  , &liveVariables_assignf1},
    {"readf"  , &liveVariables_assignf1},
    {"read"   , &liveVariables_f1}
};

/*
 * Analisis de flujo para variables vivas.
 */
void FlowGraph::liveVariables(void) {
    this->live = this->flowAnalysis<string>(
        &liveVariables_init,
        &liveVariables_initEntryOut,
        &liveVariables_initExitIn,
        [] (FlowGraph* fg, uint64_t id, set<string> S) { return S; },
        [] (FlowGraph* fg, uint64_t id, set<string> S) { return S; },
        functions,
        false,
        false
    );

    for (pair<uint64_t, vector<set<string>>> sets : this->live) {
        // Ignoramos la variable BASE
        this->live[sets.first][0].erase("BASE");
        this->live[sets.first][1].erase("BASE");

        // Eliminamos las variables estaticas
        for (string staticVar : this->staticVars) {
            this->live[sets.first][0].erase(staticVar);
            this->live[sets.first][1].erase(staticVar);
        }
    }
}

set<string> instrToValidate = {
    "assignw", "assignb", "add", "sub", "minus", "mult", "mod", "ftoi", "itof",
    "eq", "neq", "lt", "leq", "gt", "geq", "or", "and" 
};

/*
 * Elimina las asignaciones de variables muertas.
 */
void FlowGraph::deleteDeadVariables(void) {
    this->liveVariables();

    set<string> out;
    T_Instruction instr;
    for (pair<uint64_t, FlowNode*> n : this->V) {
        // Creamos un nuevo bloque y obtenemos el OUT del bloque actual.
        vector<T_Instruction> newBlock;
        out = this->live[n.first][1];

        for (int i = n.second->block.size()-1; i >= 0; i--) {
            instr = n.second->block[i];
            // La instruccion se agregara al nuevo bloque si no es una asignacion, o
            // si se hace una asignacion a memoria o si la variable esta viva.
            if (
                instrToValidate.count(instr.id) == 0 || 
                instr.result.is_acc || out.count(instr.result.name) > 0
            ) {
                newBlock.push_back(instr); 
                out = (*functions[instr.id]) (out, instr);
            }
        }

        // Asignamos el nuevo bloque.
        reverse(newBlock.begin(), newBlock.end());
        n.second->block = newBlock;
    }
}


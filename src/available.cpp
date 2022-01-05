#include "FlowGraph.hpp"

/*
 * Inicializa los conjuntos IN y OUT
 */
map<uint64_t, vector<set<Expression>>> available_init(FlowGraph* fg) {
    // Los IN seran todas las expresiones y el OUT sera vacio
    map<uint64_t, vector<set<Expression>>> sets;
    for (pair<uint64_t, FlowNode*> n : fg->V) sets[n.first] = {{}, fg->expressions};
    return sets;
}

/*
 * Inicializa el OUT de ENTRY en vacio.
 */
set<Expression> available_initEntryOut(FlowGraph* fg) { return {}; }

/*
 * Inicializa el IN de EXIT en vacio.
 */
set<Expression> available_initExitIn(FlowGraph* fg) { return {}; }

// Funciones de transicion de cada instruccion

set<Expression> available_assign(set<Expression> out, T_Instruction instr) {
    // Si no se asigna a un acceso a memoria
    if (! instr.result.is_acc) {
        // Se eliminan todas las expresiones que usen el operando modificado
        set<Expression> toErase;
        for (Expression e : out) {
            if (e.A == instr.result.name || e.B == instr.result.name) {
                toErase.insert(e);
            }
        }
        for (Expression e : toErase) {
            out.erase(e);
        }
    }

    return out;
}

set<Expression> available_valids(set<Expression> out, T_Instruction instr) {
    // Se eliminan todas las expresiones que usen el operando modificado
    set<Expression> toErase;
    for (Expression e : out) {
        if (e.A == instr.result.name || e.B == instr.result.name) {
            toErase.insert(e);
        }
    }
    for (Expression e : toErase) {
        out.erase(e);
    }

    return out;
}

set<Expression> available_f(set<Expression> out, T_Instruction instr) {
    return out;
}

set<Expression> available_preprocessing(FlowGraph *fg, uint64_t id, set<Expression> in) {
    return setUnion<Expression>(in, fg->anticipated[id][0]);
}

/*
 * Analisis de flujo para expresiones anticipadas.
 */
void FlowGraph::availableDefinitions(void) {
    this->available = this->flowAnalysis<Expression>(
        &available_init,
        &available_initEntryOut,
        &available_initExitIn,
        &available_preprocessing,
        [] (FlowGraph* fg, uint64_t id, set<Expression> S) { return S; },
        {
            {"assignw", &available_assign},
            {"assignb", &available_assign},
            {"add"    , &available_valids},
            {"sub"    , &available_valids},
            {"mult"   , &available_valids},
            {"div"    , &available_valids},
            {"mod"    , &available_valids},
            {"minus"  , &available_valids},
            {"ftoi"   , &available_valids},
            {"itof"   , &available_valids},
            {"eq"     , &available_valids},
            {"neq"    , &available_valids},
            {"lt"     , &available_valids},
            {"leq"    , &available_valids},
            {"gt"     , &available_valids},
            {"geq"    , &available_valids},
            {"or"     , &available_valids},
            {"and"    , &available_valids},
            {"goto"   , &available_f},
            {"goif"   , &available_f},
            {"goifnot", &available_f},
            {"malloc" , &available_valids},
            {"memcpy" , &available_f},
            {"free"   , &available_f},
            {"exit"   , &available_f},
            {"param"  , &available_valids},
            {"return" , &available_f},
            {"call"   , &available_valids},
            {"printc" , &available_f},
            {"printi" , &available_f},
            {"printf" , &available_f},
            {"print"  , &available_f},
            {"readc"  , &available_valids},
            {"readi"  , &available_valids},
            {"readf"  , &available_valids},
            {"read"   , &available_f}
        },
        true,
        true
    );
}

/*
 * Una vez calculadas las expresiones disponibles, podemos calcular las mas tempranas
 */
void FlowGraph::earliestDefinitions(void) {
    for (pair<uint64_t, FlowNode*> n : this->V) {
        this->earliest[n.first] = {{}, {}};

        this->earliest[n.first][0] = setSub<Expression>(
            this->anticipated[n.first][0],
            this->available[n.first][0]
        );
    }
}
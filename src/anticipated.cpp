#include "FlowGraph.hpp"

set<string> anticiped_validOperations = {
    "add", "sub", "mult", "div", "mod", "minus", "ftoi", "itof", 
    "eq", "neq", "lt", "leq", "gt", "geq", "or", "and"
};

/*
 * Inicializa los conjuntos IN y OUT de cada bloque.
 */
map<uint64_t, vector<set<Expression>>> anticipated_init(FlowGraph* fg) {
    // Obtenemos todas las expresiones del grafo
    for (pair<uint64_t, FlowNode*> n : fg->V) {
        for (T_Instruction instr : n.second->block) {
            if (anticiped_validOperations.count(instr.id)) {
                fg->expressions.insert({
                    instr.id, 
                    instr.operands[0].name, 
                    (instr.operands.size() > 1 ? instr.operands[1].name : "")
                });
            }
        }
    }

    // Los IN seran todas las expresiones y el OUT sera vacio
    map<uint64_t, vector<set<Expression>>> sets;
    for (pair<uint64_t, FlowNode*> n : fg->V) sets[n.first] = {fg->expressions, {}};
    return sets;
}

/*
 * Inicializa el OUT de ENTRY en vacio.
 */
set<Expression> anticipated_initEntryOut(FlowGraph* fg) { return {}; }

/*
 * Inicializa el IN de EXIT en vacio.
 */
set<Expression> anticipated_initExitIn(FlowGraph* fg) { return {}; }

// Funciones de transicion de cada instruccion

set<Expression> anticipated_assign(set<Expression> out, T_Instruction instr) {
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

set<Expression> anticipated_f(set<Expression> out, T_Instruction instr) {
    return out;
}

set<Expression> anticipated_postprocess(FlowGraph *fg, uint64_t id, set<Expression> in) {
    return setUnion<Expression>(in, fg->use_B[id]);
}

/*
 * Calculamos las expresiones usadas por cada bloque.
 */
void FlowGraph::computeUseB(void) {
    for (pair<uint64_t, FlowNode*> n : this->V) {
        set<Expression> use;

        for (T_Instruction instr : n.second->block) {
            if (anticiped_validOperations.count(instr.id)) {
                use.insert({
                    instr.id, 
                    instr.operands[0].name, 
                    (instr.operands.size() > 1 ? instr.operands[1].name : "")
                });
            }
        }
        this->use_B[n.first] = use;
    }
}


/*
 * Analisis de flujo para expresiones anticipadas.
 */
void FlowGraph::anticipatedDefinitions(void) {
    // Calculamos todas las expresiones usadas en cada bloque.
    this->computeUseB();

    this->anticipated = this->flowAnalysis<Expression>(
        &anticipated_init,
        &anticipated_initEntryOut,
        &anticipated_initExitIn,
        [] (FlowGraph* fg, uint64_t id, set<Expression> S) { return S; },
        &anticipated_postprocess,
        {
            {"assignw", &anticipated_assign},
            {"assignb", &anticipated_assign},
            {"add"    , &anticipated_assign},
            {"sub"    , &anticipated_assign},
            {"mult"   , &anticipated_assign},
            {"div"    , &anticipated_assign},
            {"mod"    , &anticipated_assign},
            {"minus"  , &anticipated_assign},
            {"ftoi"   , &anticipated_assign},
            {"itof"   , &anticipated_assign},
            {"eq"     , &anticipated_assign},
            {"neq"    , &anticipated_assign},
            {"lt"     , &anticipated_assign},
            {"leq"    , &anticipated_assign},
            {"gt"     , &anticipated_assign},
            {"geq"    , &anticipated_assign},
            {"or"     , &anticipated_assign},
            {"and"    , &anticipated_assign},
            {"goto"   , &anticipated_f},
            {"goif"   , &anticipated_f},
            {"goifnot", &anticipated_f},
            {"malloc" , &anticipated_assign},
            {"memcpy" , &anticipated_f},
            {"free"   , &anticipated_f},
            {"exit"   , &anticipated_f},
            {"param"  , &anticipated_assign},
            {"return" , &anticipated_f},
            {"call"   , &anticipated_assign},
            {"printc" , &anticipated_f},
            {"printi" , &anticipated_f},
            {"printf" , &anticipated_f},
            {"print"  , &anticipated_f},
            {"readc"  , &anticipated_assign},
            {"readi"  , &anticipated_assign},
            {"readf"  , &anticipated_assign},
            {"read"   , &anticipated_f}
        },
        true,
        false
    );
}
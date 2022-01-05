#include "FlowGraph.hpp"

/*
 * Inicializa los conjuntos IN y OUT
 */
map<uint64_t, vector<set<Expression>>> postponable_init(FlowGraph* fg) {
    // Los IN seran todas las expresiones y el OUT sera vacio
    map<uint64_t, vector<set<Expression>>> sets;
    for (pair<uint64_t, FlowNode*> n : fg->V) sets[n.first] = {{}, fg->expressions};
    return sets;
}

/*
 * Inicializa el OUT de ENTRY en vacio.
 */
set<Expression> postponable_initEntryOut(FlowGraph* fg) { return {}; }

/*
 * Inicializa el IN de EXIT en vacio.
 */
set<Expression> postponable_initExitIn(FlowGraph* fg) { return {}; }

// Funciones de transicion de cada instruccion

set<Expression> postponable_f(set<Expression> in, T_Instruction instr) {
    return in;
}

set<Expression> postponable_preprocess(FlowGraph *fg, uint64_t id, set<Expression> in) {
    return setUnion<Expression>(in, fg->earliest[id][0]);
}

set<Expression> postponable_postprocess(FlowGraph *fg, uint64_t id, set<Expression> in) {
    return setSub<Expression>(in, fg->use_B[id]);
}

/*
 * Analisis de flujo para expresiones diferibles.
 */
void FlowGraph::postponableDefinitions(void) {
    this->postponable = this->flowAnalysis<Expression>(
        &postponable_init,
        &postponable_initEntryOut,
        &postponable_initExitIn,
        &postponable_preprocess,
        &postponable_postprocess,
        {
            {"assignw", &postponable_f},
            {"assignb", &postponable_f},
            {"add"    , &postponable_f},
            {"sub"    , &postponable_f},
            {"mult"   , &postponable_f},
            {"div"    , &postponable_f},
            {"mod"    , &postponable_f},
            {"minus"  , &postponable_f},
            {"ftoi"   , &postponable_f},
            {"itof"   , &postponable_f},
            {"eq"     , &postponable_f},
            {"neq"    , &postponable_f},
            {"lt"     , &postponable_f},
            {"leq"    , &postponable_f},
            {"gt"     , &postponable_f},
            {"geq"    , &postponable_f},
            {"or"     , &postponable_f},
            {"and"    , &postponable_f},
            {"goto"   , &postponable_f},
            {"goif"   , &postponable_f},
            {"goifnot", &postponable_f},
            {"malloc" , &postponable_f},
            {"memcpy" , &postponable_f},
            {"free"   , &postponable_f},
            {"exit"   , &postponable_f},
            {"param"  , &postponable_f},
            {"return" , &postponable_f},
            {"call"   , &postponable_f},
            {"printc" , &postponable_f},
            {"printi" , &postponable_f},
            {"printf" , &postponable_f},
            {"print"  , &postponable_f},
            {"readc"  , &postponable_f},
            {"readi"  , &postponable_f},
            {"readf"  , &postponable_f},
            {"read"   , &postponable_f}
        },
        true,
        true
    );
}

/*
 * Una vez calculadas las expresiones diferibles, podemos calcular el punto en el cuál la 
 * expresión pasa de ser diferible a ser necesaria inmediatamente
 */
void FlowGraph::latestDefinitions(void) {
    uint64_t id;
    set<Expression> S;

    for (pair<uint64_t, FlowNode*> n : this->V) {
        id = n.first;
        this->latest[id] = {{}, {}};

        S = this->use_B[n.first];
        for (uint64_t s_id : this->E[id]) {
            S = setUnion<Expression>(
                S, 
                setIntersec<Expression>(
                    setSub<Expression>(this->expressions, this->earliest[s_id][0]),
                    setSub<Expression>(this->expressions, this->postponable[s_id][0])
                )
            );
        }

        this->latest[id][0] = setIntersec<Expression>(
            setUnion<Expression>(this->earliest[id][0], this->postponable[id][0]),
            S
        );
    }
}
#include "FlowGraph.hpp"

/*
 * Inicializa los conjuntos IN y OUT
 */
map<uint64_t, vector<set<Expression>>> used_init(FlowGraph* fg) {
    // Los IN y OUT seran vacios
    map<uint64_t, vector<set<Expression>>> sets;
    for (pair<uint64_t, FlowNode*> n : fg->V) sets[n.first] = {{}, {}};
    return sets;
}

/*
 * Inicializa el OUT de ENTRY en vacio.
 */
set<Expression> used_initEntryOut(FlowGraph* fg) { return {}; }

/*
 * Inicializa el IN de EXIT en vacio.
 */
set<Expression> used_initExitIn(FlowGraph* fg) { return {}; }

// Funciones de transicion de cada instruccion

set<Expression> used_f(set<Expression> in, T_Instruction instr) {
    return in;
}

set<Expression> used_preprocess(FlowGraph *fg, uint64_t id, set<Expression> in) {
    return setUnion<Expression>(in, fg->use_B[id]);
}

set<Expression> used_postprocess(FlowGraph *fg, uint64_t id, set<Expression> in) {
    return setSub<Expression>(in, fg->latest[id][0]);
}

/*
 * Analisis de flujo para expresiones diferibles.
 */
void FlowGraph::usedDefinitions(void) {
    this->used = this->flowAnalysis<Expression>(
        &used_init,
        &used_initEntryOut,
        &used_initExitIn,
        &used_preprocess,
        &used_postprocess,
        {
            {"assignw", &used_f},
            {"assignb", &used_f},
            {"add"    , &used_f},
            {"sub"    , &used_f},
            {"mult"   , &used_f},
            {"div"    , &used_f},
            {"mod"    , &used_f},
            {"minus"  , &used_f},
            {"ftoi"   , &used_f},
            {"itof"   , &used_f},
            {"eq"     , &used_f},
            {"neq"    , &used_f},
            {"lt"     , &used_f},
            {"leq"    , &used_f},
            {"gt"     , &used_f},
            {"geq"    , &used_f},
            {"or"     , &used_f},
            {"and"    , &used_f},
            {"goto"   , &used_f},
            {"goif"   , &used_f},
            {"goifnot", &used_f},
            {"malloc" , &used_f},
            {"memcpy" , &used_f},
            {"free"   , &used_f},
            {"exit"   , &used_f},
            {"param"  , &used_f},
            {"return" , &used_f},
            {"call"   , &used_f},
            {"printc" , &used_f},
            {"printi" , &used_f},
            {"printf" , &used_f},
            {"print"  , &used_f},
            {"readc"  , &used_f},
            {"readi"  , &used_f},
            {"readf"  , &used_f},
            {"read"   , &used_f}
        },
        false,
        false
    );
}

/*
 * Obtiene un nuevo temporal que no se haya usado
 */
string newTemp(uint64_t &current, set<string> temps, string prefix) {
    string temp = prefix + to_string(current);
    while (temps.count(temp) > 0) {
        current++;
        temp = prefix + to_string(current);
    }
    current++;

    cout << "# " << temp << "\n";
    return temp;
}

set<string> byteOperations = {
    "eq", "neq", "lt", "leq", "gt", "geq", "or", "and"
};

/*
 * Realizamos el analisis de Lazy Code Motion
 */
void FlowGraph::lazyCodeMotion(void) {
    this->anticipatedDefinitions();
    this->availableDefinitions();
    this->earliestDefinitions();
    this->postponableDefinitions();
    this->latestDefinitions();
    this->usedDefinitions();

    // Calculamos todas las variables usadas en el programa.
    set<string> temps;
    for (pair<uint64_t, FlowNode*> n : this->V) {
        for (T_Instruction instr : n.second->block) {
            if (instr.id == "goto") {
                continue;
            }

            if (instr.id == "goif" || instr.id == "goifnot") {
                temps.insert(instr.operands[0].name);
            }
            else if (instr.id == "call") {
                temps.insert(instr.result.name);
            }
            else {
                temps.insert(instr.result.name);
                if (instr.result.is_acc) temps.insert(instr.result.acc);

                if (instr.operands.size() > 0) {
                    temps.insert(instr.operands[0].name);
                    if (instr.operands[0].is_acc) temps.insert(instr.operands[0].acc);
                }

                if (instr.operands.size() > 1) temps.insert(instr.operands[1].name);
            }
        }
    }

    // Aplicamos el criterio de Lazy Code Motion
    map<Expression, string> newTemps;
    set<Expression> S;
    uint64_t currentT = 0, currentF = 0;
    T_Instruction instr;

    // Primero calculamos por cada bloque B el conjunto latest[B] /\ used[B] y para todas
    // las expressiones A op B en ese conjunto se agrega al principio del bloque la
    // instruccion  T := A op B  donde T es un temporal nuevo.
    for (pair<uint64_t, FlowNode*> n : this->V) {
        S = setIntersec<Expression>(this->latest[n.first][0], this->used[n.first][1]);

        for (Expression e : S) {
            if (newTemps.count(e) == 0) {
                if (e.A[0] == 'f' || (e.B != "" && e.B[0] == 'f')) {
                    newTemps[e] = newTemp(currentF, temps, "f");
                }
                else {
                    newTemps[e] = newTemp(currentT, temps, "T");
                }
            }

            instr = {e.op, {newTemps[e], "", false}, {{e.A, "", false}}};
            if (e.B != "") {
                instr.operands.push_back({e.B, "", false});
            }

            n.second->block.insert(n.second->block.begin(), instr);
        }
    }

    Expression e;
    string assignType;

    // Luego, por cada bloque B, calculamos el conjunto  use_B /\ (~latest[B] \/ used[B])
    // y para toda instruccion  V := A op B  tal que  A op B  esta en el conjunto, 
    // sustituimos la instruccion por   V := T  donde  T  es el temporal por el que se
    // sustituyo la expresion en el paso anterior.
    for (pair<uint64_t, FlowNode*> n : this->V) {
        S = setIntersec<Expression>(
            this->use_B[n.first],
            setUnion<Expression>(
                setSub<Expression>(this->expressions, this->latest[n.first][0]),
                this->used[n.first][1]
            )
        );

        for (uint64_t i = 0; i < n.second->block.size(); i++) {
            instr = n.second->block[i];
            
            e = {
                instr.id, 
                (instr.operands.size() > 0 ? instr.operands[0].name : ""), 
                (instr.operands.size() > 1 ? instr.operands[1].name : "")
            };

            if (S.count(e) > 0 && instr.result.name != newTemps[e]) {
                assignType = byteOperations.count(instr.id) > 0 ? "assignb" : "assignw";

                n.second->block[i] = {
                    assignType,
                    instr.result,
                    {{newTemps[e], "", false}}
                };
            }
        }
    }
}
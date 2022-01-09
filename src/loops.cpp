#include "FlowGraph.hpp"

/*
 * Inicializa los conjuntos IN y OUT de cada bloque.
 */
map<uint64_t, vector<set<uint64_t>>> dominators_init(FlowGraph* fg) {
    set<uint64_t> nodes;
    for (pair<uint64_t, FlowNode*> n : fg->V) nodes.insert(n.first);

    // Los IN seran vaciosy los OUT seran todos los nodos
    map<uint64_t, vector<set<uint64_t>>> sets;
    for (pair<uint64_t, FlowNode*> n : fg->V) sets[n.first] = {{}, nodes};
    return sets;
}

/*
 * Inicializa el OUT de ENTRY en vacio.
 */
set<uint64_t> dominators_initEntryOut(FlowGraph* fg) { return {}; }

/*
 * Inicializa el IN de EXIT en vacio.
 */
set<uint64_t> dominators_initExitIn(FlowGraph* fg) { return {}; }

// Funciones de transicion de cada instruccion

set<uint64_t> dominators_f(set<uint64_t> in, T_Instruction instr) {
    return in;
}

set<uint64_t> dominators_preprocess(FlowGraph *fg, uint64_t id, set<uint64_t> in) {
    in.insert(id);
    return in;
}

/*
 * Analisis de flujo para calcular dominadores.
 */
void FlowGraph::computeDominators(void) {
    this->dominators = this->flowAnalysis<uint64_t>(
        &dominators_init,
        &dominators_initEntryOut,
        &dominators_initExitIn,
        &dominators_preprocess,
        [] (FlowGraph* fg, uint64_t id, set<uint64_t> S) { return S; },
        {
            {"assignw", &dominators_f},
            {"assignb", &dominators_f},
            {"add"    , &dominators_f},
            {"sub"    , &dominators_f},
            {"mult"   , &dominators_f},
            {"div"    , &dominators_f},
            {"mod"    , &dominators_f},
            {"minus"  , &dominators_f},
            {"ftoi"   , &dominators_f},
            {"itof"   , &dominators_f},
            {"eq"     , &dominators_f},
            {"neq"    , &dominators_f},
            {"lt"     , &dominators_f},
            {"leq"    , &dominators_f},
            {"gt"     , &dominators_f},
            {"geq"    , &dominators_f},
            {"or"     , &dominators_f},
            {"and"    , &dominators_f},
            {"goto"   , &dominators_f},
            {"goif"   , &dominators_f},
            {"goifnot", &dominators_f},
            {"malloc" , &dominators_f},
            {"memcpy" , &dominators_f},
            {"free"   , &dominators_f},
            {"exit"   , &dominators_f},
            {"param"  , &dominators_f},
            {"return" , &dominators_f},
            {"call"   , &dominators_f},
            {"printc" , &dominators_f},
            {"printi" , &dominators_f},
            {"printf" , &dominators_f},
            {"print"  , &dominators_f},
            {"readc"  , &dominators_f},
            {"readi"  , &dominators_f},
            {"readf"  , &dominators_f},
            {"read"   , &dominators_f}
        },
        true,
        true
    );
}

/*
 * Calcula los ciclos naturales del codigo.
 */
void FlowGraph::computNaturalLoops(void) {
    // Calculamos los dominadores
    this->computeDominators();

    // Calculamos los arcos de retorno en el grafo
    map<uint64_t, uint64_t> retEdges = {};
    set<uint64_t> visited = {0};
    stack<uint64_t> toVisite;
    uint64_t node;

    // Para ello aplicamos DFS
    toVisite.push(0);
    while (toVisite.size() > 0) {
        node = toVisite.top();
        toVisite.pop();

        for (uint64_t succ : this->E[node]) {
            // Si no esta visitado lo agregamos a la cola.
            if (visited.count(succ) == 0) {
                visited.insert(succ);
                toVisite.push(succ);
            }
            // En cambio si ya esta visitado y el sucesor domina al nodo, entonces es
            // un arco de retorno
            else if (this->dominators[node][1].count(succ) > 0) {
                retEdges[node] = succ;
            }
        }
    }

    // Calculamos los ciclos naturales usando los arcos de retorno
    set<uint64_t> exits;
    for (pair<uint64_t, uint64_t> edge : retEdges) {
        // Aplicamos DFS sobre el grafo inverso.
        visited = {edge.first, edge.second};
        if (edge.first != edge.second) toVisite.push(edge.first);

        while (toVisite.size() > 0) {
            node = toVisite.top();
            toVisite.pop();

            for (uint64_t pred : this->Einv[node]) {
                // Si no esta visitado, lo agregamos a la cola y al cilo natural.
                if (visited.count(pred) == 0) {
                    visited.insert(pred);
                    toVisite.push(pred);
                }
            }
        }

        // Calculamos los bloques de salida.
        exits = {};
        for (uint64_t B : visited) {
            for (uint64_t succ : this->E[B]) {
                if (visited.count(succ) == 0) {
                    exits.insert(B);
                    break;
                }
            }
        }

        // Verificamos si el loop ya existe (por lo tanto hay dos loops con el mismo
        // header, que se considerarian el mismo loop)
        if (this->naturalLoops.count(edge.second) > 0) {
            for (uint64_t B : visited) {
                this->naturalLoops[edge.second].blocks.insert(B);
            }
            for (uint64_t B : exits) {
                this->naturalLoops[edge.second].exits.insert(B);
            }
        }
        else {
            T_Loop l;
            l.header = edge.second;
            l.blocks = visited;
            l.exits = exits;
            l.hasPreHeader = false;
            l.preHeader = 0;
            this->naturalLoops[edge.second] = l;
        }
    }
}

set<string> loops_validOperations = {
    "assignw", "assignb",
    "add", "sub", "mult", "div", "mod", "minus", "ftoi", "itof", 
    "eq", "neq", "lt", "leq", "gt", "geq", "or", "and"
};

void addPreHeader(FlowGraph *fg, uint64_t header, set<uint64_t> loop) {
    uint64_t preHeader = fg->lastID++;

    // Creamos un nuevo bloque.
    fg->V.insert({preHeader, new FlowNode(preHeader, fg->V[header]->is_function)});
    fg->V[preHeader]->f_id = fg->V[header]->f_id;

    // Si el header original era una funcion, transferimos todos los atributos 
    // correspondientes
    if (fg->V[header]->is_function) {
        fg->V[header]->is_function = false;
        fg->F.erase(fg->V[header]->getName());

        fg->V[preHeader]->function_id   = fg->V[header]->function_id;
        fg->V[preHeader]->function_size = fg->V[header]->function_size;
        fg->V[preHeader]->function_end  = fg->V[header]->function_end;
        fg->F[fg->V[preHeader]->getName()] = preHeader;

        // Sustituimos las llamadas al header por llamadas al 
        // pre-header
        for (uint64_t call : fg->called[header]) {
            fg->caller[call] = preHeader;
        }
        fg->called[preHeader] = fg->called[header];
        fg->called.erase(header);
    }

    // Todos los predecesores del header que no son parte del ciclo ahora son 
    // predecesores del pre-header
    string name = fg->V[preHeader]->getName();
    fg->Einv[preHeader] = {};
    set<uint64_t> toDelete;
    for (uint64_t pred : fg->Einv[header]) {
        if (loop.count(pred) == 0) {
            fg->E[pred].erase(header);
            fg->E[pred].insert(preHeader);

            // Si el predecesor es un salto, tenemos que sustituir el
            // nombre del bloque al que se esta saltando.
            if (
                fg->V[pred]->block.back().id == "goto" ||
                fg->V[pred]->block.back().id == "goif" ||
                fg->V[pred]->block.back().id == "goifnot" 
                ) {
                fg->V[pred]->block.back().result.name = name;
            }

            fg->Einv[preHeader].insert(pred);
            toDelete.insert(pred);
        }
    }
    for (uint64_t pred : toDelete) fg->Einv[header].erase(pred);
    fg->Einv[header].insert(preHeader); 
    fg->E[preHeader] = {header};

    fg->naturalLoops[header].hasPreHeader = true;
    fg->naturalLoops[header].preHeader = preHeader;
}

/*
 * Detecta y mueve fuera de los ciclos los calculos invariantes.
 */
void FlowGraph::invariantDetection(void) {
    // Por cada ciclo natural.
    pair<uint64_t, uint64_t> definition;
    T_Instruction instr, instr_j;
    uint64_t preHeader;
    string var, var_j;
    bool change = true, domain, invariant;

    while (change) {
        this->computNaturalLoops();
        // Obtenemos los bucles en sentido inverso (primero los que aparecen luego en 
        // el codigo)
        vector<uint64_t> loops;
        for (pair<uint64_t, T_Loop> loop : this->naturalLoops) {
            loops.push_back(loop.first);
        }
        reverse(loops.begin(), loops.end());

        // Calculamos el alcance de las definiciones
        this->reachingDefinitions();

        change = false;
        for (uint64_t header : loops) {
            for (uint64_t B : this->naturalLoops[header].blocks) {
                // Verificamos que el bloque domina todas las salidas.
                domain = true;
                for (uint64_t B_exit : this->naturalLoops[header].exits) {
                    // Si la salida no esta dominada por B, pasamos al siguiente bloque.
                    if (this->dominators[B_exit][1].count(B) == 0) {
                        domain = false;
                        break;
                    }
                }
                if (! domain) continue;

                for (uint64_t i = 0; i < this->V[B]->block.size(); i++) {
                    instr = this->V[B]->block[i];
                    // Verificamos si la instruccion es valida
                    if (loops_validOperations.count(instr.id) == 0) continue; 

                    // Verificamos que la asignacion no se realiza a un acceso a memoria
                    if (instr.result.is_acc) continue;

                    // Verificamos que ningun otro bloque define esta variable y que los
                    // usos alcanzan la definicion (B, i)
                    var = instr.result.name;
                    invariant = true;
                    for (uint64_t B_j : this->naturalLoops[header].blocks) {
                        for (uint64_t j = 0; j < this->V[B_j]->block.size(); j++) {
                            instr_j = this->V[B_j]->block[j];

                            // Verificamos si usa en sus operandos a la variable.
                            if (
                                (instr_j.operands.size() > 0 && (
                                    instr_j.operands[0].name == var ||
                                    instr_j.operands[0].acc == var
                                )) || 
                                (instr_j.operands.size() > 1 && instr_j.operands[1].name == var)
                                ) {
                                // En caso de ser asi, verificamos que la definicion de esa
                                // variable corresponde a la de (B, i) y solo esa.
                                if (this->reaching[B_j][0][var].size() != 1) {
                                    invariant = false;
                                    break;
                                }

                                definition = *this->reaching[B_j][0][var].begin();
                                if (definition.first != B || definition.second != i) {
                                    invariant = false;
                                    break;
                                }
                            }

                            // Verificamos que la instruccion es valida y no es un acceso
                            // a memoria
                            if (loops_validOperations.count(instr.id) == 0) continue;
                            if (instr_j.result.is_acc) continue;

                            // Si la instruccion no es la misma que la del ciclo externo y
                            // define a la misma variable, entonces no es una definicion
                            // invariante.
                            if (B != B_j && i != j && instr_j.result.name == var) {
                                invariant = false;
                                break;
                            }
                        }
                        if (! invariant) break;
                    }
                    
                    // Si la instruccion es invariante, lo movemos al pre-header del bloque.
                    if (invariant) {
                        // Verificamos si el bloque ya tiene un pre-header
                        if (! this->naturalLoops[header].hasPreHeader) {
                            addPreHeader(this, header, this->naturalLoops[header].blocks);
                        }

                        // Agregamos la instruccion al pre-header
                        preHeader = this->naturalLoops[header].preHeader;
                        this->V[preHeader]->block.push_back(instr);
                        // Eliminamos la instruccion del bloque.
                        this->V[B]->block.erase(this->V[B]->block.begin() + i);
                        i--;

                        change = true;
                    }
                }
            }
        }
    }
}


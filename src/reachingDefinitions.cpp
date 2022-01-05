#include "FlowGraph.hpp"

set<string> assignInstructions = {
    "assignw",
    "assignb",
    "add"    ,
    "sub"    ,
    "mult"   ,
    "div"    ,
    "mod"    ,
    "minus"  ,
    "ftoi"   ,
    "itof"   ,
    "eq"     ,
    "neq"    ,
    "lt"     ,
    "leq"    ,
    "gt"     ,
    "geq"    ,
    "or"     ,
    "and"    ,
    "malloc" ,
    "param"  ,
    "call"   ,
    "readc"  ,
    "readi"  ,
    "readf"
};

map<string, set<pair<uint64_t, uint64_t>>> F_B(
    FlowNode *n, 
    map<string, set<pair<uint64_t, uint64_t>>> in
) {
    T_Instruction instr;
    for (uint64_t i = 0; i < n->block.size(); i++) {
        instr = n->block[i];

        // Si la instruccion realiza una asignacion no a memoria, entonces matamos la 
        // definicion y agregamos una nueva
        if (assignInstructions.count(instr.id) > 0 && ! instr.result.is_acc) {
            in[instr.result.name] = {{n->id, i}};
        }
    }
    return in;
}

map<string, set<pair<uint64_t, uint64_t>>> mapUnion(
    map<string, set<pair<uint64_t, uint64_t>>> U, 
    map<string, set<pair<uint64_t, uint64_t>>> V
) {
    for (pair<string, set<pair<uint64_t, uint64_t>>> e : V) {
        if (U.count(e.first) > 0) {
            for (pair<uint64_t, uint64_t> instr : e.second) {
                U[e.first].insert(instr);
            }
        }
        else {
            U[e.first] = e.second;
        }
    }
    return U;
}


/*
 * Algoritmo de analisis de flujo para el calculo de definiciones vigentes.
 */
void FlowGraph::reachingDefinitions(void) {
    // IN y OUT de cada bloque.
    map<string, set<pair<uint64_t, uint64_t>>> aux, base;
    bool change = true;
    uint64_t id;

    // Inicializamos cada IN y OUT como vacios
    for (pair<uint64_t, FlowNode*> n : this->V) {
        this->reaching[n.first] = {{}, {}};
    }

    // Ejecutamos hasta alcanzar un punto fijo
    while (change) {
        change = false;
        for (pair<uint64_t, FlowNode*> n : this->V) {
            id = n.first;
            aux = this->reaching[id][0];

            // Obtenemos la entrada del bloque.
            this->reaching[id][0] = {};

            // Calculamos la union de los predecesores.
            for (uint64_t u_id : this->Einv[id]) {
                this->reaching[id][0] = mapUnion(
                    this->reaching[id][0], 
                    this->reaching[u_id][1]
                );
            }
            // Agregamos los llamadores en caso de ser necesario
            if (this->caller.count(id-1) > 0) {
                this->reaching[id][0] = mapUnion(
                    this->reaching[id][0], 
                    this->reaching[this->caller[id-1]][1]
                );
            }

            change = change || this->reaching[id][0] != aux;

            // Calculamos el out del bloque
            aux = this->reaching[id][1];
            this->reaching[id][1] = F_B(this->V[id], this->reaching[id][0]);

            // Verificamos si hubo un cambio
            change = change || this->reaching[id][1] != aux;
        }
    }
}

/*
 * Obtenemos la definicion mas temprana de una variable.
 */
string getEarlyDefinition(
    FlowGraph *fg, 
    string var, 
    map<string, set<pair<uint64_t, uint64_t>>> reaching
) {
    pair<uint64_t, uint64_t> blockInstr;
    T_Instruction instr;
    while (reaching.count(var) > 0 && reaching[var].size() == 1) {
        blockInstr = *reaching[var].begin();
        instr = fg->V[blockInstr.first]->block[blockInstr.second];

        // Si no se realiza una asignacion directa o se realiza un acceso a memoria, 
        // finalizamos.
        if (
            (instr.id != "assignw" && instr.id != "assignb") || 
            instr.result.is_acc || instr.operands[0].is_acc
        ) {
            break;
        }

        var = instr.operands[0].name;
    }
    return var;
}

T_Instruction replaceOperands(
    FlowGraph *fg, 
    T_Instruction instr, 
    map<string, set<pair<uint64_t, uint64_t>>> reaching
) {
    // Ignoramos las llamadas a funciones
    if (instr.id == "call") return instr;

    if (instr.operands.size() > 0) {
        instr.operands[0].name = getEarlyDefinition(fg, instr.operands[0].name, reaching);
        if (instr.operands[0].is_acc) {
            instr.operands[0].acc = getEarlyDefinition(fg, instr.operands[0].acc, reaching);
        } 
    }
    if (instr.operands.size() > 1) {
        instr.operands[1].name = getEarlyDefinition(fg, instr.operands[1].name, reaching);
    }
    return instr;
}

/*
 * Reemplaza las definiciones por las mas antiguas disponibles y realiza calculos
 * constantes en caso de ser necesario.
 */
void FlowGraph::replaceDefinitions(void) {
    map<string, set<pair<uint64_t, uint64_t>>> in;
    T_Instruction instr;

    for (pair<uint64_t, FlowNode*> n : this->V) {
        in = this->reaching[n.first][0];

        for (uint64_t i = 0; i < n.second->block.size(); i++) {
            instr = n.second->block[i];
            n.second->block[i] = replaceOperands(this, instr, in);

            // Si la instruccion realiza una asignacion no a memoria, entonces matamos la
            // definicion y agregamos una nueva
            if (assignInstructions.count(instr.id) > 0 && ! instr.result.is_acc) {
                in[instr.result.name] = {{n.first, i}};
            }
        }
    }
}

// Definimos como se operan las diversas instrucciones con operadores constantes.

T_Instruction arit(
    T_Instruction instr, 
    int (*intOp) (int, int), 
    float (*floatOp) (float, float)
) {
    string op;

    // Verificamos si uno de los operadores es flotante
    if (instr.operands[0].name[0] == 'f' || instr.operands[1].name[0] == 'f') {
        op = to_string((*floatOp) (stof(instr.operands[0].name), stof(instr.operands[1].name)));
    }
    else {
        op = to_string((*intOp) (stoi(instr.operands[0].name), stoi(instr.operands[1].name)));
    }

    return {"assignw", instr.result, {{op, "", false}}};
}

T_Instruction numberComp(T_Instruction instr, bool (*compOp) (float, float)) {
    bool result = (*compOp) (stof(instr.operands[0].name), stof(instr.operands[1].name));
    string op = result ? "True" : "False";
    return {"assignb", instr.result, {{op, "", false}}};
}

T_Instruction boolean(T_Instruction instr, bool (*boolOp) (bool, bool)) {
    bool left = (instr.operands[0].name == "1") || (instr.operands[0].name == "True");
    bool right = (instr.operands[1].name == "1") || (instr.operands[1].name == "True");
    string op  = (*boolOp) (left, right) ? "True" : "False";
    return {"assignb", instr.result, {{op, "", false}}};
}

T_Instruction minusInstr(T_Instruction instr) {
    string op;

    // Verificamos si uno de los operadores es flotante
    if (instr.operands[0].name[0] == 'f') {
        op = to_string(-(stof(instr.operands[0].name)));
    }
    else {
        op = to_string(-(stoi(instr.operands[0].name)));
    }

    return {"assignw", instr.result, {{op, "", false}}};
}

T_Instruction eq(T_Instruction instr) {
    string op;

    // Verificamos si son numeros
    if ('0' <= instr.operands[0].name[0] && instr.operands[0].name[0] <= '9') {
        op = stof(instr.operands[0].name) == stof(instr.operands[1].name) ? "True" : "False";
    }
    // O caracteres
    else if (instr.operands[0].name[0] == '\'') {
        op = instr.operands[0].name == instr.operands[1].name ? "True" : "False";
    }
    // Y por ultimo booleanos
    else {
        bool left, right;
        left = instr.operands[0].name == "True" || instr.operands[0].name == "1";
        right = instr.operands[1].name == "True" || instr.operands[1].name == "1";
        op = left == right ? "True" : "False";
    }

    return {"assignb", instr.result, {{op, "", false}}};
}

T_Instruction neq(T_Instruction instr) {
    string op;

    // Verificamos si son numeros
    if ('0' <= instr.operands[0].name[0] && instr.operands[0].name[0] <= '9') {
        op = stof(instr.operands[0].name) != stof(instr.operands[1].name) ? "True" : "False";
    }
    // O caracteres
    else if (instr.operands[0].name[0] == '\'') {
        op = instr.operands[0].name != instr.operands[1].name ? "True" : "False";
    }
    // Y por ultimo booleanos
    else {
        bool left, right;
        left = instr.operands[0].name == "True" || instr.operands[0].name == "1";
        right = instr.operands[1].name == "True" || instr.operands[1].name == "1";
        op = left != right ? "True" : "False";
    }

    return {"assignb", instr.result, {{op, "", false}}};
}

template <typename T>  T add(T a, T b)   { return a + b; }
template <typename T>  T sub(T a, T b)   { return a - b; }
template <typename T>  T mult(T a, T b)  { return a * b; }
template <typename T>  T div(T a, T b)   { return a / b; }
int modi(int a, int b)        { return a % b; }
float modf(float a, float b)  { return 0 * a * b; }
bool lt(float a, float b)     { return a < b; }
bool leq(float a, float b)    { return a <= b; }
bool gt(float a, float b)     { return a > b; }
bool geq(float a, float b)    { return a >= b; }
bool orInstr(bool a, bool b)  { return a || b; }
bool andInstr(bool a, bool b) { return a && b; }

map<string, pair<int (*) (int, int), float (*) (float, float)>> aritInstr = {
    {"add" , {&add<int>, &add<float>}},
    {"sub" , {&sub<int>, &sub<float>}},
    {"mult", {&mult<int>, &mult<float>}},
    {"div" , {&div<int>, &div<float>}},
    {"mod" , {&modi, &modf}}
};

map<string, bool (*) (float, float)> numberCompInstr = {
    {"lt"  , &lt},
    {"leq" , &leq},
    {"gt"  , &gt},
    {"geq" , &geq}
};

map<string, bool (*) (bool, bool)> boolInstr = {
    {"or"  , &orInstr},
    {"and" , &andInstr}
};

bool isID(string var) {
    return var[0] == '_' || ('A' <= var[0] && var[0] <= 'z');
}

set<string> valids = {
    "add", "sub", "mult", "div", "mod", "minus",
    "eq", "neq", "lt", "leq", "gt", "geq", "or", "and"
};

/*
 * Realiza las operaciones que son constantes, convirtiendola en asignaciones. Retorna
 * si se realizo algun cambio.
 */
void FlowGraph::constantPropagation(void) {
    this->reachingDefinitions();

    T_Instruction instr;
    bool change = true;

    while (change) {
        this->replaceDefinitions();

        change = false;
        for (pair<uint64_t, FlowNode*> n : this->V) {
            for (uint64_t i = 0; i < n.second->block.size(); i++) {
                instr = n.second->block[i];

                // Verificamos que la instruccion es una operacion valida con operadores
                // constantes
                if (
                    valids.count(instr.id) > 0 && 
                    ! isID(instr.operands[0].name) && 
                    (instr.operands.size() < 2 || ! isID(instr.operands[1].name))
                ) {
                    // Verificamos que no sea una division entre 0
                    if (instr.id == "div" && instr.operands[1].name == "0") {
                        continue;
                    }
                    
                    change = true;

                    if (aritInstr.count(instr.id) > 0) {
                        n.second->block[i] = arit(
                            instr, 
                            aritInstr[instr.id].first, 
                            aritInstr[instr.id].second
                        );
                    }
                    else if (numberCompInstr.count(instr.id) > 0) {
                        n.second->block[i] = numberComp(instr, numberCompInstr[instr.id]);
                    }
                    else if (boolInstr.count(instr.id) > 0) {
                        n.second->block[i] = boolean(instr, boolInstr[instr.id]);
                    }
                    else if (instr.id == "minus") {
                        n.second->block[i] = minusInstr(instr);
                    }
                    else if (instr.id == "eq") {
                        n.second->block[i] = eq(instr);
                    }
                    else if (instr.id == "neq") {
                        n.second->block[i] = neq(instr);
                    }
                }
            }
        }
    }
}
#pragma once 

#include <map>
#include <set>
#include <queue>
#include <stack>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>

using namespace std;

/*
 * Representacion de op A B
 */
struct Expression {
    string op;
    string A;
    string B;

    bool operator==(const Expression& e) const {
        return op == e.op && A == e.A && B == e.B; 
    }
    bool operator<(const Expression& e) const {
        return (op + A + B) < (e.op + e.A + e.B);
    }
    friend std::ostream& operator<<(std::ostream& o, Expression e) {
        o << e.op << " " << e.A << " " << e.B;
        return o;
    }
};

struct T_Variable
{
    string name;
    string acc;
    bool is_acc;

    bool operator==(const T_Variable& v) const {
        return name == v.name && acc == v.acc && is_acc == v.is_acc; 
    }
    bool operator!=(const T_Variable& v) const {
        return ! (*this == v) ; 
    }
    bool operator<(const T_Variable& v) const {
        return (name + (is_acc ? acc : "")) < (v.name + (v.is_acc ? v.acc : ""));
    }
};

struct T_Instruction
{
    string id;
    T_Variable result;
    vector<T_Variable> operands;
    bool operator==(const T_Instruction& instr) const {
        return id == instr.id && result == instr.result && operands == instr.operands; 
    }
    bool operator<(const T_Instruction& instr) const {
        if (id != instr.id) return id < instr.id;
        if (result != instr.result) return result < instr.result;
        return operands < instr.operands;
    }
    friend std::string to_string(const T_Instruction& instr) {
        string result = "";
        result += instr.id + " " + instr.result.name;
        if (instr.result.is_acc) result += "[" + instr.result.acc + "]";
        result += " ";

        if (instr.operands.size() > 0) {
            result += instr.operands[0].name;
            if (instr.operands[0].is_acc) result += "[" + instr.operands[0].acc + "]";
            result += " ";
        }

        if (instr.operands.size() > 1) {
            result += instr.operands[1].name;
        }

        return result;
    }
    friend std::ostream& operator<<(std::ostream& o, T_Instruction instr) {
        o << to_string(instr);
        return o;
    }
};

struct T_Function
{
    string name;
    uint64_t id;
    uint64_t size;
    vector<T_Instruction> instructions;
    map<string, uint64_t> labels2instr;
    set<string> labels_leaders;
    set<uint64_t> leaders = {0};
    vector<uint64_t> vec_leaders;
};

struct T_Loop
{
    uint64_t header;
    set<uint64_t> blocks;
    set<uint64_t> exits;
    bool hasPreHeader = false;
    uint64_t preHeader;
};

class FlowNode {
    public:
        // Identificador del bloque.
        uint64_t id;
        // Indica si es el bloque inicial de una funcion.
        bool is_function;
        // En caso de ser asi, obtenemos el ID y la memoria necesaria de la funcion.
        uint64_t function_id;
        uint64_t function_size;
        // Nodo a partir del cual ya no forman parte de la funcion.
        uint64_t function_end;
        vector<T_Instruction> block = {};

        FlowNode(uint64_t id, uint64_t leader, T_Function *function, bool is_function);
        FlowNode(uint64_t id, bool is_function);

        // Funcion del analisis de flujo aplicada sobre el bloque entero.
        template <typename T>
        set<T> F(
            map<string, set<T> (*) (set<T>, T_Instruction)> functions, 
            set<T> in,
            bool forward
        );

        string getName(void);
        void print(void);
        void prettyPrint(void);
};

class FlowGraph {
    public:
        // G = (V,E)
        map<uint64_t, FlowNode*> V;
        map<uint64_t, set<uint64_t>> E;
        // Arcos inversos
        map<uint64_t, set<uint64_t>> Einv;
        // Funciones
        map<string, uint64_t> F;
        // Relaciones llamador/llamado
        map<uint64_t, uint64_t> caller;
        map<uint64_t, set<uint64_t>> called;
        // Direcciones de memoria (variables) definidas estaticamente
        set<string> staticVars;
        // Cota superior de los ID de los bloques.
        uint64_t lastID;

        // Conjuntos del analisis de flujo.
        map<uint64_t, vector<map<string, set<pair<uint64_t, uint64_t>>>>> reaching;
        map<uint64_t, vector<set<string>>> live;
        set<Expression> expressions;
        map<uint64_t, set<Expression>> use_B;
        map<uint64_t, vector<set<Expression>>> anticipated;
        map<uint64_t, vector<set<Expression>>> available;
        map<uint64_t, vector<set<Expression>>> earliest;
        map<uint64_t, vector<set<Expression>>> postponable;
        map<uint64_t, vector<set<Expression>>> latest;
        map<uint64_t, vector<set<Expression>>> used;
        map<uint64_t, vector<set<uint64_t>>> dominators;
        map<uint64_t, T_Loop> naturalLoops;

        FlowGraph(vector<T_Function*> functions, set<string> staticVars);

        void insertArc(uint64_t u, uint64_t v);
        void deleteBlock(uint64_t id);
        uint64_t makeSubGraph(T_Function *function, uint64_t init_id);
        void print(void);
        vector<FlowNode*> getOrderedBlocks(void); 
        void prettyPrint(void);

        // ==================== ANALISIS DE FLUJO ==================== //
        // Algoritmo de analisis de flujo generico.
        template <typename T>
        map<uint64_t, vector<set<T>>> flowAnalysis(
            map<uint64_t, vector<set<T>>> (*init) (FlowGraph*),
            set<T> (*initEntryOut) (FlowGraph*),
            set<T> (*initExitIn) (FlowGraph*),
            set<T> (*preprocessing) (FlowGraph*, uint64_t, set<T>),
            set<T> (*postprocessing) (FlowGraph*, uint64_t, set<T>),
            map<string, set<T> (*) (set<T>, T_Instruction)> functions,
            bool intersection,
            bool forward
        );
        template <typename T>
        void flowPrint(map<uint64_t, vector<set<T>>> sets);

        // Analisis de flujo para definiciones vigentes.
        void reachingDefinitions(void);
        void replaceDefinitions(void);
        void constantPropagation(void);

        // Analisis de flujo para variables vivas.
        void liveVariables(void);
        void deleteDeadVariables(void);

        // Analisis de flujo para lazy code motion.
        void computeUseB(void);
        void anticipatedDefinitions(void);
        void availableDefinitions(void);
        void earliestDefinitions(void);
        void postponableDefinitions(void);
        void latestDefinitions(void);
        void usedDefinitions(void);
        void lazyCodeMotion(void);

        // Analisis de flujo para ciclos.
        void computeDominators(void);
        void computNaturalLoops(void);
        void invariantDetection(void);
};


/*
 * Funcion generica F_B 
 */
template <typename T>
set<T> FlowNode::F(
    map<string, set<T> (*) (set<T>, T_Instruction)> functions, 
    set<T> entry,
    bool forward
) {
    if (forward) {

        set<T> out = entry;
        for (T_Instruction instr : this->block) {
            // Aplica la funcion correspondiente al ID de cada instruccion en el bloque.
            out = (*functions[instr.id]) (out, instr);
        }
        return out;
    }
    else {
        set<T> in = entry;
        T_Instruction instr;
        for (int i = this->block.size()-1; i >= 0; i--) {
            // Aplica la funcion correspondiente al ID de cada instruccion en el bloque.
            instr = this->block[i];
            in = (*functions[instr.id]) (in, instr);
        }
        return in;
    }
}

template <typename T>
set<T> setUnion(set<T> U, set<T> V) {
    if (U.size() > V.size()) {
        for (T e : V) U.insert(e);
        return U;
    }
    else {
        for (T e : U) V.insert(e);
        return V;
    }
}

template <typename T>
set<T> setIntersec(set<T> U, set<T> V) {
    set<T> W;

    if (U.size() > V.size()) {
        for (T e : V)  {
            if (U.count(e) > 0) {
                W.insert(e);
            }
        }
    }
    else {
        for (T e : U)  {
            if (V.count(e) > 0) {
                W.insert(e);
            }
        }
    }
    
    return W;
}

template <typename T>
set<T> setSub(set<T> U, set<T> V) {
    if (U.size() > V.size()) {
        for (T e : V) {
            U.erase(e);
        }
        return U;
    } 
    else {
        set<T> W;

        for (T e : U) {
            if (V.count(e) == 0) {
                W.insert(e);
            }
        }

        return W;
    }
}


/*
 * Algoritmo generico de analisis de flujo.
 *
 * Parametros:
 * -----------
 *      * map<uint64_t, vector<set<T>>> (*init) (FlowGraph*) 
 *          Funcion que inicializa los conjuntos dado el grafo de flujo.
 * 
 *      * set<T> (*initEntryOut) (FlowGraph*)
 *          Funcion que inicializa el conjunto OUT del nodo ENTRY.
 * 
 *      * set<T> (*initExitIn) (FlowGraph*)
 *          Funcion que inicializa el conjunto IN del nodo EXIT.
 * 
 *      * set<T> (*preprocessing) (FlowGraph*, uint64_t, set<T>)
 *          Funcion que realiza un preprocesamiento al conjunto antes de pasarlo por F_B.
 * 
 *      * map<string, flowFunction> functions
 *          Mapea los ID de las instrucciones del TAC a la funcion transformadora 
 *          correspondiente.
 * 
 *      * bool intersection 
 *          Indica si se realizara una interseccion o una union entre conjuntos al saltar
 *          entro bloques.
 * 
 *      * bool forward
 *          Indica si el analisis es hacia adelante o hacia atras.
 * 
 * Returns:
 * --------
 *      * map<uint64_t, vector<set<T>>>
 *          Diccionario desde los ID de los bloques a sus correspondientes par de 
 *          conjuntos IN, OUT.
 */
template <typename T>
map<uint64_t, vector<set<T>>> FlowGraph::flowAnalysis(
    map<uint64_t, vector<set<T>>> (*init) (FlowGraph*),
    set<T> (*initEntryOut) (FlowGraph*),
    set<T> (*initExitIn) (FlowGraph*),
    set<T> (*preprocessing) (FlowGraph*, uint64_t, set<T>),
    set<T> (*postprocessing) (FlowGraph*, uint64_t, set<T>),
    map<string, set<T> (*) (set<T>, T_Instruction)> functions,
    bool intersection,
    bool forward
) {
    // Inicializamos el IN y OUT de cada bloque.
    map<uint64_t, vector<set<T>>> sets = (*init)(this);
    // Inicializamos el OUT e IN de ENTRY y EXIT respectivamente.
    set<T> entry_out = (*initEntryOut)(this);
    set<T> exit_in = (*initExitIn)(this);
    set<T> aux;
    set<uint64_t> R;

    bool change = true, first;

    vector<uint64_t> ids;
    for (pair<uint64_t, FlowNode*> n : this->V) {
        ids.push_back(n.first);
    }
    if (! forward) {
        reverse(ids.begin(), ids.end());
    }

    // Ejecutamos hasta alcanzar un punto fijo
    while (change) {
        change = false;
        for (uint64_t id : ids) {
            aux = sets[id][!forward];

            // Obtenemos la entrada del bloque.
            sets[id][!forward] = set<T>();
            // Calculamos la union/intercepcion de los predecesores/sucesores.
            R = forward ? this->Einv[id] : this->E[id];
            if (intersection) {
                first = true;
                for (uint64_t u_id : R) {
                    if (first) {
                        sets[id][!forward] = sets[u_id][forward];
                        first = false;
                    }
                    else {
                        sets[id][!forward] = setIntersec<T>(sets[id][!forward], sets[u_id][forward]);
                    }
                }
                // Agregamos los llamador o llamadores en caso de ser necesario
                if (forward && this->caller.count(id-1) > 0) {
                    sets[id][0] = setIntersec<T>(sets[id][0], sets[this->caller[id-1]][1]);
                }
                else if (! forward && this->caller.count(id) > 0) {
                    sets[id][1] = setIntersec<T>(sets[id][1], sets[this->caller[id]][0]);
                }

                // Si es el primer nodo del grafo y estamos en forward
                if ((id == 0 || this->V[id]->is_function) && forward) {
                    sets[id][0] = setIntersec<T>(sets[id][0], entry_out);
                }
                // En cambio, si es un nodo final y estamos en backward
                else if (! forward && this->Einv.count(id) == 0) {
                    sets[id][1] = setIntersec<T>(sets[id][1], exit_in);
                }
            }
            else {
                for (uint64_t u_id : R) {
                    sets[id][!forward] = setUnion<T>(sets[id][!forward], sets[u_id][forward]);
                }
                // Agregamos los llamador o llamadores en caso de ser necesario
                if (forward && this->caller.count(id-1) > 0) {
                    sets[id][0] = setUnion<T>(sets[id][0], sets[this->caller[id-1]][1]);
                }
                else if (! forward && this->caller.count(id) > 0) {
                    sets[id][1] = setUnion<T>(sets[id][1], sets[this->caller[id]][0]);
                }

                // Si es el primer nodo del grafo y estamos en forward
                if ((id == 0 || this->V[id]->is_function) && forward) {
                    sets[id][0] = setUnion<T>(sets[id][0], entry_out);
                }
                // En cambio, si es un nodo final y estamos en backward
                else if (! forward && this->Einv.count(id) == 0) {
                    sets[id][1] = setUnion<T>(sets[id][1], exit_in);
                }
            }
            change = change || sets[id][!forward] != aux;

            // Calculamos el out del bloque
            aux = sets[id][forward];
            sets[id][forward] = preprocessing(this, id, sets[id][!forward]);
            sets[id][forward] = this->V[id]->F<T>(functions, sets[id][forward], forward);
            sets[id][forward] = postprocessing(this, id, sets[id][forward]);

            // Verificamos si hubo un cambio
            change = change || sets[id][forward] != aux;
        }
    }

    return sets;
}


/*
 * Imprime de forma bonita el resultado de un analisis de flujo.
 */
template <typename T>
void FlowGraph::flowPrint(map<uint64_t, vector<set<T>>> sets) {
    for (pair<uint64_t, FlowNode*> n : this->V) {
        // Nombre del nodo.
        if (n.second->is_function) {
            string size = to_string(n.second->function_size);
            cout << "\n\033[1;3;34m" << n.second->getName() << " (" + size << "):\033[0m\n";
        }
        else {
            cout << "\033[1;3m" << n.second->getName() << ":\033[0m\n";
        }

        // Predecesores del nodo.
        cout << "    \033[1mPREDS:\033[0m {";
        for (uint64_t v : this->Einv[n.first]) {
            cout << this->V[v]->getName() << ", ";
        }
        if (n.first == 0 || this->V[n.first]->is_function) {
            cout << "ENTRY, ";
        }
        cout << "}\n";

        // IN del nodo.
        cout << "    \033[1mIN:\033[0m    {";
        for (T elem : sets[n.first][0]) {
            cout << elem << ", ";
        }
        cout << "}\n";

        // BLOQUE
        cout << "    \033[1mBLOCK:\033[0m\n";
        string space, max_instr = "assignw";
        for (T_Instruction instr : n.second->block) {
            space = string(max_instr.size() - instr.id.size() + 1, ' ');
            
            cout << "        | \033[3m" << instr.id << "\033[0m" 
                << space << instr.result.name;

            if (instr.result.is_acc)
                cout << "[" << instr.result.acc << "]";
            cout << " ";

            for (T_Variable operand : instr.operands) {
                cout << operand.name;
                if (operand.is_acc)
                    cout << "[" << operand.acc << "]";
                cout << " ";
            }
            cout << "\n";
        }

        // OUT del nodo.
        cout << "    \033[1mOUT:\033[0m   {";
        for (T elem : sets[n.first][1]) {
            cout << elem << ", ";
        }
        cout << "}\n";

        // Sucesores del nodo.
        cout << "    \033[1mSUCCS:\033[0m {";
        if (this->E.count(n.first) == 0 || this->E[n.first].size() == 0) {
            cout << "EXIT, ";
        }
        else {
            for (uint64_t v : this->E[n.first]) {
                cout << this->V[v]->getName() << ", ";
            }
        }
        cout << "}\n\n";
    }
}

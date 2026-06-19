#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// rozne typy uzlov pre ast 
typedef enum { 
    VAR, NOT, AND, OR 
} NodeType;

// stuct pre jeden ast node
typedef struct ASTNode {
    NodeType type;
    char var;               
    struct ASTNode* left;
    struct ASTNode* right;
} ASTNode;

// stuct pre jeden bdd node
typedef struct BDDNode {
    int var_index;          // index premennej
    struct BDDNode* low;    // vetva pre 0
    struct BDDNode* high;   // vetva pre 1
    int is_terminal;        // ak je listom
    char value;             // '0' alebo '1' ak is_terminal
} BDDNode;

//struct pre jeden cely bdd diagram
typedef struct BDD {
   int num_vars;           // pocet premennych
   BDDNode* root;          // ukazovatel na root
} BDD;

// globalne premenne pre parsovanie
const char* input;
int pos;

// globalne premenne pre hash tabulku
#define MAX_UNIQUE_NODES 100000
BDDNode* unique_table[MAX_UNIQUE_NODES];
int unique_count;

// listy bdd - is_terminal
BDDNode* TERMINAL_0 = NULL;
BDDNode* TERMINAL_1 = NULL;

// ast node 
static ASTNode* create_node(NodeType type, char var, ASTNode* left, ASTNode* right) {
    ASTNode* n = malloc(sizeof(ASTNode));
    n->type = type; 
    n->var = var; 
    n->left = left; 
    n->right = right;
    return n;
}

//funkcie na parsovanie
char peek_char() { 
    return input[pos]; 
}
char get_char()  { 
    return input[pos++]; 
}
int is_whitespace(char c) {
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}
void skip_ws() {
    while (is_whitespace(peek_char())) {
        get_char();
    }
}
int is_var(char c) {
    return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
}

//dekleracia funkcie
ASTNode* parse_expr();

ASTNode* parse_primary() {
    skip_ws();
    if (peek_char() == '(') {
        get_char();
        ASTNode* e = parse_expr();
        if (peek_char() == ')') get_char();
        return e;
    } else if (is_var(peek_char())) {
        return create_node(VAR, get_char(), NULL, NULL);
    }
    return NULL;
}

// najvyssia priorita not
ASTNode* parse_not() {
    skip_ws();
    if (peek_char() == '!') {
        get_char();
        return create_node(NOT, 0, parse_not(), NULL);
    }
    return parse_primary();
}

//stredna priorita and
ASTNode* parse_and() {
    ASTNode* left = parse_not();
    while (1) {
        skip_ws();
        if (peek_char() == '&') {
            get_char();
            ASTNode* right = parse_not();
            left = create_node(AND, 0, left, right);
        } else break;
    }
    return left;
}

// najvyssia priorita or 
ASTNode* parse_or() {
    ASTNode* left = parse_and();
    while (1) {
        skip_ws();
        if (peek_char() == '|') {
            get_char();
            ASTNode* right = parse_and();
            left = create_node(OR, 0, left, right);
        } else break;
    }
    return left;
}

ASTNode* parse_expr() {
    return parse_or();
}

// ast vysledky vyrazu
int ast_result(ASTNode* node, const char* inputs, const char* var_order) {
    if (!node) return -1;
    switch (node->type) {
        case VAR:
            if (node->var == '0') return 0;
            if (node->var == '1') return 1;
            else{ // ak uzol nie je terminalny
                const char* p = strchr(var_order, node->var); //ukazovatel na prvy vyskyt znaku
                int index = p - var_order; // index znaku
                return inputs[index] - '0'; //znak 0 alebo 1 sa odcita od cisla 0 
            }
        case NOT:
            return !ast_result(node->left, inputs, var_order);
        case AND:
            return ast_result(node->left, inputs, var_order) & ast_result(node->right, inputs, var_order);
        case OR:
            return ast_result(node->left, inputs, var_order) | ast_result(node->right, inputs, var_order);
        default:
            return -1;
    }
}

ASTNode* replace_var(ASTNode* node, char var, int value) {
    if (!node) return NULL;
    if (node->type == VAR) {
        if (node->var == var)
            return create_node(VAR, value ? '1' : '0', NULL, NULL);
        else{
            return create_node(VAR, node->var, NULL, NULL);
        }
    }
    ASTNode* L = replace_var(node->left, var, value);
    ASTNode* R = replace_var(node->right, var, value);
    return create_node(node->type, 0, L, R);
}

BDDNode* make_bdd_node(int var_index, BDDNode* low, BDDNode* high) {
    if (low == high) return low; //pokial uz existuje taka vetva nevytvarame novu
    for (int i = 0; i < unique_count; ++i) {
        BDDNode* n = unique_table[i];
        if (!n->is_terminal
         && n->var_index == var_index
         && n->low == low
         && n->high == high)
            return n;
    }
    BDDNode* n = malloc(sizeof(BDDNode));
    n->var_index = var_index;
    n->low  = low;
    n->high = high;
    n->is_terminal = 0;
    n->value = 0;
    unique_table[unique_count++] = n;
    return n;
}

// bdd z ast
BDDNode* build_bdd_from_ast(ASTNode* ast, const char* var_order, int depth, int num_vars) {
    if (depth == num_vars) {
        int v = ast_result(ast, "", var_order);
        if (v == 1){
            return TERMINAL_1;
        }
        else{
            return TERMINAL_0;
        }
    }
    char var = var_order[depth]; // dana premenna
    //pre kazdy low a high vyvorime node
    ASTNode* a0 = replace_var(ast, var, 0);
    ASTNode* a1 = replace_var(ast, var, 1);
    BDDNode* low  = build_bdd_from_ast(a0, var_order, depth+1, num_vars);
    BDDNode* high = build_bdd_from_ast(a1, var_order, depth+1, num_vars);
    return make_bdd_node(depth, low, high);
}

// bdd create funkcia
BDD* BDD_create(const char* bfunkcia, const char* poradie) {
    // inicializacia terminalych nodes
    if (!TERMINAL_0) {
        TERMINAL_0 = malloc(sizeof(BDDNode));
        TERMINAL_0->is_terminal = 1; TERMINAL_0->value = '0';
        TERMINAL_0->low = TERMINAL_0->high = NULL;
        TERMINAL_1 = malloc(sizeof(BDDNode));
        TERMINAL_1->is_terminal = 1; TERMINAL_1->value = '1';
        TERMINAL_1->low = TERMINAL_1->high = NULL;
    }
    unique_count = 0; //pocet nodes po redukcii
    input = bfunkcia; 
    pos = 0;
    ASTNode* ast = parse_expr(); //ast strom
    BDD* bdd = malloc(sizeof(BDD));
    bdd->num_vars = strlen(poradie);
    bdd->root = build_bdd_from_ast(ast, poradie, 0, bdd->num_vars);
    return bdd;
}

int cound_bdd_nodes(int num_vars) {
    return (1 << num_vars); // 2^N uzlov
}

double reduction(int full_node_count) {
    int reduced_node_count = unique_count; 
    return 100.0 * (full_node_count - reduced_node_count) / full_node_count;
}

void print_bdd(BDDNode* node, int indent) {
    if (!node) return;
    for (int i = 0; i < indent; ++i) printf("  ");
    printf("[%p] ", (void*)node);
    if (node->is_terminal) {
        printf("Terminal: %c\n", node->value);
    } else {
        printf("Var index: %d\n", node->var_index);
        for (int i = 0; i < indent; ++i) printf("  ");
        printf("Low (0):\n");
        print_bdd(node->low, indent+1);
        for (int i = 0; i < indent; ++i) printf("  ");
        printf("High (1):\n");
        print_bdd(node->high, indent+1);
    }
}

void through_bdd(BDDNode* node, int* count) {
    if (!node) return;
    (*count)++;
    through_bdd(node->low, count);
    through_bdd(node->high, count);
}

int count_bdd_nodes(BDD* bdd) {
    int count = 0;
    through_bdd(bdd->root, &count);
    return count;
}

char BDD_use(BDD* bdd, const char* vstup) {
    BDDNode* n = bdd->root;
    while (!n->is_terminal) {
        int i = n->var_index;
        if (vstup[i] == '0') n = n->low;
        else if (vstup[i] == '1') n = n->high;
        else return '?';
    }
    return n->value;
}

void test_bdd_use(const char* expr, const char* var_order) {
    FILE *file = fopen("test_results.txt", "a");  
    if (!file) {
        printf("Chyba pri otváraní súboru na zápis!\n");
        return;
    }
    input = expr; 
    pos = 0;
    ASTNode* ast = parse_expr();
    BDD* bdd = BDD_create(expr, var_order);
    int n = strlen(var_order); 
    int total = 1 << n;
    char buf[32];
    for (int mask = 0; mask < total; ++mask) {
        for (int i = 0; i < n; ++i) {
            buf[i] = ((mask >> (n - 1 - i)) & 1) ? '1' : '0';
        }
        buf[n] = '\0';
        char r1 = BDD_use(bdd, buf);
        int ev = ast_result(ast, buf, var_order);
        char r2 = (ev < 0 ? '?' : '0' + ev);
        if (r1 != r2) {
            fprintf(file, "Chyba: Test %s → BDD: %c, AST: %c\n", buf, r1, r2);
        } else {
            fprintf(file, "Test %s → OK\n", buf);
        }
    }
    fclose(file); 
}

void swap(char* a, char* b) {
    char temp = *a;
    *a = *b;
    *b = temp;
}

void generate_random_permutation(char* arr, int n) {
    for (int i = n - 1; i > 0; --i) {
        int j = rand() % (i + 1);
        swap(&arr[i], &arr[j]);
    }
}

void best_order_test(const char* expr, char* arr, int n, int* best_count, BDD** best_bdd, char* best_order) {
    for (int permutation_count = 0; permutation_count < n; ++permutation_count) {
        generate_random_permutation(arr, strlen(arr));
        BDD* bdd = BDD_create(expr, arr);
        int nodes = count_bdd_nodes(bdd);
        if (*best_bdd == NULL) {
            *best_bdd = bdd;            
            *best_count = nodes;    
            strcpy(best_order, arr); 
        }
        if(nodes <*best_count){
            *best_bdd = bdd;            
            *best_count = nodes;     
            strcpy(best_order, arr);
        }
    }
}

BDD* BDD_create_with_best_order(const char* expr, const char* var_order) {
    int n = strlen(var_order);
    char* arr = malloc(n + 1);
    strcpy(arr, var_order);
    arr[n] = '\0';
    int best_count = 0;  
    BDD* best_bdd = NULL;
    char best_order[32];
    best_order[0] = '\0'; 

    int max_permutations = n;  // pocet permutacii podla poctu premennych

    best_order_test(expr, arr ,max_permutations, &best_count, &best_bdd, best_order);
    free(arr);
    return best_bdd;  
}

void generate_random_dnf(char* buffer, int max_len, int num_vars, int num_terms) {
    const char vars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    int len = 0;
    for (int t = 0; t < num_terms; ++t) {
        if (t > 0 && len < max_len - 3) {
            buffer[len++] = ' ';
            buffer[len++] = '|';
            buffer[len++] = ' ';
        }
        buffer[len++] = '(';
        int used[26] = {0};
        int num_in_term = 1 + rand() % num_vars;  

        for (int i = 0; i < num_in_term; ++i) {
            int var_idx;
            do {
                var_idx = rand() % num_vars;
            } while (used[var_idx]);
            used[var_idx] = 1;

            if (i > 0 && len < max_len - 3) {
                buffer[len++] = ' ';
                buffer[len++] = '&';
                buffer[len++] = ' ';
            }

            if (rand() % 2 && len < max_len - 1) {
                buffer[len++] = '!';
            }

            if (len < max_len - 1)
                buffer[len++] = vars[var_idx];
        }

        if (len < max_len - 1)
            buffer[len++] = ')';
    }
    buffer[len] = '\0';
}

int main() {
    FILE *file = fopen("results.txt", "w");
    if (!file) {
        printf("Chyba pri otváraní súboru na zápis!\n");
        return 1;
    }
    int num_vars = 8;      // pocet premennych
    int num_terms = 3;     // pocet termov

    double total_time_bdd_create = 0;
    double total_time_best_order = 0;
    double total_reduction_percentage = 0;
    double total_time_bdd_use = 0;
    for (int i = 0; i < 100; ++i) {
        fprintf(file, "===========\n");
        printf("Test %d\n", i + 1);
        fprintf(file, "Test %d\n", i + 1);

        BDD* best_bdd = NULL;
        int best_count = 0;

        //generovanie var_order podla abecedy
        char var_order[27]; 
        for (int j = 0; j < num_vars; ++j)
            var_order[j] = 'A' + j;
        var_order[num_vars] = '\0';

        char expr[4096];  
        generate_random_dnf(expr, sizeof(expr), num_vars, num_terms);

        fprintf(file, "Vygenerovaná DNF Booleovská funkcia\n%s\n\n", expr);

        clock_t start_bdd_create = clock();

        BDD* bdd_basic = BDD_create(expr, var_order);
    
        clock_t end_bdd_create = clock();   
        double time_bdd_create = (double)(end_bdd_create - start_bdd_create) / CLOCKS_PER_SEC;
        total_time_bdd_create += time_bdd_create;
        int nodes_basic = count_bdd_nodes(bdd_basic);
        fprintf(file, "BDD_create: %d uzlov\n", nodes_basic);
        int full_node_count = cound_bdd_nodes(num_vars);
        int reduced_node_count = nodes_basic;

        double reduction_percentage = reduction(full_node_count);
        total_reduction_percentage += reduction_percentage;
        fprintf(file, "Percentuálne zredukovanie: %.2f%%\n", reduction_percentage);

        clock_t start_best_order = clock();
        BDD* bdd_best = BDD_create_with_best_order(expr, var_order);
        clock_t end_best_order = clock();   // Stopneme clock po zavolaní funkcie
        double time_best_order = (double)(end_best_order - start_best_order) / CLOCKS_PER_SEC;
        total_time_best_order += time_best_order;
        int nodes_best = count_bdd_nodes(bdd_best);
        fprintf(file, "BDD_create_with_best_order: %d uzlov\n", nodes_best);

        double reduction = 100.0 * (nodes_basic - nodes_best) / nodes_basic;
        fprintf(file, "Dodatočná redukcia pomocou best_order: %.2f %%\n\n", reduction);
         
        clock_t start_bdd_use = clock();
        test_bdd_use(expr, var_order);
        clock_t end_bdd_use = clock();
        double time_bdd_use = (double)(end_bdd_use - start_bdd_use) / CLOCKS_PER_SEC;
        total_time_bdd_use += time_bdd_use;
        
    }
    double avg_time_bdd_create = total_time_bdd_create / 100;
    double avg_time_best_order = total_time_best_order / 100;
    double avg_reduction_percentage = total_reduction_percentage / 100;
    double avg_time_bdd_use = total_time_bdd_use / 100;

    FILE *avg = fopen("avg_time_and_reduction.txt", "a");

    fprintf(avg, "\nTest for %d number of vars.\n", num_vars);
    fprintf(avg, "Priemerný čas pre BDD_create: %.6f sekúnd\n", avg_time_bdd_create);
    fprintf(avg, "Priemerný čas pre BDD_create_with_best_order: %.6f sekúnd\n", avg_time_best_order);
    fprintf(avg, "Priemerná percentuálna miera zredukovania: %.6f\n", avg_reduction_percentage);
    fprintf(avg, "Priemerný čas pre bdd_use: %.6f sekúnd\n", avg_time_bdd_use);

    fclose(file);
    fclose(avg);
    return 0;
}
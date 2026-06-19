# Binary Decision Diagram (BDD) Implementation in C

## Overview

This project implements a **Binary Decision Diagram (BDD)** generator and evaluator for Boolean functions written in Disjunctive Normal Form (DNF).

The program:

* Parses Boolean expressions into an Abstract Syntax Tree (AST)
* Constructs a reduced Binary Decision Diagram (BDD)
* Eliminates redundant nodes using reduction rules
* Evaluates Boolean functions through BDD traversal
* Tests correctness against direct AST evaluation
* Searches for better variable orderings to minimize the BDD size
* Measures performance and reduction efficiency

---

## Features

* Boolean expression parser
* AST (Abstract Syntax Tree) generation
* Reduced Ordered Binary Decision Diagram (ROBDD)
* Unique table for node sharing
* Automatic BDD reduction
* Boolean function evaluation using BDD
* Random DNF expression generation
* Variable ordering optimization
* Performance benchmarking
* Correctness testing

---

## Supported Operators

| Operator | Meaning  |    |
| -------- | -------- | -- |
| `!`      | NOT      |    |
| `&`      | AND      |    |
| `        | `        | OR |
| `( )`    | Grouping |    |

### Example

```text
(A & B) | (!C & D)
```

---

## Project Architecture

### 1. Parsing

The input Boolean expression is converted into an AST.

Example:

```text
(A & B) | C
```

becomes:

```text
      OR
     /  \
   AND   C
   / \
  A   B
```

---

### 2. AST Evaluation

The AST can be directly evaluated for any input combination.

Function:

```c
ast_result()
```

---

### 3. BDD Construction

The algorithm recursively:

1. Selects a variable according to the specified ordering.
2. Creates two subproblems:

   * variable = 0
   * variable = 1
3. Recursively builds child nodes.
4. Merges equivalent nodes.

Function:

```c
BDD_create()
```

---

### 4. Reduction Rules

The implementation applies two standard ROBDD reduction rules:

#### Remove Redundant Nodes

If:

```text
low == high
```

then the node is removed.

#### Merge Equivalent Nodes

Nodes having:

* same variable
* same low branch
* same high branch

are shared through a unique table.

Function:

```c
make_bdd_node()
```

---

## Variable Ordering Optimization

BDD size strongly depends on variable ordering.

The project implements:

```c
BDD_create_with_best_order()
```

which:

* generates random permutations of variable orders
* builds multiple BDDs
* selects the smallest one

This demonstrates the impact of variable ordering on BDD reduction.

---

## BDD Evaluation

Once constructed, a BDD can be evaluated efficiently:

```c
BDD_use()
```

Traversal follows:

* Low branch for value `0`
* High branch for value `1`

until reaching a terminal node.

---

## Random Boolean Function Generator

The project automatically generates random DNF expressions.

Example:

```text
(A & !B & C) | (!A & D)
```

Function:

```c
generate_random_dnf()
```

Parameters:

* Number of variables
* Number of terms

---

## Testing

Correctness is verified by comparing:

```text
BDD result
vs
AST result
```

for every possible input assignment.

Function:

```c
test_bdd_use()
```

Results are written into:

```text
test_results.txt
```

---

## Performance Measurements

For each generated Boolean function the program records:

* BDD construction time
* Best-order search time
* Reduction percentage
* Evaluation time

The experiment is repeated:

```text
100 times
```

and average values are stored.

---

## Output Files

### results.txt

Contains detailed information for each test:

```text
Test 1
Generated Boolean function
BDD size
Reduction percentage
Best-order BDD size
Additional reduction
```

### test_results.txt

Contains correctness verification:

```text
Test 0001 → OK
Test 0010 → OK
...
```

### avg_time_and_reduction.txt

Contains average statistics:

```text
Average BDD_create time
Average BDD_create_with_best_order time
Average reduction percentage
Average BDD_use time
```

---

## Compilation

Compile using GCC:

```bash
gcc main.c -o bdd
```

Recommended flags:

```bash
gcc -O2 main.c -o bdd
```

---

## Running

```bash
./bdd
```

The program automatically:

1. Generates random Boolean functions
2. Builds BDDs
3. Tests correctness
4. Measures performance
5. Stores results into output files

---

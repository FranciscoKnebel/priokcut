# Priority K-cut Algorithm Implementation

Given an AIG graph file in the AIGER format, the program evaluates the priority K-cuts for all the vertices. You can set
the maximum number of cuts (`k`) for each vertex and the number of inputs (`i`) for each cut.

The algorithm manages the memory in a very efficient manner.

### Usage
1. Compile the source code:
```
g++ priokcuts.cpp -o priokcuts
```
2. Call the program, passing the AIG file as an argument:
```
./priokcuts aiger/example.aag
```
There are some basic options. You can:
* Set the number of k-cuts
```
./priokcuts aiger/example.aag -k 3
```
* Set the maximum number of inputs for each cut
```
./priokcuts aiger/example.aag -i 6
```
* Display the results on screen (this slow down the execution time for large graphs)
```
./priokcuts aiger/example.aag -d
```

### Limitations
The program do not support AIG graphs with latches yet.

###	Memory usage

Running the program to compute the `k` cuts with `i` inputs for an AIG graph with `M` vertices and `E` edges uses:

* `4*E` bytes to store the edges
* `16*M` bytes to store the vertices
* `(4+4*i)*k*M` bytes for the cuts
* `4*M` bytes for auxiliary data

### Theoretical capacity

An AIG graph of up to 1.073.741.824 vertices.

### Complexity

Linear, directly proportional to the number of vertices.

### Performance notes

Practical applications of this algorithm can operate on graphs
of up to one million vertices. For these graphs, memory usage is
approximately 50 to 100 MB, depending on the values
of `i` and `k`. In general, the lower the values of `i` and `k`, the lower the memory usage and execution time.
	
For small AIG graphs (up to 1000 vertices), graph data is expected
to fit entirely into the L1 cache of modern processors,
leading to extremely fast executions.

### Licence

You can use the software for scientific or academic purposes only. Commercial use is not allowed.

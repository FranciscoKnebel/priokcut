# Priority K-cut Algorithm Implementation

Given an AIG graph file in the Aiger ASCII format, the program evaluates the priority K-cuts for all the vertices. You can set
the maximum number of cuts (`k`) for each vertex and the number of inputs (`i`) for each cut.

The algorithm manages the memory in a very efficient manner.

### Usage
1. Compile the source code:
```
g++ priokcuts.cpp -o priokcuts
```
2. Call the program, passing the AIG file (in the Aiger ASCII format) as an argument:
```
./priokcuts aiger/example.aag
```
The default value for `k` is 2, and the default value for `i` is 3, but you can set these values:
```
./priokcuts [input-file] [max-cuts] [max-inputs]
```
### Limitations
The program do not support AIG graphs with latches yet.

###	Memory usage

For an AIG graph with `M` vertices and `E` edges, the program will use:

* `4*E` bytes to store the edges in the main memory
* `12*M` bytes for the vertices
* `(4+4*i)*k*M` bytes for the cuts

### Theoretical capacity

An AIG graph of up to 1.073.741.824 vertices.

### Complexity

Not calculated yet, but expected to be quadratic according to the number of vertices.

### Performance notes

Practical applications of this algorithm can operate on graphs
of up to one million vertices. For these graphs, memory usage is
approximately 24 to 240 MB, depending on the values
of max_cuts and max_inputs.
	
For small AIG graphs (up to 1000 vertices), graph data is expected
to fit entirely into the L1 cache of modern processors,
leading to extremely fast executions.

### Licence

You can use the software for scientific or academic purposes only. Commercial use is not allowed.

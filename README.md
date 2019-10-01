# Priority K-cut Algorithm Implementation

Given an AIG file in the AIGER format (binary or ASCII), the program evaluates the priority K-cuts for all the vertices. You can set the maximum number of cuts (`k`) for each vertex and the number of inputs (`i`) for each cut.

### Usage
1. Compile the source code (`-O3` is optional but recommended to enhance execution time)
```
g++ -O3 priokcuts.cpp -o priokcuts
```
2. Call the program. The path of the AIG file is the only required argument. Both binary and ASCII formats are accepted, but binary files runs faster.
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
The program do not support AIGs with latches yet.

### Memory usage

Running the program to compute the `k` cuts with `i` inputs for an AIG graph with `M` uses:

* `16*M` bytes to store the vertices
* `(4+4*i)*k*M` bytes for the cuts
* `4*M` bytes for auxiliary data

For very large graphs, make sure your machine have enough memory!

### Theoretical capacity

An AIG graph of up to 1.073.741.824 vertices.

### Complexity

Linear, directly proportional to the number of vertices.

### Performance notes
The algorithm was tested for very large graphs. In an 8GB RAM Intel Core-i7 machine, the algorithm takes about 1 second to process 1.6 million of vertices with `k = 2` and `i = 6`. In general, the lower the values of `i` and `k`, the lower the memory usage and execution time. However, the rate `execution time / number of vertices` remains constant regardless the values of `k` or `i`.
	
For small AIG graphs (up to 1000 vertices), graph data is expected
to fit entirely into the L1 cache of modern processors,
leading to extremely fast executions (less than 1 ms).

### Licence

You can use the software for scientific or academic purposes only. Commercial use is not allowed.

# Priority K-cut Algorithm Implementation

Given an AIG file in the AIGER format (binary or ASCII), the program evaluates the priority K-cuts for all the vertices. You can set the maximum number of cuts (`p`) stored for each vertex and the number of inputs (`k`) of each cut.

### Usage
1. Build the program:
```
make
```
2. Run the program. The path of the AIG file is the only required argument. Both binary and ASCII formats are accepted, but binary files runs faster.
```
./priokcuts aiger/example.aag
```
There are some basic options. You can:
* Set the number of k-cuts stored for each vertex
```
./priokcuts aiger/example.aag -p 3
```
* Set the maximum number of inputs for each cut
```
./priokcuts aiger/example.aag -k 6
```
* Display the results on screen (this slows down the execution time for large graphs)
```
./priokcuts aiger/example.aag -d
```

### Limitations
The program do not support AIGs with latches yet.

### Memory usage

Running the program to compute `p` cuts for each vertex (each cut with `k` inputs) for an AIG with `M` vertices uses:

* `16*M` bytes to store the vertices
* `4*(k+1)*p*M` bytes to store the cuts
* `4*M`bytes for auxiliary data (worst case), `log2(4*M)` (best case)

For very large graphs (> 50.000.000 vertices), make sure your computer have enough memory!

### Theoretical capacity

An AIG graph of up to 1.073.741.824 vertices.

### Complexity

Linear, directly proportional to the number of vertices.

### Performance notes
The algorithm was tested for very large graphs (> 50.000.000 vertices). In an 8GB RAM Intel Core-i7 machine, the algorithm takes about 1 second to process 2.4 million of vertices with `k = 4` and `p = 2`. In general, the lower the values of `p` and `k`, the lower the memory usage and execution time.

### Licence

You can use the software for scientific or academic purposes only. Commercial use is not allowed.

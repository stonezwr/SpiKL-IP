# SpiKL-IP

This repo the implementation of SpiKL-IP in the resevoir on the LSM network.

The code can be run in parallel to crossvalidate Speech data TI46.

The papar <a href="https://www.frontiersin.org/articles/10.3389/fnins.2019.00031/abstract"> Information-Theoretic Intrinsic Plasticity for Online Unsupervised Learning in Spiking Neural Networks</a> is published by the Frontiers in Neuroscience.

Contact <zhangwenrui@tamu.edu> if you have any questions or concerns.

### Some syntax for the netlist:

1) neuronGroup: Use this to create a group of neurons

neurongroup [name] [num_neurons] [excitatory/inhibitory] [vmem](Optional)

2) column: Use this to create the reservoir

column [name] [x] [y] [z]

x, y, z are three dimensions of the reservoir

3) lsmsynapse: Specify the connections

lsmsynapse [input_neuron_group] [output_neuron_group] [input_connect] [output_connect] [weight_max] [mode] [fixed/learning]

input_connect, output_connect:

Select $input_connect neurons to randomly connect to $output_connect neurons.

Notice that the input_connect can only be 1 or -1

If input_connect == -1 and output_connect == -1, that means the fully connectivity

mode = 0, 1, 2

0: initialize all weights as zero

1: initialize all weights as a fixed number (weight_max)

2: initialize the weights randomly in (-1, 1)

fixed/learning : tell you whether the synapse can be learned or not.

### Run the code:
```sh
$ make -j
$ ./NeuromorphicSim
```

### Define
In the def.h file, most of neuron parameters can be changed. Users can enable SpiKL\_IP to test network with IP, and disable that define to test network without IP.

### Read spikes
The spikes generated in the input layer and reservoir will be written in the files in fold spikes. 
Each file has two columns. The first one represents index of neuron and the second one represents spike time.



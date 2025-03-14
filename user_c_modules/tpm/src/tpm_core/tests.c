#include <stdio.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>

#include <math.h>

///////////////////////////////////////////////////////////////
//math helpers

int sign(int x) {
    return (x > 0) - (x < 0);
}

int sigma_func(float x) {
    return x > 0 ? 1 : -1;
}

int g_func(int w, int l){
    int w_abs = abs(w);
    if(w_abs>l){
        return l *  sign(w);
    }
    return w;
}

int o_func(int a, int b){
    return a==b;
}

//TPM learn rules

int hebbian_rule(int neuronInput ,int neuronOutput, int outputLocal, int outputExternal, int weight){
	return weight +  neuronInput*outputLocal * o_func(neuronOutput,outputLocal) * o_func(outputLocal,outputExternal);
}

///////////////////////////////////////////////////////////////
//init and config helpers

size_t calculate_buffersize(size_t h, size_t* k, size_t* n){
    size_t size = 0;
    for(int i = 0; i<h; i++){
        size += k[i]*n[i]; 
    }
    return size;
}

//////////////////////////////////////////////////////////////
//TPM calculation functions

//output from this function is an array of integer numbers
// -> this output can then be used as an input for antoher layer
// -> or it can be used for the final output in calculate_tau
void calculate_layer(int* layer_input_buffer, size_t input_count, int* layer_output_buffer, int* layer_weights_buffer, size_t layer_neuron_count){

    for (int i = 0; i < layer_neuron_count; i++){
        int32_t neuron_accumulator = 0;
        for (int j = 0; j < input_count; j++)
        {
            printf("%d %d\n",layer_input_buffer[i*input_count+j], layer_weights_buffer[i*input_count+j]);
            neuron_accumulator += layer_input_buffer[i*input_count+j]*layer_weights_buffer[i*input_count+j]; //The amount of inputs must match the number of weights
        }
        float neuron_local_field = (float)neuron_accumulator / sqrtf((float)input_count);
        layer_output_buffer[i] = (int)sigma_func(neuron_local_field);
        printf("%d\n",layer_output_buffer[i]);
        printf("-------------\n");
    }
}

int calculate_tau(int* input, int input_count){

    int tau = 1; //The activation function makes all outputs either -1 or 1, no need for a bigger datatype
    for (int i = 0; i < input_count; i++)
    {
        printf("input: %d, index: %d\n", input[i], i);
        tau *= input[i];
    }
    return tau;
}

int calculate_network(size_t* k, size_t* n, size_t h, int* input_buffer, int* output_buffer, int* weights_buffer, size_t neuron_count){
    
    int used_weights = 0;
    int used_neurons = 0;
    size_t input_len = n[0]; //The amount of inputs is now defined by n[]

    for(int i = 0; i<h; i++){
        input_len = n[i];
        calculate_layer(input_buffer, input_len, &output_buffer[used_neurons], &weights_buffer[used_weights], k[i]); //No further transformation to the outputs in no_overlap
        for(int j = 0; j<k[i]; j++){ //now set the input for the next iteration to the outputs of this layer
            input_buffer[j] = output_buffer[used_neurons+j];
            printf("output: %d, index: %d\n",input_buffer[j],j);
        }

        used_weights += k[i]*input_len; //this way we know where the layer starts in memory
        used_neurons += k[i]; //Used for tracking the output_buffer
    } 
    printf("final_input_len: %d\n", k[h-1]);
    // //Now the input buffer is the outputs of the last layer, calculate Tau (The network output) 
    int network_output = calculate_tau(input_buffer, k[h-1]);
    return network_output;
}


//todo: implement other learn_rules
void learn(size_t* k, int l,size_t h, int* initial_input, size_t initial_input_size, int* output_buffer,int local_output, int external_output,int* weights){
    int* input = initial_input;
    size_t input_len = initial_input_size;
    int weight_index_start = 0;
    int output_index_start = 0;
    for (int layerIndex = 0; layerIndex < h; layerIndex++){
        int* current_layer_output = &output_buffer[output_index_start];
        for(int j = 0; j < k[layerIndex]; j++){
            for(int i = 0; i<input_len; i++){
				int weightIndex = weight_index_start + j*input_len + i;
                weights[weightIndex] = g_func(hebbian_rule(input[i],current_layer_output[j],local_output,external_output,weights[weightIndex]),l);
            }
        }
        input = &output_buffer[output_index_start];
        input_len = k[layerIndex];
        output_index_start += k[layerIndex];
        weight_index_start += k[layerIndex]*input_len;
    }

}

int *generateRandomArray(int size, int min, int max) {
    // Seed the random number generator
    srand(time(NULL));

    // Allocate memory for the array
    int *arr = (int *)malloc(size * sizeof(int));
    if (arr == NULL) {
        printf("Memory allocation failed!\n");
        exit(1);
    }

    // Generate random integers within the specified range
    for (int i = 0; i < size; i++) {
        arr[i] = rand() % (max - min + 1) + min;
    }

    return arr;
}


int main() {

    size_t k[2] = {4,2};
    size_t n[2] = {2,2};
    size_t h = 2;
    int input_buffer[8] = {1,1,1,1,1,1,1,1};
    int weights[12] = {-1,1, -1,1, -1,1, -1,1, -1,1, -1,1};
    int* output_buffer = malloc(3*sizeof(int));
    size_t neuron_count = 12;
    
    int output = calculate_network(k,n,h,input_buffer,output_buffer,weights,neuron_count);
    printf("%d\n",output);
    return 0;
}



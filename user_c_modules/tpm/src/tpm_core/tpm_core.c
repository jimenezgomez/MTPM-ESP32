#include <math.h>

//Learning rules and TPM Scenarios handler types
typedef void (*create_stimulus_fromOutput_t)(int8_t* inputBuffer_layer, int8_t* outputBuffer_prevLayer, size_t k_layer, size_t n_layer);


///////////////////////////////////////////////////////////////
//math helpers

float rsqrt(float x){
    float result;
     asm volatile (
        "wfr        f1, %1\n"
        "rsqrt0.s	f0, f1\n"
        "mul.s		f2, f1, f0\n"
        "const.s		f3, 3;\n"
        "mul.s		f4, f3, f0\n"
        "const.s		f5, 1\n"
        "msub.s		f5, f2, f0\n"
        "maddn.s		f0, f4, f5\n"
        "mul.s		f2, f1, f0\n"
        "mul.s		f1, f3, f0\n"
        "const.s		f3, 1\n"
        "msub.s		f3, f2, f0\n"
        "maddn.s		f0, f1, f3\n"
        "rfr		%0, f0\n"
        :"=r"(result):"r"(x) )
    ;
    return result;
}

int8_t sign(int8_t x) {
    return (x > 0) - (x < 0);
}

int8_t sigma_func(float x) {
    return x > 0 ? 1 : -1;
}

int8_t g_func(int8_t w, uint8_t l){
    int8_t w_abs = abs(w);
    if(w_abs>l){
        return l *  sign(w);
    }
    return w;
}

int8_t o_func(int8_t a, int8_t b){
    return a==b;
}

//TPM learn rules

int8_t hebbian_rule(int8_t neuronInput ,int8_t neuronOutput, int8_t outputLocal, int8_t outputExternal, int8_t weight){
	return weight +  neuronInput*outputLocal * o_func(neuronOutput,outputLocal) * o_func(outputLocal,outputExternal);
}

///////////////////////////////////////////////////////////////
//init and config helpers

size_t calculate_buffersize(size_t h, size_t* k, size_t* n){
    size_t size = 0;
    for(uint8_t i = 0; i<h; i++){
        size += k[i]*n[i]; 
    }
    return size;
}

//////////////////////////////////////////////////////////////
//TPM calculation functions

//output from this function is an array of integer numbers
// -> this output can then be used as an input for antoher layer
// -> or it can be used for the final output in calculate_tau
void calculate_layer(int8_t* layer_input_buffer, size_t input_count, int8_t* layer_output_buffer, int8_t* layer_weights_buffer, size_t layer_neuron_count){

    for (uint8_t i = 0; i < layer_neuron_count; i++){
        int32_t neuron_accumulator = 0;
        for (uint8_t j = 0; j < input_count; j++)
        {
            neuron_accumulator += layer_input_buffer[i*input_count+j]*layer_weights_buffer[i*input_count+j]; //The amount of inputs must match the number of weights
        }

        float neuron_local_field = (float)neuron_accumulator * rsqrt((float)input_count);
        layer_output_buffer[i] = sigma_func(neuron_local_field);
    }
}

int8_t calculate_tau(int8_t* input, size_t input_count){

    int8_t tau = 1; //The activation function makes all outputs either -1 or 1, no need for a bigger datatype
    for (size_t i = 0; i < input_count; i++)
    {
        tau *= input[i];
    }
    return tau;
}

int8_t calculate_network(size_t* k, size_t* n, size_t h, int8_t* input_buffer, int8_t* output_buffer, int8_t* weights_buffer, size_t neuron_count, create_stimulus_fromOutput_t scenarioHandler){
    
    uint8_t used_weights = 0;
    uint8_t used_neurons = 0;
    size_t input_len = n[0];

    for(uint8_t i = 0; i<h; i++){
        input_len = n[i];
        calculate_layer(input_buffer, input_len, &output_buffer[used_neurons], &weights_buffer[used_weights], k[i]); //No further transformation to the outputs in no_overlap
        scenarioHandler(input_buffer, &output_buffer[used_neurons], k[i], n[i]);

        used_weights += k[i]*input_len; //this way we know where the layer starts in memory
        used_neurons += k[i]; //Used for tracking the output_buffer
    } 
    // //Now the input buffer is the outputs of the last layer, calculate Tau (The network output) 
    int8_t network_output = calculate_tau(input_buffer, k[h-1]);
    return network_output;
}


//todo: implement other learn_rules
void learn(size_t* k, uint8_t l,size_t h, int8_t* initial_input, size_t initial_input_size, int8_t* output_buffer,int8_t local_output, int8_t external_output,int8_t* weights){
    int8_t* input = initial_input;
    size_t input_len = initial_input_size;
    uint8_t weight_index_start = 0;
    uint8_t output_index_start = 0;
    for (uint8_t layerIndex = 0; layerIndex < h; layerIndex++){
        int8_t* current_layer_output = &output_buffer[output_index_start];
        for(uint8_t j = 0; j < k[layerIndex]; j++){
            for(uint8_t i = 0; i<input_len; i++){
				uint8_t weightIndex = weight_index_start + j*input_len + i;
                weights[weightIndex] = g_func(hebbian_rule(input[i],current_layer_output[j],local_output,external_output,weights[weightIndex]),l);
            }
        }
        input = &output_buffer[output_index_start]; //Set the input to this layers output
        input_len = k[layerIndex]; //The amount of outputs equals the amount of neurons
        output_index_start += k[layerIndex];
        weight_index_start += k[layerIndex]*input_len;
    }

}



void noOverlap_stimHandler(int8_t* inputBuffer_layer, int8_t* outputBuffer_prevLayer, size_t k_layer, size_t n_layer){
    for(size_t i = 0; i<k_layer; i++){ //now set the input for the next iteration to the outputs of this layer
        for(size_t j = 0; j<n_layer; j++){ //now set the input for the next iteration to the outputs of this layer
            inputBuffer_layer[i*n_layer + j] = outputBuffer_prevLayer[i*n_layer + j];
        }
    }
}
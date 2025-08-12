void noOverlap_stimHandler(int8_t* inputBuffer_layer, int8_t* outputBuffer_prevLayer, size_t k_layer, size_t n_layer){
    for(size_t i = 0; i<k_layer; i++){ //now set the input for the next iteration to the outputs of this layer
        for(size_t j = 0; j<n_layer; j++){ //now set the input for the next iteration to the outputs of this layer
            inputBuffer_layer[i*n_layer + j] = outputBuffer_prevLayer[i*n_layer + j];
        }
    }
}
bool noOverlap_check(size_t* k, size_t* n, size_t h){
    for(size_t i = h-1; i>0; i--){
        if (k[i-1] != k[i]*n[i]) { return false; }
    }
    return true;
}
scenario_handler_t noOverlap_Handler = (scenario_handler_t){noOverlap_stimHandler, noOverlap_check};

void fullOverlap_stimHandler(int8_t* inputBuffer_layer, int8_t* outputBuffer_prevLayer, size_t k_layer, size_t n_layer){
    for(size_t i = 0; i<k_layer; i++){ //now set the input for the next iteration to the outputs of this layer
        for(size_t j = 0; j<n_layer; j++){ //now set the input for the next iteration to the outputs of this layer
            inputBuffer_layer[i*n_layer + j] = outputBuffer_prevLayer[j];
        }
    }
}
//TODO
bool fullOverlap_check(size_t* k, size_t* n, size_t h){
    // for(size_t i = h-1; i>0; i--){
    //     if (k[i-1] != k[i]*n[i]) { return false; }
    // }
    return true;
}
scenario_handler_t fullOverlap_Handler = (scenario_handler_t){fullOverlap_stimHandler, fullOverlap_check};

void partialOverlap_stimHandler(int8_t* inputBuffer_layer, int8_t* outputBuffer_prevLayer, size_t k_layer, size_t n_layer){
    for(size_t i = 0; i<k_layer; i++){ //now set the input for the next iteration to the outputs of this layer
        for(size_t j = 0; j<n_layer; j++){ //now set the input for the next iteration to the outputs of this layer
            inputBuffer_layer[i*n_layer + j] = outputBuffer_prevLayer[i+j];
        }
    }
}
//TODO
bool partialOverlap_check(size_t* k, size_t* n, size_t h){
    // for(size_t i = h-1; i>0; i--){
    //     if (k[i-1] != k[i]*n[i]) { return false; }
    // }
    return true;
}
scenario_handler_t partialOverlap_Handler = (scenario_handler_t){partialOverlap_stimHandler, partialOverlap_check};


//n must contain h items.
uint8_t* CreateStimulationStructure(uint8_t* n, size_t h, uint8_t k_last){
    uint8_t* k = (uint8_t*)malloc(h*sizeof(uint8_t)); //Each neuron has one output, so lets allocate an array for the outputs
    k[h-1] = k_last;

    for (uint8_t layer = 1; layer < h; layer++){
        k[h-1-layer] = n[h-layer]*k[h-layer]
    }

    return k;
}


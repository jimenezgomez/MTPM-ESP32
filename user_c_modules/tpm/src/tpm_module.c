#include "py/obj.h"
#include "py/runtime.h"
#include "esp_random.h"
#include "esp_log.h"
#include "./tpm_core/tpm_core.c"
#include "./tpm_core/tpm_scenarios.c"

typedef struct _TPM_obj_t {
    mp_obj_base_t base;
    //TPMm Settings
    size_t h;
    size_t* k;
    size_t* n;
    uint8_t l;
    uint8_t m;
    uint8_t learn_rule;
    uint8_t scenario;
    size_t neuron_count;

    //handlers
    scenario_handler_t* scenarioHandler;
    learnRule_handler_t* learnRuleHandler;

    //Buffers
    int8_t* weights;
    size_t weights_buffersize;
    int8_t* input_buffer; 
    size_t input_buffersize;// This is K_0 * N_0 
    int8_t* output_buffer; //This is neuron_count size
    int8_t last_output;
} TPM_obj_t;

// This represents TPM.__new__ and TPM.__init__, which is called when
// the user instantiates a TPM object.
static mp_obj_t TPM_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // Allocates the new object and sets the type.
    mp_arg_check_num(n_args, n_kw, 7, 7, true); // Ensure the correct number of arguments
    TPM_obj_t *self = mp_obj_malloc(TPM_obj_t, type);

    // Extract parameters from args
    // H: Layer size
    size_t h = mp_obj_get_int(args[0]);
    if (!MP_OBJ_IS_TYPE(args[1], &mp_type_list)) {
        mp_raise_TypeError("Argument K must be a list");
    }
    self->h = h;
    
    //K: Neuron count for each layer
    mp_obj_list_t *list_k = MP_OBJ_TO_PTR(args[1]);
    //Check size
    if(list_k->len != h) {
            mp_raise_ValueError("The number of layers (h) does not match the number of items inside k");
            return mp_const_none;
    }
    //Assign
    self->k = m_new(size_t, list_k->len);
    size_t neuron_count = 0;
    for (size_t i = 0; i < h; i++) {
        mp_obj_t item = list_k->items[i];
        if (MP_OBJ_IS_INT(item)) {
            self->k[i] = mp_obj_get_int(item);
            neuron_count += self->k[i];
        } else {
            mp_raise_TypeError("List K items must be integers");
        }
    }
    // if (!MP_OBJ_IS_TYPE(args[1], &mp_type_list)) {
    //     mp_raise_TypeError("Argument K must be a list");
    // }

    //N: Neuron input size for each layer
    mp_obj_list_t *list_n = MP_OBJ_TO_PTR(args[2]);
    //Check size
    if(list_n->len != h) {
            mp_raise_ValueError("The number of layers (h) does not match the number of items inside n");
            return mp_const_none;
        }
    //Assign
    self->n = m_new(size_t, list_n->len);
    for (size_t i = 0; i < h; i++) {
        mp_obj_t item = list_n->items[i];
        if (MP_OBJ_IS_INT(item)) {
            self->n[i] = mp_obj_get_int(item);
        } else {
            mp_raise_TypeError("List N items must be integers");
        }
    }

    //Assign L, M LearnRule and Scenario

    uint8_t l = mp_obj_get_int(args[3]);
    uint8_t m = mp_obj_get_int(args[4]);
    uint8_t rule = mp_obj_get_int(args[5]);
    switch (rule){
        case 0:
        case 1:
        case 2:
            break;
        default:
            mp_raise_ValueError("You must use the following numbers for the learning rules: \n\t- 0: Hebbian \n\t- 1: Anti-Hebbian \n\t- 2: Random-Walk \n\n");
            return mp_const_none;
    }
    uint8_t scenario = mp_obj_get_int(args[6]);
    switch (scenario){
        case 0:
            self->scenarioHandler = &noOverlap_Handler;
            break;
        case 1:
            self->scenarioHandler = &partialOverlap_Handler;
            break;
        case 2:
            self->scenarioHandler = &fullOverlap_Handler;
            break;
        default:
            mp_raise_ValueError("You must use the following numbers for the Scenarios: \n\t- 0: Without overlap\n");
            return mp_const_none;
    }

    //Check if Structure is valid
    if (self->scenarioHandler->check_structure(self->k,self->n, self->h) != true) {
        mp_raise_ValueError("The provided structure is not valid.\n - Please check if the scenario, K or N arrays are correct.");
        return mp_const_none;
    }

    //populate weights from parameters
    size_t buffersize = calculate_buffersize(h,self->k,self->n);
    int8_t* w = m_new(int8_t, buffersize);
    // !- ESP32 ONLY!!
    esp_fill_random(w,buffersize);
    // !-
    for (size_t i = 0; i < buffersize; i++) {
        w[i] = w[i] % (l+1);  // Limit weights from -l to l
    }

    self->l = l;
    self->m = m;
    self->learn_rule = rule;
    self->scenario = scenario;
    self->weights = w;
    self->weights_buffersize = buffersize;
    self->neuron_count = neuron_count;
    self->input_buffersize = (self->n[0]*self->k[0]);
    self->input_buffer = m_new(int8_t, self->input_buffersize);
    self->output_buffer = m_new(int8_t, neuron_count);
    self->last_output = 0;

    // The make_new function always returns self.
    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t TPM_get_weights(mp_obj_t self_in) {
    // The first argument is self. It is cast to the *TPM_obj_t
    // type so we can read its attributes.
    TPM_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int8_t* w = self->weights;
    size_t buffersize = self->weights_buffersize;    
    return mp_obj_new_bytearray(buffersize,w);
}
static MP_DEFINE_CONST_FUN_OBJ_1(TPM_weights, TPM_get_weights);

static mp_obj_t TPM_get_k(mp_obj_t self_in) {
    // The first argument is self. It is cast to the *TPM_obj_t
    // type so we can read its attributes.
    TPM_obj_t *self = MP_OBJ_TO_PTR(self_in);
    size_t* k = self->k;
    return mp_obj_new_bytearray(self->h,k);
}
static MP_DEFINE_CONST_FUN_OBJ_1(TPM_k, TPM_get_k);

static mp_obj_t TPM_get_n(mp_obj_t self_in) {
    // The first argument is self. It is cast to the *TPM_obj_t
    // type so we can read its attributes.
    TPM_obj_t *self = MP_OBJ_TO_PTR(self_in);
    size_t* n = self->n;
    return mp_obj_new_bytearray(self->h,n);
}
static MP_DEFINE_CONST_FUN_OBJ_1(TPM_n, TPM_get_n);

static mp_obj_t TPM_calculate_output(mp_obj_t self_in, mp_obj_t input_array) {
    TPM_obj_t *self = MP_OBJ_TO_PTR(self_in);

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(input_array, &bufinfo, MP_BUFFER_READ);
    if (self->input_buffersize != (bufinfo.len)) {
        mp_raise_msg_varg(&mp_type_ValueError,
            MP_ERROR_TEXT("Input does not match n[0]: received=%d expected=%d"),
            bufinfo.len, self->input_buffersize); 
        return mp_const_none;
    }
    int8_t* casted_buffer = (int8_t*)bufinfo.buf;
    for(uint8_t i = 0; i < bufinfo.len; i++) {
        self->input_buffer[i] = casted_buffer[i];
    }    

    self->last_output = calculate_network(self->k, self->n,self->h,self->input_buffer,self->output_buffer,self->weights,self->neuron_count, self->scenarioHandler);
    return mp_obj_new_int(self->last_output);
}

static MP_DEFINE_CONST_FUN_OBJ_2(TPM_calculate_output_OBJ, TPM_calculate_output);

static mp_obj_t TPM_learn_output(mp_obj_t self_in, mp_obj_t input_array, mp_obj_t external_output_mp) {
    TPM_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(input_array, &bufinfo, MP_BUFFER_READ);
    if (self->input_buffersize != (bufinfo.len)) {
        mp_raise_msg_varg(&mp_type_ValueError,
            MP_ERROR_TEXT("Input does not match n[0]: received=%d expected=%d"),
            bufinfo.len, self->input_buffersize); 
        return mp_const_none;
    }
    int8_t* casted_buffer = (int8_t*)bufinfo.buf;
    for(uint8_t i = 0; i < bufinfo.len; i++) {
        self->input_buffer[i] = casted_buffer[i];
    }
    int8_t ext_output = mp_obj_get_int(external_output_mp);
    
    learn(self->k,self->l,self->h,casted_buffer,self->input_buffersize,self->output_buffer, self->last_output,ext_output,self->weights);

    return mp_const_none;
}

static MP_DEFINE_CONST_FUN_OBJ_3(TPM_learn_OBJ, TPM_learn_output);


static const mp_rom_map_elem_t TPM_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_get_weights), MP_ROM_PTR(&TPM_weights) },
    { MP_ROM_QSTR(MP_QSTR_get_n), MP_ROM_PTR(&TPM_n) },
    { MP_ROM_QSTR(MP_QSTR_get_k), MP_ROM_PTR(&TPM_k) },
    { MP_ROM_QSTR(MP_QSTR_calculate), MP_ROM_PTR(&TPM_calculate_output_OBJ) },
    { MP_ROM_QSTR(MP_QSTR_learn), MP_ROM_PTR(&TPM_learn_OBJ) },
};
static MP_DEFINE_CONST_DICT(TPM_locals_dict, TPM_locals_dict_table);

// This defines the type(TPM) object.
MP_DEFINE_CONST_OBJ_TYPE(
    _type_TPM,
    MP_QSTR_TPM,
    MP_TYPE_FLAG_NONE,
    make_new, TPM_make_new,
    locals_dict, &TPM_locals_dict
    );
 
static const mp_rom_map_elem_t tpm_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_tpm) },
    { MP_ROM_QSTR(MP_QSTR_TPM), MP_ROM_PTR(&_type_TPM) },
};
 
static MP_DEFINE_CONST_DICT(tpm_module_globals, tpm_module_globals_table);
 
const mp_obj_module_t mp_module_tpm = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&tpm_module_globals,
};

// Register the module to make it available in Python.
MP_REGISTER_MODULE(MP_QSTR_tpm, mp_module_tpm);

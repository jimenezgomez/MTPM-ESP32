# MTPM Utilities for MicroPython

This MicroPython C module provides a set of utilities to work with **Multilayer Tree Parity Machines (MTPMs)**.
It exposes a class `TPM` with methods for calculating outputs, performing learning steps, and retrieving network parameters and weights.

Once the module is compiled and installed into MicroPython, the following API will be available.

---

## Installation

Compile and include this C module into your MicroPython firmware build.
Follow the MicroPython [custom C module guide](https://docs.micropython.org/en/latest/develop/cmodules.html) for details.

In the root directory of the repository, the file `compile.sh` can automatically build and upload the artifact to an ESP32, however it requires both [micropython](https://github.com/micropython/micropython) and [esp-idf (v5.4.2)](https://github.com/espressif/esp-idf/tree/v5.4.2) to be in the same directory as the script. The `esp-idf` version `5.4.2` is required according to the [Official MicroPython for ESP32 Setup guide](https://github.com/micropython/micropython/tree/master/ports/esp32).

---

## Usage Example

The following example creates a two-layer Multilayer Tree Parity Machine, with 8 neurons on the first layer (each receiving 2 inputs), and 2 neurons on the second layer (each receiving 4 inputs). This MTPM also has a synaptic weights range of `(-3, 3)` and binary inputs in it's first layer. It uses the Hebbian Learning rule and a No-Overlap scenario.

```python
import tpm
import struct

tpm = TPM(
    h=2,          # Layer count
    k=[8,2],      # Neurons per layer
    n=[2,4],      # Inputs per neuron for each layer
    l=3,          # Weight range parameter. In this case (-3,3)
    m=1,          # First layer input range. In this case (-1,1) without counting zero.
    rule=0,       # Learning rule ID (Only Hebbian available for now)
    scenario=0    # Scenario ID (0: No-Overlap, 1: Partial-Overlap, 2: Full-Overlap)
)

input_buf = struct.pack('16b',*[1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1])
result = tpm.calculate(input_buf)

external_output = 1 #Another MTPM's calculated output
tpm.learn(input_buf, external_output) #Learn using the external output and the last calculated output in the buffer.

for byte in tpm.get_weights():
    signed_byte = struct.unpack('b', bytes([byte]))[0]
    print(signed_byte)
```

---

## API Reference

### **`TPM(h, k, n, l, m, rule, scenario)`**

Constructor for a Tree Parity Machine.

**Parameters:**

* `h` *(int)* – Number of layers.
* `k` *(Array of ints)* – Number of neurons per layer.
* `n` *(Array of ints)* – Number of inputs per neuron.
* `l` *(int)* – Weight range parameter (weights range from `-l` to `+l`).
* `m` *(int)* –  First layer input range parameter. (Inputs range from `-m` to `m` without counting `0`)
* `rule` *(int - 0)* – Learning rule ID (Only Hebbian available).
* `scenario` *(int - 0, 1 or 2)* – Scenario ID (0: No-Overlap, 1: Partial-Overlap, 2: Full-Overlap).

---

### **`TPM.calculate(input_array)`**

Computes the TPM output for the given input array.

**Parameters:**

* `input_array` *(buffer-compatible object)* – Input vector for the TPM.

  * Must support the buffer protocol (e.g., `bytes`, `bytearray`, `memoryview`).
  * Internally read using:

    ```c
    mp_get_buffer_raise(input_array, &bufinfo, MP_BUFFER_READ);
    ```

**Returns:**

* Output value (implementation-defined, usually `-1` or `+1`).

---

### **`TPM.learn(input_array, ext_output)`**

Performs a learning step using the given input and an external output.

**Parameters:**

* `input_array` *(buffer-compatible object)* – Same requirements as `calculate()`.
* `ext_output` *(int)* – External TPM output to use for the learning step.

**Returns:**

* None

---

### **`TPM.get_n()`**

Returns the **N** parameter (inputs per neuron) for each layer.

**Returns:**

* `bytearray` of size `H` (number of layers), where each byte corresponds to the `N` value for that layer.

---

### **`TPM.get_k()`**

Returns the **K** parameter (neurons per layer) for each layer.

**Returns:**

* `bytearray` of size `H` (number of layers), where each byte corresponds to the `K` value for that layer.

---

### **`TPM.get_weights()`**

Returns the MTPM’s flat weights array.

**Returns:**

* `bytearray` containing all weights.
* Size is calculated using a helper function from the core library, based on the network architecture.

---

### Notes

* **Buffer inputs:** `calculate()` and `learn()` require a buffer-compatible object. Passing lists will not work; use `bytes`, `bytearray`, or `memoryview`.
* **Return types:** Many getters return `bytearray` objects so they can be directly read or unpacked in Python.
* This module is intended for embedded use in ESP32 and similar microcontrollers running MicroPython.

---

## Project Structure

The directory `mp_examples` contains two examples, one basic (`basic.py`) example that shows how to quickly get started, and `main.py`, which demonstrates how to use this library for synchronizing two neural networks using MQTT. `main.py` is meant to be used with the setup files found in the `setup` directory.

The source is split in two: the module and the core. 

**The module** contains all the definitions that expose the core functionality to the MicroPython user. It is all contained in the file `tpm/src/tpm_module.c`. **The core** is in the directory `tpm/src/tpm_core`. It contains mathematical operations as well as other auxiliary functions used for MTPM synchronization. These functions are required for learning, calculating the output of a network, calculating the size of the network (the buffersize of the weights array), among many other uses.  
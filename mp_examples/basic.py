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
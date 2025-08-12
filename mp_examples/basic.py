import tpm
import array
import struct

a = tpm.TPM(2,[8,2],[2,4],3,1,0,1)
for byte in a.get_weights():
    signed_byte = struct.unpack('b', bytes([byte]))[0]  # 'b' for signed char
    print(signed_byte)


buf = struct.pack('16b',*[1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1])
a.calculate(buf)

# /////
a.calculate(buf)
a.learn(buf,-1)
for byte in a.get_weights():
    signed_byte = struct.unpack('b', bytes([byte]))[0]  # 'b' for signed char
    print(signed_byte)

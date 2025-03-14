import tpm
import array
import struct

a = tpm.TPM(1,[3],[1],3,1,0)
for byte in a.get_weights():
    signed_byte = struct.unpack('b', bytes([byte]))[0]  # 'b' for signed char
    print(signed_byte)


buf = struct.pack('3b',*[1,1,1])
a.calculate(buf)

# /////
a.calculate(buf)
a.learn(buf,-1)
for byte in a.get_weights():
    signed_byte = struct.unpack('b', bytes([byte]))[0]  # 'b' for signed char
    print(signed_byte)

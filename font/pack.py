with open('segoeui.ttf', 'rb') as input_file:
    file_bytes = input_file.read()

with open('../src/segoeui.h', 'w') as output_file:
    output_file.write('const unsigned char segoeui[] = {')
    for byte in file_bytes:
        output_file.write('0x{:02X}, '.format(byte))
    output_file.write('};')

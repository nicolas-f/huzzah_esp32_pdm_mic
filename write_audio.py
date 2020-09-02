#install pyserial
import serial
import serial.tools.list_ports
import struct

baud = 2000000
print("Serial ports:")

ports = serial.tools.list_ports.comports(include_links=False)

if len(ports) > 0:
    for port in ports:
        print(port.device)

    # on établit la communication série
    arduino = serial.Serial(ports[0].device, baud, timeout=1)

    max_samples = 48000 * 10
    sample_size = 2
    bufferlength = 512 * sample_size
    with open("audio.raw", "wb") as f:
        while True:
            f.write(arduino.read(bufferlength))
            max_samples -= bufferlength / sample_size
            if max_samples < 0:
                break





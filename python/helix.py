import serial

class Helix:
    def __init__(self, port, baudrate=9600):
        self.ser = serial.Serial(port, baudrate=baudrate)
        print self.ser.readline()
    def meas_freq(self, channel=1):
        self.ser.write("FREQ%d.READ?\n" % channel)
        resp = self.ser.readline().strip()
        return int(resp)

if __name__ == "__main__":
    import glob
    ports = glob.glob("/dev/ttyUSB*")
    h = Helix(ports[-1])

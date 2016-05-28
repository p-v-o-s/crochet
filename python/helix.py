import serial, time

class Helix:
    def __init__(self, port, baudrate=9600):
        self.ser = serial.Serial(port, baudrate=baudrate)
        #print(self.ser.readline())
    def meas_freq(self, channel=1):
        cmd = "FREQ%d.READ?\n" % channel
        self.ser.write(cmd.encode())
        resp = self.ser.readline().strip()
        return int(resp)

if __name__ == "__main__":
    import glob
    import numpy as np
    ports = glob.glob("/dev/ttyUSB*")
    h = Helix(ports[-1])
    data = []
    try:
        t0 = time.time()
        while True:
            rec = (time.time() - t0, h.meas_freq())
            print(rec)
            data.append(rec)
            time.sleep(2.0)
    except KeyboardInterrupt:
        pass
    finally:
        D = np.array(data)
        np.savetxt("output.csv", D, delimiter=",")

import serial
import time
import sys
import glob

class GantryControl():
    def __init__(self, port) -> None:
        self.ser = self.connectSerial(port)
        # self.ser = self.serial_ports()
    
    # Query available serial ports and return available one
    def serial_ports(self):
        """ Lists serial port names

            :raises EnvironmentError:
                On unsupported or unknown platforms
            :returns:
                A list of the serial ports available on the system
        """
        if sys.platform.startswith('win'):
            ports = ['COM%s' % (i + 1) for i in range(256)]
        elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
            # this excludes your current terminal "/dev/tty"
            ports = glob.glob('/dev/tty[A-Za-z]*')
        elif sys.platform.startswith('darwin'):
            ports = glob.glob('/dev/tty.*')
        else:
            raise EnvironmentError('Unsupported platform')

        result = []
        for port in ports:
            try:
                s = serial.Serial(port, baudrate=115200)
                # s.close()
                print(port)
                return s
                # result.append(port)
            except:
                pass

        # return result

    def connectSerial(self, port):
        # Initialize serial
        ser = serial.Serial(
            port=port,
            baudrate=115200
        )
        print('connected to {}'.format(port))
        return ser

    # Send command to motion controller
    def command(self, command_string):
        self.ser.write(str.encode(command_string))
        time.sleep(1)

        while True:
            line = self.ser.readline()
            print(line)

            if b'ok' in line:
                break

    # Set feedrate (mm/min)
    def setSpeed(self, speed_mm_min):
        self.command("G0 F{}\r\n".format(speed_mm_min))

    # Move in the XY plane (mm)
    def moveXY(self, x_mm=0, y_mm=0):
        self.command("G0 X{} Y{}\r\n".format(x_mm, y_mm))

    def moveZ(self, z_deg):
        z_mm = z_deg/10.0
        self.command("G0 Z{}\r\n".format(z_mm))

    # Sets the current position to (0, 0, 0)
    def setOrigin(self):
        self.command("G92 X0 Y0 Z0\r\n")

# Test Code
gantry = GantryControl()
time.sleep(2)
gantry.setSpeed(500)
gantry.setOrigin()

for i in range(3):
    gantry.moveXY(x_mm=9) # about 90 degrees
    time.sleep(1)
    gantry.moveXY(x_mm=0)
    time.sleep(1)

gantry.ser.close()

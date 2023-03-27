import serial
import time
import sys
import glob

MAX_Z = 480

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
    def set_speed(self, speed_mm_min):
        self.command("G0 F{}\r\n".format(speed_mm_min))

    # Move in the XY plane (mm)
    def move_xz(self, x_mm=0, z_mm=0):
        self.command("G0 X{} Z{} F1500\r\n".format(x_mm, z_mm))

    def move_x(self, x_mm):
        self.command("G0 X{} F1500\r\n".format(x_mm))

    def move_arm(self, y_deg):
        y_mm = y_deg/10.0
        self.command("G0 Y{} F100\r\n".format(y_mm))
        self.disable_steppers()

    # Sets the current position to (0, 0, 0)
    def set_origin(self):
        self.command("G92 X0 Y0 Z0\r\n")

    def auto_home_xz(self):
        self.command("G28 X Z\r\n")

    def auto_home_arm(self):
        self.move_arm(-40)
        self.disable_steppers()
        self.command("G92 Y0\r\n")
    
    def disable_steppers(self):
        self.command("M18\r\n")

# Test Code
# gantry = GantryControl('/dev/tty/USB0')
# time.sleep(2)
# gantry.auto_home_arm()
# gantry.auto_home_xz()


# gantry.ser.close()

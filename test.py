import serial
import time

# Configuration
serial_port = "COM7"  # Replace with your serial port
baud_rate = 256000  # Baud rate (not strictly relevant for USB CDC)
read_timeout = 1  # Timeout for serial read operations in seconds


def measure_throughput(port, duration):
    """
    Measure the throughput of the serial port in bytes per second.

    Args:
        port (str): Serial port name (e.g., "COM7" or "/dev/ttyUSB0").
        duration (int): Duration of the measurement in seconds.

    Returns:
        float: Measured throughput in bytes per second.
    """
    try:
        # Open the serial port
        with serial.Serial(port, baud_rate, timeout=read_timeout) as ser:
            ser.set_buffer_size(rx_size=4096)

            print(f"Measuring throughput on {port} for {duration} seconds...")

            # Flush any existing data in the input buffer
            ser.reset_input_buffer()

            # Start measurement
            start_time = time.time()
            total_bytes = 0

            while time.time() - start_time < duration:
                # Read all available data from the buffer
                data = ser.read(ser.in_waiting or 1)
                total_bytes += len(data)

            # Calculate throughput
            elapsed_time = time.time() - start_time
            throughput = total_bytes / elapsed_time
            return throughput

    except Exception as e:
        print(f"Error: {e}")
        return 0


if __name__ == "__main__":
    measurement_duration = 10  # Measure throughput for 10 seconds
    throughput = measure_throughput(serial_port, measurement_duration)
    print(f"Measured Throughput: {throughput:.2f} bytes/second")

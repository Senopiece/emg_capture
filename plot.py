import argparse
import threading
import serial
import time
import matplotlib.pyplot as plt
from collections import deque

# Create a deque for each channel to store the last N records
dmaxlen = 10000
channels = 6  # Ensure this matches the number of channels you're using
data = [deque([0] * dmaxlen, maxlen=dmaxlen) for _ in range(channels)]

# Set up the plot
fig, ax = plt.subplots()
lines = []
for i in range(channels):
    line, = ax.plot(data[i], label=f'Channel {i}')
    lines.append(line)
ax.set_ylim(0, 4095)  # ADC range for 12-bit resolution is 0-4095
ax.set_title("Real-Time ADC Data")
ax.set_xlabel("Samples")
ax.set_ylabel("ADC Value")
ax.legend()

# Text objects to display data rate and buffer size
data_rate_text = ax.text(
    0.05,
    0.87,
    "",
    transform=ax.transAxes,
    fontsize=12,
    verticalalignment="top",
    horizontalalignment="left",
)

buffer_size_text = ax.text(
    0.05,
    0.95,
    "",
    transform=ax.transAxes,
    fontsize=12,
    verticalalignment="top",
    horizontalalignment="left",
)

# Global flag to track if the plot window is open
plot_running = True

# Data rate tracking variables
packet_count = 0
last_update_time = time.time()
last_incoming_len = 0

# Serial configuration
baud_rate = 256000
bytes_per_channel = 2
packet_size = channels * bytes_per_channel


class SyntheticSerial:
    """Mock serial port for generating synthetic ADC data."""
    def __init__(self):
        self.current_count = 0
        self.buffer = bytearray()  # Use a bytearray to store bytes
        self.buffer_lock = threading.Lock()  # Lock for thread-safe access to the buffer
        self.channels = 6  # Example number of channels
        self.running = True

        # Start the worker thread to add bytes to the buffer
        self.worker_thread = threading.Thread(target=self._worker)
        self.worker_thread.start()

    def _worker(self):
        """Worker thread that adds 100 packets each 0.05 second to the buffer (so that the rate 2000 packets per second)."""
        while self.running:
            start_time = time.time()  # Track the start time of the 1-second period

            # Generate a synthetic packet with incrementing values
            packets = bytearray()
            for _ in range(100):
                for channel in range(self.channels):
                    value = (self.current_count + channel*100) % 4096
                    packets += value.to_bytes(2, byteorder='little')
                packets += b'\xFF\xFF'  # Delimiter
                self.current_count += 1

            # Add the packet bytes to the buffer (thread-safe)
            with self.buffer_lock:
                self.buffer.extend(packets)

            # Calculate the remaining time to sleep to maintain the 1-second period
            elapsed_time = time.time() - start_time
            sleep_time = 0.05 - elapsed_time

            if sleep_time > 0:
                time.sleep(sleep_time)  # Sleep for the remaining time

    @property
    def in_waiting(self):
        """Return the number of bytes waiting in the buffer."""
        with self.buffer_lock:
            return len(self.buffer)

    def read(self, size=1):
        """Read bytes from the buffer."""
        with self.buffer_lock:
            # Read up to `size` bytes from the buffer
            data = self.buffer[:size]
            # Remove the read bytes from the buffer
            self.buffer = self.buffer[size:]
        return bytes(data)

    def close(self):
        """Stop the worker thread and clean up."""
        self.running = False
        self.worker_thread.join()


def on_close(event):
    """Handle the plot window close event."""
    global plot_running
    plot_running = False
    print("Plot window closed.")


def read_packets(ser):
    """
    Read and decode all packets ending with [0xFF, 0xFF] from the serial buffer.
    """
    global packet_count, last_incoming_len

    buffer = bytearray()

    try:
        # Read available bytes from the serial buffer
        incoming = ser.read(ser.in_waiting or 1)
        last_incoming_len = len(incoming)

        # Track the incoming buffer length for smoothing (optional monitoring)
        if not incoming:
            return
        
        buffer.extend(incoming)

        # Process packets ending with [0xFF, 0xFF]
        while True:
            # Check if [0xFF, 0xFF] exists in the buffer
            delimiter_index = buffer.find(b"\xFF\xFF")
            if delimiter_index == -1:
                break  # No complete packet found yet

            # Extract the packet (up to but excluding [0xFF, 0xFF])
            packet = buffer[:delimiter_index]
            buffer = buffer[delimiter_index + 2:]  # Remove the packet and delimiter

            # Process the packet
            try:
                if len(packet) == packet_size:
                    # Append each channel's value to their respective deque
                    for i in range(channels):
                        data[i].append(int.from_bytes(packet[2*i:2*(i+1)], byteorder="little"))
                    packet_count += 1
                else:
                    print(f"Unexpected packet size: {len(packet)} (expected {packet_size})")
            except Exception as e:
                print(f"Packet processing error: {e}")
    except Exception as e:
        print(f"Error while reading packets: {e}")


def plot_update(ser):
    """Update the plot with the latest data."""
    global packet_count, last_update_time, last_incoming_len

    while plot_running:
        # Read and process all available packets
        read_packets(ser)

        # Update each channel's plot line
        for i, line in enumerate(lines):
            line.set_ydata(data[i])

        # Calculate and display debug values
        current_time = time.time()
        elapsed_time = current_time - last_update_time
        if elapsed_time >= 1.0:
            data_rate = packet_count / elapsed_time
            packet_count = 0

            data_rate_text.set_text(f"Data Rate: {data_rate:.2f} Hz")
            buffer_size_text.set_text(f"Debt Size: {int(last_incoming_len)} bytes")

            last_update_time = current_time

        # Redraw the plot
        fig.canvas.draw_idle()
        plt.pause(0.01)  # Allow matplotlib to process GUI events


def main(serial_port):
    """Main loop for reading serial data and updating the plot."""
    global plot_running

    # Connect the close event handler
    fig.canvas.mpl_connect("close_event", on_close)

    if serial_port == 'synthetic':
        # Use synthetic data generator
        ser = SyntheticSerial()
        print("Starting synthetic data mode...")
        try:
            plot_update(ser)
        finally:
            print("Exiting program.")
            ser.close()
    else:
        # Open real serial connection
        try:
            with serial.Serial(serial_port, baud_rate, timeout=1) as ser:
                print(f"Listening on {serial_port} at {baud_rate} baud...")
                plot_update(ser)
        except Exception as e:
            print(f"Serial connection error: {e}")
        finally:
            print("Exiting program.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        prog='Signal Plotter',
        description='Plot real-time ADC data from serial port or generate synthetic data.'
    )
    parser.add_argument('-p', '--port', type=str, required=True,
                        help="Serial port name or 'synthetic' for synthetic data")
    args = parser.parse_args()

    main(args.port)
import serial
import time
import matplotlib.pyplot as plt
from collections import deque

# Create a deque to store the last N records
dmaxlen = 10000
data = deque([0] * dmaxlen, maxlen=dmaxlen)

# Set up the plot
fig, ax = plt.subplots()
(line,) = ax.plot(data)
ax.set_ylim(0, 4095)  # ADC range for 12-bit resolution is 0-4095
ax.set_title("Real-Time ADC Data")
ax.set_xlabel("Samples")
ax.set_ylabel("ADC Value")

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

# Serial configuration
serial_port = "COM7"  # Replace with your serial port (e.g., '/dev/ttyUSB0')
baud_rate = 256000
packet_size = 2 * 16  # Expecting 2-byte payloads after COBS decoding

# Buffer for incoming serial data
buffer = bytearray()
buffer_lens = deque([0] * 100, maxlen=100)
smooth_buffer_len = 0


def on_close(event):
    """Handle the plot window close event."""
    global plot_running
    plot_running = False
    print("Plot window closed.")


def read_packets(ser):
    """
    Read and decode all packets ending with [0xFF, 0xFF] from the serial buffer.

    Args:
        ser: Serial connection object.
    """
    global buffer, packet_count, smooth_buffer_len

    try:
        # Read available bytes from the serial buffer
        incoming = ser.read(ser.in_waiting or 1)

        # Track the incoming buffer length for smoothing (optional monitoring)
        buffer_lens.append(len(incoming))
        smooth_buffer_len = sum(buffer_lens) / len(buffer_lens)

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
            buffer = buffer[delimiter_index + 2 :]  # Remove the packet and delimiter

            # Process the packet
            try:
                # Decode the packet or process raw data
                if len(packet) == packet_size:
                    value = int.from_bytes(packet[:2], byteorder="little")
                    data.append(value)
                    packet_count += 1
                else:
                    print(f"Unexpected packet size: {len(packet)}")
            except Exception as e:
                print(f"Packet processing error: {e}")
    except Exception as e:
        print(f"Error while reading packets: {e}")


def plot_update(ser):
    """Update the plot with the latest data."""
    global packet_count, last_update_time, smooth_buffer_len

    while plot_running:
        # Read and process all available packets
        read_packets(ser)

        # Update the plot
        line.set_ydata(data)

        # Calculate and display debug values
        current_time = time.time()
        elapsed_time = current_time - last_update_time
        if elapsed_time >= 1.0:
            data_rate = packet_count / elapsed_time
            packet_count = 0

            data_rate_text.set_text(f"Data Rate: {data_rate:.2f} Hz")
            buffer_size_text.set_text(f"Debt Size: {int(smooth_buffer_len)} bytes")

            last_update_time = current_time

        # Redraw the plot
        fig.canvas.draw_idle()
        plt.pause(0.01)  # Allow matplotlib to process GUI events


def main():
    """Main loop for reading serial data and updating the plot."""
    global plot_running

    # Connect the close event handler
    fig.canvas.mpl_connect("close_event", on_close)

    # Open serial connection
    try:
        with serial.Serial(serial_port, baud_rate, timeout=1) as ser:
            print(f"Listening on {serial_port} at {baud_rate} baud...")

            # Main loop
            plot_update(ser)

    except Exception as e:
        print(f"Serial connection error: {e}")
    finally:
        print("Exiting program.")


if __name__ == "__main__":
    main()

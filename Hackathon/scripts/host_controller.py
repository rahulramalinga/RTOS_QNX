import socket
import pygame
import time
import struct

# --- Config ---
# Replace with the QNX VM's static IP address if it changes
VM_IP = "192.168.136.130"
VM_PORT = 5000

# UDP Packet Structure (matches controller_input.h)
# 6 floats (24 bytes) + 1 uint32 (4 bytes) = 28 bytes
# Struct Format: 'ffffffI'
# axes: LS_X, LS_Y, RS_X, RS_Y, LT, RT
# buttons: bitmask

def main():
    pygame.init()
    pygame.joystick.init()

    if pygame.joystick.get_count() == 0:
        print("Error: No Xbox controller found!")
        return

    joy = pygame.joystick.Joystick(0)
    joy.init()
    print(f"Connected to: {joy.get_name()}")

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    print(f"Streaming controller data to {VM_IP}:{VM_PORT}...")

    try:
        while True:
            pygame.event.pump()

            # Read Axes (-1.0 to 1.0)
            ls_x = joy.get_axis(0)
            ls_y = joy.get_axis(1)
            rs_x = joy.get_axis(2)
            rs_y = joy.get_axis(3)
            # Triggers: pygame maps them 0 to 1 normally, but sometimes -1 to 1
            lt = (joy.get_axis(4) + 1) / 2.0 if joy.get_numaxes() > 4 else 0
            rt = (joy.get_axis(5) + 1) / 2.0 if joy.get_numaxes() > 5 else 0

            # Read Buttons (bitmask)
            buttons = 0
            for i in range(joy.get_numbuttons()):
                if joy.get_button(i):
                    buttons |= (1 << i)

            # Pack data (Little Endian)
            packet = struct.pack('<ffffffI', ls_x, ls_y, rs_x, rs_y, lt, rt, buttons)
            sock.sendto(packet, (VM_IP, VM_PORT))

            time.sleep(0.016)  # ~60 Hz

    except KeyboardInterrupt:
        print("\nStreaming stopped.")
    finally:
        pygame.quit()

if __name__ == "__main__":
    main()

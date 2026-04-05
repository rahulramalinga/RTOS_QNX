# Event Horizon: RTOS Mastery Manual
## QNX SDP 8.0 — Real-Time Systems Showcase

This project, **Event Horizon: Reactor Crisis**, is more than a game; it is a high-fidelity simulation of an **Asynchronous Real-Time Operating System (RTOS)** scheduler. Every mechanic maps directly to core OS and RTOS principles.

---

### 1. Task Management & Scheduling
**Concept**: Preemptive Multitasking & Priority.
- **In-Game**: The 5 subsystems (Defense, Nav, Reactor, Comms, Repairs) are conceptually separate threads running concurrently in the background.
- **RTOS Mapping**: Switching between full-screen views is analogous to a **Context Switch**. The Tab-Bar Alerts (Red/Yellow) represent **High-Priority Interrupt Service Requests (ISRs)** that demand immediate attention from the "Kernel" (the player).

### 2. Strategic Resource Allocation (The Energy Pool)
**Concept**: Resource Contention & Mutual Exclusion.
- **In-Game**: All automated bots draw from a single `Energy` pool. If energy hits zero, all bots fail.
- **RTOS Mapping**: This simulates **CPU Cycle Allocation** or **Shared Memory segments**. Over-provisioning (running all bots at once) leads to "Starvation" of other critical ship systems, mirroring real-world system stability issues under extreme load.

### 3. Asynchronous IPC (Inter-Process Communication)
**Concept**: Message Passing & Data Sync.
- **In-Game**: The **Navigation** and **Comms** tasks receive "Command Center" data asynchronously.
- **RTOS Mapping**: This uses the `MsgSend` and `MsgReceive` patterns (simulated in the logic) to bridge data between separate "processes" (the ship's bridge and the remote station).

### 4. Determinism & Low Latency (UDP Driver)
**Concept**: Real-Time Determinism vs. Best-Effort Latency.
- **In-Game**: The UDP Controller Driver (v5.0) bypasses the "Best-Effort" USB passthrough of the VM for "Real-Time" 60Hz precision.
- **RTOS Mapping**: This demonstrates the need for **Low-Latency Interrupt Latency**. In an RTOS, missing a 16ms deadline (one frame) can be catastrophic; the UDP driver ensures the "Control Signal" (Xbox Input) arrives before the next scheduler tick.

### 5. Kernel-Level Maintenance (Manual Repairs)
**Concept**: Privilege Escalation & Manual Override.
- **In-Game**: Repairs can NEVER be automated; the "Kernel" (Player) must manually intervene.
- **RTOS Mapping**: This represents **Critical Kernel Space routines** that cannot be delegated to User Space bots for safety reasons. It ensures a human-in-the-loop for non-deterministic hardware failure recovery.

---
**Build Status**: Production Stable (QNX 8.0 Target)
**Graphics**: Software-Rendered 60 FPS (Zero GPU Acceleration)
**Architecture**: Event-Driven, Fully Parallel Task System

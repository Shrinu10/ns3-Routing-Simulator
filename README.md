# NS-3 Routing Simulator

A configurable network routing simulator built using **NS-3.48** for studying routing behavior, packet forwarding, and network performance across multiple network topologies.

This project is a port of a custom C++ routing simulator into the NS-3 ecosystem, enabling realistic packet-level simulation, performance analysis using FlowMonitor, and visualization through NetAnim.

---

# Features

* Multiple network topologies

  * Random Connected Graph
  * Ring Network
  * Full Mesh
  * Binary Tree

* Configurable network size

  * Tested from **20 to 500 routers**

* UDP-based packet transmission

* IPv4 routing using NS-3 Global Routing

* FlowMonitor integration

* NetAnim visualization support

* Command-line configurable simulation parameters

* Docker support for reproducible builds

---

# Technologies Used

| Component            | Technology  |
| -------------------- | ----------- |
| Simulation Framework | NS-3.48     |
| Programming Language | C++17       |
| Build System         | CMake       |
| Visualization        | NetAnim     |
| Performance Analysis | FlowMonitor |
| Containerization     | Docker      |

---

# Supported Topologies

| Topology | Description                  |
| -------- | ---------------------------- |
| Random   | Random connected graph       |
| Ring     | Circular chain of routers    |
| Mesh     | Fully connected network      |
| Tree     | Binary hierarchical topology |

---

# Simulation Metrics

The simulator measures:

* Packet Delivery Ratio (PDR)
* Average End-to-End Delay
* Throughput
* Packet Loss
* Flow Statistics

---

# Repository Structure

```text
ns3-Routing-Simulator/
│
├── src/
│   └── routing-sim.cc
│
├── docker/
│   └── Dockerfile
│
├── screenshots/
│
└── README.md
```

---

# Prerequisites

Install:

* NS-3.48
* CMake
* GCC/G++
* Python 3.x

---

# Build Instructions

## Configure

```bash
./ns3 configure --enable-examples
```

## Build

```bash
./ns3 build
```

## Run

```bash
./ns3 run routing-sim
```

---

# Command Line Parameters

| Parameter     | Description                 | Default     |
| ------------- | --------------------------- | ----------- |
| --routers     | Number of routers           | 20          |
| --topology    | random / ring / mesh / tree | random      |
| --packets     | Number of packets           | 100         |
| --packetSize  | Packet size (bytes)         | 512         |
| --source      | Source router ID            | 0           |
| --destination | Destination router ID       | Last Router |
| --dataRate    | Link bandwidth              | 10Mbps      |
| --linkDelay   | Link delay                  | 5ms         |
| --randomDelay | Enable random delay         | false       |
| --delayMin    | Minimum delay               | 1ms         |
| --delayMax    | Maximum delay               | 10ms        |
| --seed        | Random seed                 | 42          |

---

# Example Commands

## Random Topology

```bash
./ns3 run "routing-sim --routers=100 --topology=random"
```

## Ring Topology

```bash
./ns3 run "routing-sim --routers=50 --topology=ring"
```

## Mesh Topology

```bash
./ns3 run "routing-sim --routers=20 --topology=mesh"
```

## Tree Topology

```bash
./ns3 run "routing-sim --routers=31 --topology=tree"
```

## Custom Packet Count

```bash
./ns3 run "routing-sim --routers=100 --packets=500"
```

## Random Link Delays

```bash
./ns3 run "routing-sim --routers=50 --randomDelay=true --delayMin=1 --delayMax=10"
```

---

# Docker Support

The simulator can be built and executed inside Docker without installing NS-3 dependencies locally.

## Build Docker Image

```bash
docker build -t ns3-routing-simulator .
```

## Run Simulation

```bash
docker run --rm ns3-routing-simulator
```

## Run with Custom Arguments

```bash
docker run --rm ns3-routing-simulator \
routing-sim --routers=100 --topology=random
```

---

# NetAnim Visualization

The simulation automatically generates:

```text
animation.xml
```

after execution.

## Launch NetAnim

```bash
netanim
```

or

```bash
~/netanim/build/netanim
```

## Visualizing Results

1. Open NetAnim
2. Click **File → Open**
3. Select `animation.xml`
4. Press **Play**
5. Observe packet movement through the network

---

# Experimental Results

Initial experiments were conducted across all supported topologies.

| Topology | Routers | Packet Delivery Ratio |
| -------- | ------- | --------------------- |
| Random   | 20      | 100%                  |
| Ring     | 10      | 100%                  |
| Tree     | 15      | 100%                  |
| Mesh     | 8       | 100%                  |

Key observations:

* Mesh topology achieves the lowest delay.
* Tree topology introduces higher path lengths.
* Ring topology provides deterministic routing.
* Random topology balances connectivity and scalability.
* All tested configurations achieved 100% packet delivery.

---

# Future Work

Potential extensions include:

* Privacy-preserving routing
* Secret-sharing based routing protocols
* Distributed routing algorithms
* Multi-path routing
* Secure forwarding mechanisms

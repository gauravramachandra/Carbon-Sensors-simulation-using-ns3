# Carbon Sensors Simulation using ns-3

This repository contains two ns-3 C++ scenarios for an Eco Ledger style carbon monitoring platform:

- `scenarios/iot-connectivity.cc`: Single-tier WiFi network with multiple CO2 sensors sending UDP data to a gateway.
- `scenarios/iot-hierarchical.cc`: Hierarchical topology with 5 WiFi zones (2 sensors each) forwarding via local APs over a CSMA backbone to a main gateway.

The repo also includes Python visualizers for generating dashboards.

## Requirements

- ns-3.45 (or similar)
- C++17 toolchain
- Python 3 with matplotlib and numpy (for visualizations)
- Optional: NetAnim (for XML animation) and Wireshark (for PCAP analysis)

## Quick Start (using an existing ns-3 checkout)

1. Copy a scenario into your ns-3 `scratch/` folder:

Copy the scenarios into your ns-3 scratch folder. On Windows PowerShell, you can use Copy-Item:

- Copy-Item scenarios/iot-hierarchical.cc <path-to-ns-3>/scratch/
- Copy-Item scenarios/iot-connectivity.cc <path-to-ns-3>/scratch/

2. Run from the ns-3 root:

From the ns-3 root, run (PowerShell uses ./ prefix):

- Hierarchical (5 zones, 2 sensors each):
  - ./ns3 run "scratch/iot-hierarchical --nZones=5 --time=30"
- Single-tier:
  - ./ns3 run "scratch/iot-connectivity --nSensors=5 --time=50"

3. Visualize (optional):

- NetAnim (animation): open `hierarchical-carbon-trading.xml` (or `carbon-trading-animation.xml`)
- Flow Monitor: view `hierarchical-flowmon.xml` / `carbon-trading-flowmon.xml`
- Wireshark: open generated `*.pcap` files
- Python plots:

In this folder:

- python visualize_hierarchical_data.py
- python visualize_carbon_data.py

## Scenario details

### iot-connectivity.cc

- WiFi 802.11b, ConstantRateWifiManager at DsssRate1Mbps
- UDP from sensors → gateway
- Periodic sensor emissions every 5s
- NetAnim + FlowMonitor + PCAP enabled

### iot-hierarchical.cc

- 5 zones, each a separate 802.11b WiFi network
- Each zone has 2 sensors → local AP
- All APs connect to a CSMA backbone to the main gateway
- Static routing (sensors default to AP, APs default to gateway)
- UDP-only communication
- NetAnim + FlowMonitor + PCAP enabled

## What not to commit

Add a `.gitignore` to exclude build artifacts and generated outputs:

```
# ns-3/build
build/
cmake-cache/
CMakeFiles/
compile_commands.json
*.o
*.a
*.so
*.dll
*.dylib
*.exe

# Simulation outputs
*.pcap
*.pcapng
*.xml
*.tr
*.log
*.png
*.traces

# Editors/OS
.vscode/
.DS_Store
Thumbs.db
```

## License

This repo contains your custom scenarios and docs; ns-3 itself remains licensed as per the upstream project. Ensure you refer users to install ns-3 separately.

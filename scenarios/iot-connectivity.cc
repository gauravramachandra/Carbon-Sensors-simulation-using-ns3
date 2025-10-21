/*
 * Eco Ledger IoT Carbon Trading Platform Simulation
 *
 * This simulation models an IoT network for carbon monitoring and trading.
 * Multiple CO2 sensor devices periodically measure and transmit carbon dioxide
 * levels to a central gateway node that collects data for carbon trading purposes.
 *
 * Network Architecture:
 * - Multiple IoT sensor nodes (CO2 sensors deployed at different locations)
 * - One central gateway node (data collector for carbon trading platform)
 * - WiFi-based communication infrastructure
 * - UDP protocol for sensor data transmission
 *
 * Carbon Trading Workflow:
 * 1. Sensor nodes measure CO2 levels at their locations
 * 2. Data is transmitted via WiFi to the central gateway
 * 3. Gateway logs sensor data with timestamps and company IDs
 * 4. This data feeds into carbon credit calculation systems
 * 5. Companies can monitor their carbon footprint in real-time
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"

#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("EcoLedgerCarbonTrading");

// Global variables for tracking carbon data
std::map<uint32_t, double> totalCO2BySensor;      // Track total CO2 per sensor
std::map<uint32_t, uint32_t> packetCountBySensor; // Track packets per sensor
uint32_t totalPacketsReceived = 0;
uint32_t totalPacketsSent = 0;

/*
 * Custom Application: CO2 Sensor Node
 *
 * This application simulates an IoT CO2 sensor that:
 * - Periodically generates CO2 readings (simulated values)
 * - Packages the data with sensor ID and company information
 * - Transmits data to the gateway via UDP
 *
 * In a real carbon trading scenario, these sensors would be deployed at:
 * - Manufacturing facilities
 * - Transportation hubs
 * - Energy production sites
 * - Agricultural operations
 */
class CO2SensorApplication : public Application
{
  public:
    CO2SensorApplication();
    virtual ~CO2SensorApplication();

    /**
     * Setup the sensor application
     * @param socket UDP socket for transmission
     * @param gatewayAddress Gateway IP address
     * @param gatewayPort Gateway listening port
     * @param sensorId Unique sensor identifier
     * @param companyId Company/organization identifier
     * @param baselineCO2 Baseline CO2 level for this sensor location
     */
    void Setup(Ptr<Socket> socket,
               Address gatewayAddress,
               uint16_t gatewayPort,
               uint32_t sensorId,
               uint32_t companyId,
               double baselineCO2);

  private:
    virtual void StartApplication(void);
    virtual void StopApplication(void);

    /**
     * Generate and send CO2 sensor reading
     * Simulates reading from physical CO2 sensor and transmitting to gateway
     */
    void SendCO2Reading(void);

    /**
     * Generate realistic CO2 value
     * Simulates actual sensor readings with variations
     */
    double GenerateCO2Value(void);

    Ptr<Socket> m_socket;
    Address m_gatewayAddress;
    uint16_t m_gatewayPort;
    uint32_t m_sensorId;
    uint32_t m_companyId;
    double m_baselineCO2; // Baseline CO2 level (ppm)
    EventId m_sendEvent;
    Time m_interval; // Time between sensor readings
    bool m_running;
};

CO2SensorApplication::CO2SensorApplication()
    : m_socket(0),
      m_gatewayPort(0),
      m_sensorId(0),
      m_companyId(0),
      m_baselineCO2(400.0),     // Normal atmospheric CO2 ~400 ppm
      m_interval(Seconds(5.0)), // Send reading every 5 seconds
      m_running(false)
{
}

CO2SensorApplication::~CO2SensorApplication()
{
    m_socket = 0;
}

void
CO2SensorApplication::Setup(Ptr<Socket> socket,
                            Address gatewayAddress,
                            uint16_t gatewayPort,
                            uint32_t sensorId,
                            uint32_t companyId,
                            double baselineCO2)
{
    m_socket = socket;
    m_gatewayAddress = gatewayAddress;
    m_gatewayPort = gatewayPort;
    m_sensorId = sensorId;
    m_companyId = companyId;
    m_baselineCO2 = baselineCO2;
}

void
CO2SensorApplication::StartApplication(void)
{
    m_running = true;
    m_socket->Bind();
    m_socket->Connect(m_gatewayAddress);

    NS_LOG_INFO("CO2 Sensor " << m_sensorId << " (Company " << m_companyId << ") started at "
                              << Simulator::Now().GetSeconds() << "s");

    // Send first reading immediately
    SendCO2Reading();
}

void
CO2SensorApplication::StopApplication(void)
{
    m_running = false;

    if (m_sendEvent.IsPending())
    {
        Simulator::Cancel(m_sendEvent);
    }

    if (m_socket)
    {
        m_socket->Close();
    }

    NS_LOG_INFO("CO2 Sensor " << m_sensorId << " stopped at " << Simulator::Now().GetSeconds()
                              << "s");
}

double
CO2SensorApplication::GenerateCO2Value(void)
{
    // Generate realistic CO2 values with random variations
    // Industrial sites typically have 400-2000 ppm CO2
    // Add random variation ±50 ppm to simulate real sensor readings

    Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
    double variation = rand->GetValue(-50.0, 50.0);
    double co2Value = m_baselineCO2 + variation;

    // Ensure value stays positive and realistic
    if (co2Value < 300.0)
    {
        co2Value = 300.0;
    }
    if (co2Value > 3000.0)
    {
        co2Value = 3000.0;
    }

    return co2Value;
}

void
CO2SensorApplication::SendCO2Reading(void)
{
    /*
     * Carbon Trading Data Packet Format:
     * [SensorID:4bytes][CompanyID:4bytes][CO2Value:8bytes][Timestamp:8bytes]
     *
     * This packet contains all necessary information for carbon accounting:
     * - Which sensor detected the emission (location tracking)
     * - Which company/facility owns the sensor (accountability)
     * - CO2 level in ppm (carbon footprint data)
     * - When the reading was taken (temporal tracking)
     */

    double co2Value = GenerateCO2Value();
    uint64_t timestamp = Simulator::Now().GetMicroSeconds();

    // Create packet with sensor data
    std::ostringstream oss;
    oss << "SENSOR:" << m_sensorId << ",COMPANY:" << m_companyId << ",CO2:" << co2Value
        << ",TIME:" << timestamp;

    std::string data = oss.str();
    Ptr<Packet> packet = Create<Packet>((uint8_t*)data.c_str(), data.length());

    // Transmit to gateway
    int actual = m_socket->Send(packet);

    if (actual > 0)
    {
        totalPacketsSent++;
        NS_LOG_INFO("Time " << Simulator::Now().GetSeconds() << "s: "
                            << "Sensor " << m_sensorId << " (Company " << m_companyId << ") "
                            << "transmitted CO2 reading: " << co2Value << " ppm "
                            << "[Packet " << totalPacketsSent << " sent to gateway]");
    }
    else
    {
        NS_LOG_WARN("Sensor " << m_sensorId << " failed to send packet");
    }

    // Schedule next reading
    if (m_running)
    {
        m_sendEvent = Simulator::Schedule(m_interval, &CO2SensorApplication::SendCO2Reading, this);
    }
}

/*
 * Custom Application: Carbon Trading Gateway
 *
 * This application simulates the central data collection gateway that:
 * - Receives CO2 data from all sensor nodes
 * - Parses and logs sensor readings
 * - Aggregates data for carbon trading calculations
 * - Sends acknowledgments back to sensors
 *
 * In a real system, this gateway would:
 * - Store data in a blockchain ledger for transparency
 * - Calculate carbon credits/debits
 * - Generate reports for regulatory compliance
 * - Enable carbon credit trading between companies
 */
class CarbonGatewayApplication : public Application
{
  public:
    CarbonGatewayApplication();
    virtual ~CarbonGatewayApplication();

    void Setup(Ptr<Socket> socket, uint16_t port);

  private:
    virtual void StartApplication(void);
    virtual void StopApplication(void);

    /**
     * Handle incoming CO2 sensor data
     * Processes sensor readings for carbon trading platform
     */
    void HandleRead(Ptr<Socket> socket);

    /**
     * Parse and process CO2 sensor data packet
     * Extracts sensor info and logs for carbon accounting
     */
    void ProcessCO2Data(Ptr<Packet> packet, Address from);

    /**
     * Send acknowledgment back to sensor
     * Confirms data receipt for reliability
     */
    void SendAcknowledgment(Ptr<Socket> socket, Address to, uint32_t sensorId);

    Ptr<Socket> m_socket;
    uint16_t m_port;
    Address m_local;
};

CarbonGatewayApplication::CarbonGatewayApplication()
    : m_socket(0),
      m_port(0)
{
}

CarbonGatewayApplication::~CarbonGatewayApplication()
{
    m_socket = 0;
}

void
CarbonGatewayApplication::Setup(Ptr<Socket> socket, uint16_t port)
{
    m_socket = socket;
    m_port = port;
}

void
CarbonGatewayApplication::StartApplication(void)
{
    // Bind socket to listen for incoming sensor data
    InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), m_port);
    m_socket->Bind(local);
    m_socket->SetRecvCallback(MakeCallback(&CarbonGatewayApplication::HandleRead, this));

    NS_LOG_INFO("Carbon Trading Gateway started on port " << m_port << " at time "
                                                          << Simulator::Now().GetSeconds() << "s");
    NS_LOG_INFO("Gateway ready to receive CO2 sensor data for carbon accounting...");
}

void
CarbonGatewayApplication::StopApplication(void)
{
    if (m_socket)
    {
        m_socket->Close();
        m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    }

    NS_LOG_INFO("Carbon Trading Gateway stopped at " << Simulator::Now().GetSeconds() << "s");

    // Print final carbon accounting summary
    NS_LOG_INFO("=== CARBON TRADING SUMMARY ===");
    NS_LOG_INFO("Total sensor readings received: " << totalPacketsReceived);

    for (const auto& entry : totalCO2BySensor)
    {
        uint32_t sensorId = entry.first;
        double totalCO2 = entry.second;
        uint32_t packetCount = packetCountBySensor[sensorId];
        double avgCO2 = totalCO2 / packetCount;

        NS_LOG_INFO("Sensor " << sensorId << ": " << packetCount << " readings, "
                              << "Average CO2 = " << avgCO2 << " ppm");
    }
}

void
CarbonGatewayApplication::HandleRead(Ptr<Socket> socket)
{
    Ptr<Packet> packet;
    Address from;

    while ((packet = socket->RecvFrom(from)))
    {
        if (packet->GetSize() > 0)
        {
            totalPacketsReceived++;
            ProcessCO2Data(packet, from);

            // In a real system, gateway would send acknowledgment
            // For now, we just log the receipt
        }
    }
}

void
CarbonGatewayApplication::ProcessCO2Data(Ptr<Packet> packet, Address from)
{
    /*
     * Carbon Accounting Data Processing:
     *
     * The gateway performs several critical functions:
     * 1. Validates sensor data integrity
     * 2. Extracts carbon emissions information
     * 3. Associates data with specific companies/facilities
     * 4. Logs data for regulatory compliance
     * 5. Aggregates data for carbon credit calculations
     *
     * This data forms the basis for:
     * - Carbon footprint reporting
     * - Carbon credit generation (for companies under emission limits)
     * - Carbon debit calculation (for companies exceeding limits)
     * - Inter-company carbon credit trading
     * - Regulatory reporting and compliance verification
     */

    uint8_t buffer[1024];
    packet->CopyData(buffer, packet->GetSize());
    buffer[packet->GetSize()] = '\0';

    std::string data((char*)buffer);

    // Parse sensor data packet
    uint32_t sensorId = 0;
    uint32_t companyId = 0;
    double co2Value = 0.0;
    uint64_t timestamp = 0;

    size_t sensorPos = data.find("SENSOR:");
    size_t companyPos = data.find("COMPANY:");
    size_t co2Pos = data.find("CO2:");
    size_t timePos = data.find("TIME:");

    if (sensorPos != std::string::npos && companyPos != std::string::npos &&
        co2Pos != std::string::npos && timePos != std::string::npos)
    {
        // Extract values from packet
        sensorId = std::stoi(data.substr(sensorPos + 7, companyPos - sensorPos - 8));
        companyId = std::stoi(data.substr(companyPos + 8, co2Pos - companyPos - 9));
        co2Value = std::stod(data.substr(co2Pos + 4, timePos - co2Pos - 5));
        timestamp = std::stoull(data.substr(timePos + 5));

        // Update carbon accounting records
        totalCO2BySensor[sensorId] += co2Value;
        packetCountBySensor[sensorId]++;

        InetSocketAddress inetFrom = InetSocketAddress::ConvertFrom(from);

        NS_LOG_INFO("Time " << Simulator::Now().GetSeconds() << "s: "
                            << "Gateway received CO2 data from Sensor " << sensorId << " (Company "
                            << companyId << ") "
                            << "- CO2 Level: " << co2Value << " ppm "
                            << "[Source: " << inetFrom.GetIpv4() << "] "
                            << "[Packet " << totalPacketsReceived << " logged for carbon trading]");

        // In a production system, this data would be:
        // - Written to blockchain for immutable record
        // - Forwarded to carbon credit calculation engine
        // - Used to update company carbon balance
        // - Made available for carbon trading marketplace
    }
    else
    {
        NS_LOG_WARN("Gateway received malformed packet from "
                    << InetSocketAddress::ConvertFrom(from).GetIpv4());
    }
}

void
CarbonGatewayApplication::SendAcknowledgment(Ptr<Socket> socket, Address to, uint32_t sensorId)
{
    std::ostringstream oss;
    oss << "ACK:SENSOR:" << sensorId;
    std::string ack = oss.str();

    Ptr<Packet> ackPacket = Create<Packet>((uint8_t*)ack.c_str(), ack.length());
    socket->SendTo(ackPacket, 0, to);
}

/*
 * Main Simulation Setup
 *
 * This function sets up the complete carbon trading IoT network:
 * - Creates sensor nodes and gateway node
 * - Configures WiFi networking infrastructure
 * - Assigns IP addresses
 * - Deploys CO2 sensor applications
 * - Initializes gateway application
 * - Runs the simulation
 */
int
main(int argc, char* argv[])
{
    /*
     * ============================================
     * SIMULATION PARAMETERS
     * ============================================
     */

    // Number of CO2 sensor nodes in the network
    // In a real deployment, these would be distributed across
    // multiple facilities, factories, or monitoring sites
    uint32_t nSensors = 5;

    // Simulation duration (seconds)
    double simulationTime = 50.0;

    // Gateway listening port
    uint16_t gatewayPort = 9000;

    // Enable detailed logging for carbon data flow
    bool verbose = true;

    // Parse command line arguments
    CommandLine cmd;
    cmd.AddValue("nSensors", "Number of CO2 sensor nodes", nSensors);
    cmd.AddValue("time", "Simulation time in seconds", simulationTime);
    cmd.AddValue("verbose", "Enable verbose logging", verbose);
    cmd.Parse(argc, argv);

    if (verbose)
    {
        LogComponentEnable("EcoLedgerCarbonTrading", LOG_LEVEL_INFO);
    }

    NS_LOG_INFO("=================================================");
    NS_LOG_INFO("Eco Ledger Carbon Trading IoT Network Simulation");
    NS_LOG_INFO("=================================================");
    NS_LOG_INFO("Number of CO2 sensor nodes: " << nSensors);
    NS_LOG_INFO("Simulation duration: " << simulationTime << " seconds");
    NS_LOG_INFO("Gateway port: " << gatewayPort);
    NS_LOG_INFO("=================================================");

    /*
     * ============================================
     * NODE CREATION
     * ============================================
     */

    NS_LOG_INFO("Creating network nodes...");

    // Create sensor nodes (IoT CO2 sensors)
    NodeContainer sensorNodes;
    sensorNodes.Create(nSensors);

    // Create gateway node (central data collector)
    NodeContainer gatewayNode;
    gatewayNode.Create(1);

    // Combine all nodes for network setup
    NodeContainer allNodes;
    allNodes.Add(sensorNodes);
    allNodes.Add(gatewayNode);

    NS_LOG_INFO("Created " << nSensors << " sensor nodes and 1 gateway node");

    /*
     * ============================================
     * WIFI NETWORK CONFIGURATION
     * ============================================
     *
     * WiFi is used for the carbon trading network because:
     * - Easy deployment in industrial facilities
     * - Good range for facility-wide coverage
     * - Adequate bandwidth for sensor data
     * - Cost-effective for IoT applications
     */

    NS_LOG_INFO("Configuring WiFi network...");

    // WiFi channel configuration
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    // WiFi MAC layer configuration
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b); // Use 802.11b standard
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("DsssRate1Mbps"),
                                 "ControlMode",
                                 StringValue("DsssRate1Mbps"));

    WifiMacHelper mac;
    Ssid ssid = Ssid("EcoLedger-CarbonNet"); // Network name

    // Configure sensor nodes as WiFi stations
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false));

    NetDeviceContainer sensorDevices;
    sensorDevices = wifi.Install(phy, mac, sensorNodes);

    // Configure gateway as WiFi access point
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));

    NetDeviceContainer gatewayDevice;
    gatewayDevice = wifi.Install(phy, mac, gatewayNode);

    NS_LOG_INFO("WiFi network configured with SSID: EcoLedger-CarbonNet");

    /*
     * ============================================
     * MOBILITY MODEL
     * ============================================
     *
     * Positions nodes in the network:
     * - Sensors distributed in a line (simulating facility layout)
     * - Gateway positioned centrally for optimal coverage
     */

    NS_LOG_INFO("Setting up node positions...");

    MobilityHelper mobility;

    // Position sensor nodes in a line (20m apart)
    // Simulates sensors along a production line or facility perimeter
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX",
                                  DoubleValue(0.0),
                                  "MinY",
                                  DoubleValue(0.0),
                                  "DeltaX",
                                  DoubleValue(20.0),
                                  "DeltaY",
                                  DoubleValue(0.0),
                                  "GridWidth",
                                  UintegerValue(nSensors),
                                  "LayoutType",
                                  StringValue("RowFirst"));

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(sensorNodes);

    // Position gateway at central location
    Ptr<ListPositionAllocator> gatewayPosition = CreateObject<ListPositionAllocator>();
    gatewayPosition->Add(Vector(40.0, 10.0, 0.0)); // Central position
    mobility.SetPositionAllocator(gatewayPosition);
    mobility.Install(gatewayNode);

    NS_LOG_INFO("Nodes positioned: Sensors in line, Gateway at center");

    /*
     * ============================================
     * INTERNET STACK AND IP ADDRESSING
     * ============================================
     *
     * Install TCP/IP stack and assign IP addresses
     * This enables network-layer communication for sensor data
     */

    NS_LOG_INFO("Installing Internet stack...");

    InternetStackHelper internet;
    internet.Install(allNodes);

    // Assign IP addresses
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");

    Ipv4InterfaceContainer sensorInterfaces;
    sensorInterfaces = address.Assign(sensorDevices);

    Ipv4InterfaceContainer gatewayInterface;
    gatewayInterface = address.Assign(gatewayDevice);

    NS_LOG_INFO("IP addresses assigned:");
    NS_LOG_INFO("  Sensor network: 10.1.1.1 - 10.1.1." << nSensors);
    NS_LOG_INFO("  Gateway: 10.1.1." << (nSensors + 1));

    // Enable routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    /*
     * ============================================
     * APPLICATION DEPLOYMENT
     * ============================================
     */

    NS_LOG_INFO("Deploying applications...");

    // Get gateway IP address
    Ipv4Address gatewayAddr = gatewayInterface.GetAddress(0);

    // Create and configure gateway application
    Ptr<Socket> gatewaySocket =
        Socket::CreateSocket(gatewayNode.Get(0), UdpSocketFactory::GetTypeId());
    Ptr<CarbonGatewayApplication> gatewayApp = CreateObject<CarbonGatewayApplication>();
    gatewayApp->Setup(gatewaySocket, gatewayPort);
    gatewayNode.Get(0)->AddApplication(gatewayApp);
    gatewayApp->SetStartTime(Seconds(0.0));
    gatewayApp->SetStopTime(Seconds(simulationTime));

    NS_LOG_INFO("Gateway application deployed at " << gatewayAddr << ":" << gatewayPort);

    // Create and configure sensor applications
    for (uint32_t i = 0; i < nSensors; ++i)
    {
        Ptr<Socket> sensorSocket =
            Socket::CreateSocket(sensorNodes.Get(i), UdpSocketFactory::GetTypeId());
        Ptr<CO2SensorApplication> sensorApp = CreateObject<CO2SensorApplication>();

        // Assign different baseline CO2 levels to simulate different facility types
        // Factory: ~800 ppm, Office: ~500 ppm, Warehouse: ~450 ppm, etc.
        double baselineCO2 = 400.0 + (i * 100.0);

        // Each sensor belongs to a company (for demo, use sensor ID as company ID)
        uint32_t companyId = (i % 3) + 1; // 3 different companies

        Address gatewayAddress = InetSocketAddress(gatewayAddr, gatewayPort);
        sensorApp->Setup(sensorSocket, gatewayAddress, gatewayPort, i + 1, companyId, baselineCO2);

        sensorNodes.Get(i)->AddApplication(sensorApp);
        sensorApp->SetStartTime(Seconds(1.0 + i * 0.5)); // Stagger start times
        sensorApp->SetStopTime(Seconds(simulationTime));

        NS_LOG_INFO("Sensor " << (i + 1) << " deployed: "
                              << "Company " << companyId << ", "
                              << "Baseline CO2 = " << baselineCO2 << " ppm, "
                              << "IP = " << sensorInterfaces.GetAddress(i));
    }

    /*
     * ============================================
     * VISUALIZATION SETUP (NetAnim)
     * ============================================
     *
     * Creates an animation file that can be viewed with NetAnim
     * Shows node positions, packet transmissions, and network topology
     */

    NS_LOG_INFO("Setting up visualization...");

    AnimationInterface anim("carbon-trading-animation.xml");

    // Set node descriptions
    anim.UpdateNodeDescription(gatewayNode.Get(0), "Gateway");
    anim.UpdateNodeColor(gatewayNode.Get(0), 0, 255, 0);        // Green for gateway
    anim.UpdateNodeSize(gatewayNode.Get(0)->GetId(), 5.0, 5.0); // Larger size

    for (uint32_t i = 0; i < nSensors; ++i)
    {
        std::ostringstream oss;
        oss << "CO2_Sensor_" << (i + 1);
        anim.UpdateNodeDescription(sensorNodes.Get(i), oss.str());
        anim.UpdateNodeColor(sensorNodes.Get(i), 255, 0, 0); // Red for sensors
        anim.UpdateNodeSize(sensorNodes.Get(i)->GetId(), 3.0, 3.0);
    }

    anim.EnablePacketMetadata(true);
    anim.EnableIpv4RouteTracking("carbon-trading-routes.xml",
                                 Seconds(0),
                                 Seconds(simulationTime),
                                 Seconds(1));

    NS_LOG_INFO("Visualization configured - will generate carbon-trading-animation.xml");

    /*
     * ============================================
     * PCAP TRACING (Wireshark)
     * ============================================
     *
     * Enable packet capture for detailed network analysis
     */

    YansWifiPhyHelper phyTrace;
    phyTrace.SetChannel(channel.Create());
    phyTrace.EnablePcap("carbon-trading-wifi", gatewayDevice.Get(0), true);
    phyTrace.EnablePcap("carbon-trading-sensor", sensorDevices.Get(0), true);

    NS_LOG_INFO("PCAP tracing enabled for Wireshark analysis");

    /*
     * ============================================
     * ASCII TRACING
     * ============================================
     */

    AsciiTraceHelper ascii;
    phy.EnableAsciiAll(ascii.CreateFileStream("carbon-trading.tr"));

    /*
     * ============================================
     * FLOW MONITOR
     * ============================================
     *
     * Collects detailed statistics about network flows
     */

    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    /*
     * ============================================
     * SIMULATION EXECUTION
     * ============================================
     */

    NS_LOG_INFO("=================================================");
    NS_LOG_INFO("Starting simulation...");
    NS_LOG_INFO("=================================================");

    Simulator::Stop(Seconds(simulationTime));
    Simulator::Run();

    NS_LOG_INFO("=================================================");
    NS_LOG_INFO("Simulation completed");
    NS_LOG_INFO("=================================================");
    NS_LOG_INFO("Total packets sent by sensors: " << totalPacketsSent);
    NS_LOG_INFO("Total packets received by gateway: " << totalPacketsReceived);

    double deliveryRatio =
        (totalPacketsSent > 0) ? (double)totalPacketsReceived / totalPacketsSent * 100.0 : 0.0;
    NS_LOG_INFO("Packet delivery ratio: " << deliveryRatio << "%");

    NS_LOG_INFO("=================================================");

    // Also print to console
    std::cout << "\n=================================================\n";
    std::cout << "CARBON TRADING SIMULATION RESULTS\n";
    std::cout << "=================================================\n";
    std::cout << "Number of sensors: " << nSensors << "\n";
    std::cout << "Simulation time: " << simulationTime << " seconds\n";
    std::cout << "Total packets sent: " << totalPacketsSent << "\n";
    std::cout << "Total packets received: " << totalPacketsReceived << "\n";
    std::cout << "Packet delivery ratio: " << deliveryRatio << "%\n";
    std::cout << "\nCO2 Statistics by Sensor:\n";
    std::cout << "-------------------------------------------------\n";

    for (const auto& entry : totalCO2BySensor)
    {
        uint32_t sensorId = entry.first;
        double totalCO2 = entry.second;
        uint32_t packetCount = packetCountBySensor[sensorId];
        double avgCO2 = totalCO2 / packetCount;

        std::cout << "Sensor " << sensorId << ": " << packetCount << " readings, "
                  << "Average CO2 = " << std::fixed << std::setprecision(2) << avgCO2 << " ppm\n";
    }

    std::cout << "=================================================\n\n";

    /*
     * ============================================
     * FLOW MONITOR STATISTICS
     * ============================================
     */

    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

    std::cout << "\n=================================================\n";
    std::cout << "NETWORK FLOW STATISTICS\n";
    std::cout << "=================================================\n";

    for (const auto& flow : stats)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(flow.first);
        std::cout << "Flow " << flow.first << " (" << t.sourceAddress << " -> "
                  << t.destinationAddress << ")\n";
        std::cout << "  Tx Packets: " << flow.second.txPackets << "\n";
        std::cout << "  Rx Packets: " << flow.second.rxPackets << "\n";
        std::cout << "  Throughput: "
                  << flow.second.rxBytes * 8.0 /
                         (flow.second.timeLastRxPacket.GetSeconds() -
                          flow.second.timeFirstTxPacket.GetSeconds()) /
                         1024
                  << " Kbps\n";
        std::cout << "  Mean Delay: " << flow.second.delaySum.GetSeconds() / flow.second.rxPackets
                  << " s\n";
        std::cout << "-------------------------------------------------\n";
    }

    monitor->SerializeToXmlFile("carbon-trading-flowmon.xml", true, true);
    std::cout << "\nFlow monitor data saved to: carbon-trading-flowmon.xml\n";
    std::cout << "=================================================\n\n";

    // Write results to file
    std::ofstream outFile("carbon_trading_results.txt");
    if (outFile.is_open())
    {
        outFile << "Eco Ledger Carbon Trading Simulation Results\n";
        outFile << "=================================================\n\n";
        outFile << "Configuration:\n";
        outFile << "  Number of sensors: " << nSensors << "\n";
        outFile << "  Simulation time: " << simulationTime << " seconds\n\n";
        outFile << "Network Performance:\n";
        outFile << "  Total packets sent: " << totalPacketsSent << "\n";
        outFile << "  Total packets received: " << totalPacketsReceived << "\n";
        outFile << "  Packet delivery ratio: " << deliveryRatio << "%\n\n";
        outFile << "CO2 Monitoring Data:\n";
        outFile << "-------------------------------------------------\n";

        for (const auto& entry : totalCO2BySensor)
        {
            uint32_t sensorId = entry.first;
            double totalCO2 = entry.second;
            uint32_t packetCount = packetCountBySensor[sensorId];
            double avgCO2 = totalCO2 / packetCount;

            outFile << "Sensor " << sensorId << ":\n";
            outFile << "  Number of readings: " << packetCount << "\n";
            outFile << "  Average CO2 level: " << avgCO2 << " ppm\n";
            outFile << "  Total CO2 measured: " << totalCO2 << " ppm\n\n";
        }

        outFile << "=================================================\n";
        outFile << "Results saved at: " << Simulator::Now().GetSeconds() << "s\n";
        outFile.close();

        std::cout << "Results also saved to: carbon_trading_results.txt\n";
    }

    std::cout << "\n=================================================\n";
    std::cout << "VISUALIZATION FILES GENERATED\n";
    std::cout << "=================================================\n";
    std::cout << "1. NetAnim visualization: carbon-trading-animation.xml\n";
    std::cout << "   - Open with NetAnim to see animated network\n";
    std::cout << "2. Flow monitor: carbon-trading-flowmon.xml\n";
    std::cout << "3. PCAP files: carbon-trading-wifi-*.pcap\n";
    std::cout << "   - Open with Wireshark for packet analysis\n";
    std::cout << "4. ASCII trace: carbon-trading.tr\n";
    std::cout << "5. Route tracking: carbon-trading-routes.xml\n";
    std::cout << "=================================================\n\n";

    Simulator::Destroy();

    return 0;
}

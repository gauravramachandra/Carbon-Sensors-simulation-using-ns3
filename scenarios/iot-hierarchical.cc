/*
 * Hierarchical WiFi Network for IoT Carbon Trading Platform
 *
 * Network Architecture:
 * - 10 CO2 sensors grouped into 5 pairs (2 sensors per group)
 * - 5 WiFi Access Points (one for each sensor pair)
 * - 1 Main Gateway (central coordinator)
 * - Two-tier topology: Sensors → Local APs → Main Gateway
 *
 * This simulates a distributed industrial facility with:
 * - Multiple departments/zones (5 zones)
 * - Local data aggregation at zone level (5 APs)
 * - Central carbon trading platform (1 Gateway)
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/wifi-module.h"

#include <fstream>
#include <iostream>
#include <map>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("HierarchicalCarbonTrading");

// Global tracking variables
std::map<uint32_t, double> totalCO2BySensor;
std::map<uint32_t, uint32_t> packetCountBySensor;
uint32_t totalPacketsReceived = 0;
uint32_t totalPacketsSent = 0;

/*
 * CO2 Sensor Application (same as before)
 */
class CO2SensorApplication : public Application
{
  public:
    CO2SensorApplication();
    virtual ~CO2SensorApplication();

    void Setup(Ptr<Socket> socket,
               Address apAddress,
               uint16_t port,
               uint32_t sensorId,
               uint32_t zoneId,
               double baselineCO2);

  private:
    virtual void StartApplication(void);
    virtual void StopApplication(void);
    void SendCO2Reading(void);
    double GenerateCO2Value(void);

    Ptr<Socket> m_socket;
    Address m_apAddress;
    uint16_t m_port;
    uint32_t m_sensorId;
    uint32_t m_zoneId;
    double m_baselineCO2;
    EventId m_sendEvent;
    Time m_interval;
    bool m_running;
};

CO2SensorApplication::CO2SensorApplication()
    : m_socket(0),
      m_port(0),
      m_sensorId(0),
      m_zoneId(0),
      m_baselineCO2(400.0),
      m_interval(Seconds(5.0)),
      m_running(false)
{
}

CO2SensorApplication::~CO2SensorApplication()
{
    m_socket = 0;
}

void
CO2SensorApplication::Setup(Ptr<Socket> socket,
                            Address apAddress,
                            uint16_t port,
                            uint32_t sensorId,
                            uint32_t zoneId,
                            double baselineCO2)
{
    m_socket = socket;
    m_apAddress = apAddress;
    m_port = port;
    m_sensorId = sensorId;
    m_zoneId = zoneId;
    m_baselineCO2 = baselineCO2;
}

void
CO2SensorApplication::StartApplication(void)
{
    m_running = true;
    m_socket->Bind();
    m_socket->Connect(m_apAddress);
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
}

double
CO2SensorApplication::GenerateCO2Value(void)
{
    Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
    double variation = rand->GetValue(-50.0, 50.0);
    double co2Value = m_baselineCO2 + variation;
    return std::max(300.0, std::min(3000.0, co2Value));
}

void
CO2SensorApplication::SendCO2Reading(void)
{
    double co2Value = GenerateCO2Value();
    std::ostringstream oss;
    oss << "SENSOR:" << m_sensorId << ",ZONE:" << m_zoneId << ",CO2:" << co2Value;

    std::string data = oss.str();
    Ptr<Packet> packet = Create<Packet>((uint8_t*)data.c_str(), data.length());

    if (m_socket->Send(packet) > 0)
    {
        totalPacketsSent++;
        NS_LOG_INFO("Time " << Simulator::Now().GetSeconds() << "s: Sensor " << m_sensorId
                            << " (Zone " << m_zoneId << ") sent CO2: " << co2Value << " ppm");
    }

    if (m_running)
    {
        m_sendEvent = Simulator::Schedule(m_interval, &CO2SensorApplication::SendCO2Reading, this);
    }
}

/*
 * Local Access Point Application
 * Receives data from sensors and forwards to main gateway
 */
class LocalAPApplication : public Application
{
  public:
    LocalAPApplication();
    virtual ~LocalAPApplication();

    void Setup(Ptr<Socket> receiveSocket,
               Ptr<Socket> forwardSocket,
               uint16_t receivePort,
               Address gatewayAddress,
               uint32_t zoneId);

  private:
    virtual void StartApplication(void);
    virtual void StopApplication(void);
    void HandleRead(Ptr<Socket> socket);
    void ForwardToGateway(Ptr<Packet> packet);

    Ptr<Socket> m_receiveSocket;
    Ptr<Socket> m_forwardSocket;
    uint16_t m_receivePort;
    Address m_gatewayAddress;
    uint32_t m_zoneId;
    uint32_t m_packetsReceived;
    uint32_t m_packetsForwarded;
};

LocalAPApplication::LocalAPApplication()
    : m_receiveSocket(0),
      m_forwardSocket(0),
      m_receivePort(0),
      m_zoneId(0),
      m_packetsReceived(0),
      m_packetsForwarded(0)
{
}

LocalAPApplication::~LocalAPApplication()
{
}

void
LocalAPApplication::Setup(Ptr<Socket> receiveSocket,
                          Ptr<Socket> forwardSocket,
                          uint16_t receivePort,
                          Address gatewayAddress,
                          uint32_t zoneId)
{
    m_receiveSocket = receiveSocket;
    m_forwardSocket = forwardSocket;
    m_receivePort = receivePort;
    m_gatewayAddress = gatewayAddress;
    m_zoneId = zoneId;
}

void
LocalAPApplication::StartApplication(void)
{
    InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), m_receivePort);
    m_receiveSocket->Bind(local);
    m_receiveSocket->SetRecvCallback(MakeCallback(&LocalAPApplication::HandleRead, this));

    m_forwardSocket->Bind();
    m_forwardSocket->Connect(m_gatewayAddress);

    NS_LOG_INFO("Local AP Zone " << m_zoneId << " started on port " << m_receivePort);
}

void
LocalAPApplication::StopApplication(void)
{
    if (m_receiveSocket)
    {
        m_receiveSocket->Close();
    }
    if (m_forwardSocket)
    {
        m_forwardSocket->Close();
    }
    NS_LOG_INFO("Local AP Zone " << m_zoneId << ": Received=" << m_packetsReceived
                                 << ", Forwarded=" << m_packetsForwarded);
}

void
LocalAPApplication::HandleRead(Ptr<Socket> socket)
{
    Ptr<Packet> packet;
    Address from;

    while ((packet = socket->RecvFrom(from)))
    {
        if (packet->GetSize() > 0)
        {
            m_packetsReceived++;
            NS_LOG_INFO("Time " << Simulator::Now().GetSeconds() << "s: AP Zone " << m_zoneId
                                << " received packet from sensor");
            ForwardToGateway(packet);
        }
    }
}

void
LocalAPApplication::ForwardToGateway(Ptr<Packet> packet)
{
    // Add zone info and forward to main gateway
    if (m_forwardSocket->Send(packet) > 0)
    {
        m_packetsForwarded++;
        NS_LOG_INFO("Time " << Simulator::Now().GetSeconds() << "s: AP Zone " << m_zoneId
                            << " forwarded to main gateway");
    }
}

/*
 * Main Gateway Application
 * Receives forwarded data from all local APs
 */
class MainGatewayApplication : public Application
{
  public:
    MainGatewayApplication();
    virtual ~MainGatewayApplication();

    void Setup(Ptr<Socket> socket, uint16_t port);

  private:
    virtual void StartApplication(void);
    virtual void StopApplication(void);
    void HandleRead(Ptr<Socket> socket);
    void ProcessData(Ptr<Packet> packet, Address from);

    Ptr<Socket> m_socket;
    uint16_t m_port;
};

MainGatewayApplication::MainGatewayApplication()
    : m_socket(0),
      m_port(0)
{
}

MainGatewayApplication::~MainGatewayApplication()
{
}

void
MainGatewayApplication::Setup(Ptr<Socket> socket, uint16_t port)
{
    m_socket = socket;
    m_port = port;
}

void
MainGatewayApplication::StartApplication(void)
{
    InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), m_port);
    m_socket->Bind(local);
    m_socket->SetRecvCallback(MakeCallback(&MainGatewayApplication::HandleRead, this));
    NS_LOG_INFO("Main Gateway started on port " << m_port);
}

void
MainGatewayApplication::StopApplication(void)
{
    if (m_socket)
    {
        m_socket->Close();
    }
    NS_LOG_INFO("Main Gateway: Total packets received = " << totalPacketsReceived);
}

void
MainGatewayApplication::HandleRead(Ptr<Socket> socket)
{
    Ptr<Packet> packet;
    Address from;

    while ((packet = socket->RecvFrom(from)))
    {
        if (packet->GetSize() > 0)
        {
            totalPacketsReceived++;
            ProcessData(packet, from);
        }
    }
}

void
MainGatewayApplication::ProcessData(Ptr<Packet> packet, Address from)
{
    uint8_t buffer[1024];
    packet->CopyData(buffer, packet->GetSize());
    buffer[packet->GetSize()] = '\0';
    std::string data((char*)buffer);

    // Parse: SENSOR:X,ZONE:Y,CO2:Z
    size_t sensorPos = data.find("SENSOR:");
    size_t zonePos = data.find("ZONE:");
    size_t co2Pos = data.find("CO2:");

    if (sensorPos != std::string::npos && zonePos != std::string::npos &&
        co2Pos != std::string::npos)
    {
        uint32_t sensorId = std::stoi(data.substr(sensorPos + 7, zonePos - sensorPos - 8));
        uint32_t zoneId = std::stoi(data.substr(zonePos + 5, co2Pos - zonePos - 6));
        double co2Value = std::stod(data.substr(co2Pos + 4));

        totalCO2BySensor[sensorId] += co2Value;
        packetCountBySensor[sensorId]++;

        InetSocketAddress inetFrom = InetSocketAddress::ConvertFrom(from);
        NS_LOG_INFO("Time " << Simulator::Now().GetSeconds() << "s: Main Gateway received - Sensor "
                            << sensorId << " (Zone " << zoneId << ") CO2: " << co2Value
                            << " ppm [Source: " << inetFrom.GetIpv4() << "]");
    }
}

/*
 * Main Simulation
 */
int
main(int argc, char* argv[])
{
    uint32_t nZones = 5;         // Number of zones (each with 2 sensors and 1 AP)
    uint32_t sensorsPerZone = 2; // Sensors per zone
    double simulationTime = 30.0;
    uint16_t sensorPort = 9000;  // Port for sensor → AP communication
    uint16_t gatewayPort = 9001; // Port for AP → Gateway communication
    bool verbose = true;

    CommandLine cmd;
    cmd.AddValue("nZones", "Number of zones", nZones);
    cmd.AddValue("time", "Simulation time", simulationTime);
    cmd.AddValue("verbose", "Enable logging", verbose);
    cmd.Parse(argc, argv);

    if (verbose)
    {
        LogComponentEnable("HierarchicalCarbonTrading", LOG_LEVEL_INFO);
    }

    uint32_t totalSensors = nZones * sensorsPerZone;

    NS_LOG_INFO("=================================================");
    NS_LOG_INFO("Hierarchical WiFi Carbon Trading Network");
    NS_LOG_INFO("=================================================");
    NS_LOG_INFO("Zones: " << nZones);
    NS_LOG_INFO("Sensors per zone: " << sensorsPerZone);
    NS_LOG_INFO("Total sensors: " << totalSensors);
    NS_LOG_INFO("Total APs: " << nZones);
    NS_LOG_INFO("Simulation time: " << simulationTime << "s");
    NS_LOG_INFO("=================================================");

    // Create nodes
    NodeContainer sensorNodes;
    sensorNodes.Create(totalSensors);

    NodeContainer apNodes;
    apNodes.Create(nZones);

    NodeContainer mainGateway;
    mainGateway.Create(1);

    // WiFi setup for each zone (sensors ↔ local AP)
    InternetStackHelper internet;
    internet.Install(sensorNodes);
    internet.Install(apNodes);
    internet.Install(mainGateway);

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("DsssRate1Mbps"),
                                 "ControlMode",
                                 StringValue("DsssRate1Mbps"));

    Ipv4AddressHelper address;
    MobilityHelper mobility;

    // Setup each zone: 2 sensors + 1 AP
    for (uint32_t zone = 0; zone < nZones; ++zone)
    {
        // Get sensor nodes for this zone
        NodeContainer zoneSensors;
        for (uint32_t s = 0; s < sensorsPerZone; ++s)
        {
            zoneSensors.Add(sensorNodes.Get(zone * sensorsPerZone + s));
        }

        NodeContainer zoneAP;
        zoneAP.Add(apNodes.Get(zone));

        // WiFi for this zone
        WifiMacHelper mac;
        std::ostringstream ssidStr;
        ssidStr << "Zone" << (zone + 1) << "-Net";
        Ssid ssid = Ssid(ssidStr.str());

        // Sensors as stations
        mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));
        NetDeviceContainer zoneSensorDevices = wifi.Install(phy, mac, zoneSensors);

        // AP
        mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
        NetDeviceContainer zoneAPDevice = wifi.Install(phy, mac, zoneAP);

        // IP addressing for zone (10.1.X.0/24)
        std::ostringstream baseAddr;
        baseAddr << "10.1." << (zone + 1) << ".0";
        address.SetBase(baseAddr.str().c_str(), "255.255.255.0");

        Ipv4InterfaceContainer zoneSensorInterfaces = address.Assign(zoneSensorDevices);
        Ipv4InterfaceContainer zoneAPInterface = address.Assign(zoneAPDevice);

        // Position nodes for this zone
        mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                      "MinX",
                                      DoubleValue(zone * 60.0),
                                      "MinY",
                                      DoubleValue(0.0),
                                      "DeltaX",
                                      DoubleValue(20.0),
                                      "DeltaY",
                                      DoubleValue(0.0),
                                      "GridWidth",
                                      UintegerValue(sensorsPerZone),
                                      "LayoutType",
                                      StringValue("RowFirst"));
        mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        mobility.Install(zoneSensors);

        // Position AP above sensors
        Ptr<ListPositionAllocator> apPos = CreateObject<ListPositionAllocator>();
        apPos->Add(Vector(zone * 60.0 + 10.0, 15.0, 0.0));
        mobility.SetPositionAllocator(apPos);
        mobility.Install(zoneAP);

        NS_LOG_INFO("Zone " << (zone + 1) << " configured: " << ssidStr.str() << " at "
                            << baseAddr.str());
    }

    // Main backbone network (APs ↔ Main Gateway) using CSMA (Ethernet)
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));

    NodeContainer backboneNodes;
    backboneNodes.Add(apNodes);
    backboneNodes.Add(mainGateway);

    NetDeviceContainer backboneDevices = csma.Install(backboneNodes);

    address.SetBase("10.2.1.0", "255.255.255.0");
    Ipv4InterfaceContainer backboneInterfaces = address.Assign(backboneDevices);

    Ipv4InterfaceContainer mainAPInterfaces;
    for (uint32_t i = 0; i < nZones; ++i)
    {
        mainAPInterfaces.Add(backboneInterfaces.Get(i));
    }
    Ipv4InterfaceContainer mainGatewayInterface;
    mainGatewayInterface.Add(backboneInterfaces.Get(nZones));

    // Position main gateway at center-top
    Ptr<ListPositionAllocator> gwPos = CreateObject<ListPositionAllocator>();
    gwPos->Add(Vector(120.0, 30.0, 0.0));
    mobility.SetPositionAllocator(gwPos);
    mobility.Install(mainGateway);

    // Use static routing to avoid network confusion with multiple WiFi networks
    Ipv4StaticRoutingHelper staticRouting;

    // Set default routes for sensors to their local AP
    for (uint32_t zone = 0; zone < nZones; ++zone)
    {
        Ptr<Node> ap = apNodes.Get(zone);
        Ipv4Address apAddr = ap->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();

        for (uint32_t s = 0; s < sensorsPerZone; ++s)
        {
            Ptr<Node> sensor = sensorNodes.Get(zone * sensorsPerZone + s);
            Ptr<Ipv4StaticRouting> sensorRouting =
                staticRouting.GetStaticRouting(sensor->GetObject<Ipv4>());
            sensorRouting->SetDefaultRoute(apAddr, 1);
        }
    }

    // Set default routes for APs to main gateway
    Ipv4Address gwAddr = mainGatewayInterface.GetAddress(0);
    for (uint32_t zone = 0; zone < nZones; ++zone)
    {
        Ptr<Node> ap = apNodes.Get(zone);
        Ptr<Ipv4StaticRouting> apRouting = staticRouting.GetStaticRouting(ap->GetObject<Ipv4>());
        apRouting->SetDefaultRoute(gwAddr, 2); // Interface 2 is the CSMA backbone
    }

    // Deploy applications
    Ipv4Address gatewayAddr = mainGatewayInterface.GetAddress(0);

    // Main Gateway
    Ptr<Socket> gwSocket = Socket::CreateSocket(mainGateway.Get(0), UdpSocketFactory::GetTypeId());
    Ptr<MainGatewayApplication> gwApp = CreateObject<MainGatewayApplication>();
    gwApp->Setup(gwSocket, gatewayPort);
    mainGateway.Get(0)->AddApplication(gwApp);
    gwApp->SetStartTime(Seconds(0.0));
    gwApp->SetStopTime(Seconds(simulationTime));

    // Local APs
    for (uint32_t zone = 0; zone < nZones; ++zone)
    {
        Ptr<Socket> apRecvSocket =
            Socket::CreateSocket(apNodes.Get(zone), UdpSocketFactory::GetTypeId());
        Ptr<Socket> apFwdSocket =
            Socket::CreateSocket(apNodes.Get(zone), UdpSocketFactory::GetTypeId());

        Ptr<LocalAPApplication> apApp = CreateObject<LocalAPApplication>();
        Address gwAddress = InetSocketAddress(gatewayAddr, gatewayPort);
        apApp->Setup(apRecvSocket, apFwdSocket, sensorPort, gwAddress, zone + 1);

        apNodes.Get(zone)->AddApplication(apApp);
        apApp->SetStartTime(Seconds(0.0));
        apApp->SetStopTime(Seconds(simulationTime));
    }

    // Sensors
    for (uint32_t i = 0; i < totalSensors; ++i)
    {
        uint32_t zone = i / sensorsPerZone;
        Ptr<Node> apNode = apNodes.Get(zone);
        Ipv4Address apAddr = apNode->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();

        Ptr<Socket> sensorSocket =
            Socket::CreateSocket(sensorNodes.Get(i), UdpSocketFactory::GetTypeId());
        Ptr<CO2SensorApplication> sensorApp = CreateObject<CO2SensorApplication>();

        double baselineCO2 = 400.0 + (i * 50.0);
        Address apAddress = InetSocketAddress(apAddr, sensorPort);

        sensorApp->Setup(sensorSocket, apAddress, sensorPort, i + 1, zone + 1, baselineCO2);
        sensorNodes.Get(i)->AddApplication(sensorApp);
        sensorApp->SetStartTime(Seconds(1.0 + i * 0.2));
        sensorApp->SetStopTime(Seconds(simulationTime));
    }

    // NetAnim visualization
    AnimationInterface anim("hierarchical-carbon-trading.xml");

    // Main Gateway (Blue)
    anim.UpdateNodeDescription(mainGateway.Get(0), "Main_Gateway");
    anim.UpdateNodeColor(mainGateway.Get(0), 0, 0, 255);
    anim.UpdateNodeSize(mainGateway.Get(0)->GetId(), 6.0, 6.0);

    // Local APs (Green)
    for (uint32_t z = 0; z < nZones; ++z)
    {
        std::ostringstream desc;
        desc << "AP_Zone" << (z + 1);
        anim.UpdateNodeDescription(apNodes.Get(z), desc.str());
        anim.UpdateNodeColor(apNodes.Get(z), 0, 200, 0);
        anim.UpdateNodeSize(apNodes.Get(z)->GetId(), 4.0, 4.0);
    }

    // Sensors (Red, different shades per zone)
    for (uint32_t i = 0; i < totalSensors; ++i)
    {
        uint32_t zone = i / sensorsPerZone;
        std::ostringstream desc;
        desc << "Sensor" << (i + 1) << "_Z" << (zone + 1);
        anim.UpdateNodeDescription(sensorNodes.Get(i), desc.str());
        anim.UpdateNodeColor(sensorNodes.Get(i), 255, zone * 40, 0);
        anim.UpdateNodeSize(sensorNodes.Get(i)->GetId(), 2.5, 2.5);
    }

    // Enable tracing
    csma.EnablePcap("hierarchical", backboneDevices.Get(nZones), true);

    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    NS_LOG_INFO("Starting simulation...");
    Simulator::Stop(Seconds(simulationTime));
    Simulator::Run();

    NS_LOG_INFO("=================================================");
    NS_LOG_INFO("Simulation Results");
    NS_LOG_INFO("=================================================");
    NS_LOG_INFO("Total packets sent: " << totalPacketsSent);
    NS_LOG_INFO("Total packets received: " << totalPacketsReceived);
    double ratio =
        (totalPacketsSent > 0) ? (double)totalPacketsReceived / totalPacketsSent * 100.0 : 0.0;
    NS_LOG_INFO("Delivery ratio: " << ratio << "%");
    NS_LOG_INFO("=================================================");

    monitor->SerializeToXmlFile("hierarchical-flowmon.xml", true, true);

    std::cout << "\n=== HIERARCHICAL NETWORK RESULTS ===\n";
    std::cout << "Total sensors: " << totalSensors << "\n";
    std::cout << "Zones: " << nZones << "\n";
    std::cout << "Packets sent: " << totalPacketsSent << "\n";
    std::cout << "Packets received: " << totalPacketsReceived << "\n";
    std::cout << "Delivery ratio: " << ratio << "%\n";
    std::cout << "\nVisualization: hierarchical-carbon-trading.xml\n";
    std::cout << "=====================================\n";

    Simulator::Destroy();
    return 0;
}

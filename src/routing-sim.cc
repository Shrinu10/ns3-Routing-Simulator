/**
 * @file routing-sim.cc
 * @brief NS-3 port of the Network Routing Simulator with NetAnim visualization.
 *
 * Recreates our custom C++ simulator's functionality inside NS-3:
 *   - Configurable router count, topology, link delay, bandwidth
 *   - 4 topology types: random, ring, mesh, tree
 *   - UDP packet flows between source and destination
 *   - NetAnim animation output (animation.xml)
 *   - FlowMonitor statistics with table-format terminal output
 *   - Circle layout for visual clarity
 *
 * Usage:
 *   ./ns3 run "routing-sim --routers=50 --topology=random --packets=100"
 *   ./ns3 run "routing-sim --routers=20 --topology=ring --linkDelay=5ms"
 *   ./ns3 run "routing-sim --routers=100 --randomDelay=true --delayMin=1 --delayMax=10"
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/mobility-module.h"

#include <cmath>
#include <vector>
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <set>
#include <sstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("RoutingSim");

// =====================================================================
//  Topology builders
// =====================================================================

/**
 * Build a ring topology: node i <-> node (i+1) % n.
 * Returns list of (a, b) pairs representing bidirectional links.
 */
static std::vector<std::pair<int, int>>
BuildRing(int n)
{
    std::vector<std::pair<int, int>> links;
    for (int i = 0; i < n; ++i)
    {
        links.push_back({i, (i + 1) % n});
    }
    return links;
}

/**
 * Build a full mesh topology: every pair connected.
 */
static std::vector<std::pair<int, int>>
BuildMesh(int n)
{
    std::vector<std::pair<int, int>> links;
    for (int i = 0; i < n; ++i)
    {
        for (int j = i + 1; j < n; ++j)
        {
            links.push_back({i, j});
        }
    }
    return links;
}

/**
 * Build a binary tree: node i connects to children 2i+1, 2i+2.
 */
static std::vector<std::pair<int, int>>
BuildTree(int n)
{
    std::vector<std::pair<int, int>> links;
    for (int i = 0; i < n; ++i)
    {
        int left = 2 * i + 1;
        int right = 2 * i + 2;
        if (left < n)
            links.push_back({i, left});
        if (right < n)
            links.push_back({i, right});
    }
    return links;
}

/**
 * Build a random connected graph:
 *   Step 1: Random spanning tree (guarantees connectivity)
 *   Step 2: ~N extra random edges (richer connectivity)
 */
static std::vector<std::pair<int, int>>
BuildRandom(int n, uint32_t seed)
{
    std::vector<std::pair<int, int>> links;
    Ptr<UniformRandomVariable> rng = CreateObject<UniformRandomVariable>();
    rng->SetStream(seed);

    // Step 1: Random spanning tree
    std::vector<int> order(n);
    std::iota(order.begin(), order.end(), 0);
    // Shuffle [1, n) using NS-3 RNG
    for (int i = n - 1; i >= 1; --i)
    {
        int j = rng->GetInteger(0, i);
        std::swap(order[i], order[j]);
    }
    for (int i = 1; i < n; ++i)
    {
        int parent = rng->GetInteger(0, i - 1);
        links.push_back({order[i], order[parent]});
    }

    // Step 2: Extra random edges (~N more)
    std::set<std::pair<int, int>> existingLinks;
    for (auto& link : links)
    {
        int a = std::min(link.first, link.second);
        int b = std::max(link.first, link.second);
        existingLinks.insert({a, b});
    }

    int target = n;
    int added = 0;
    for (int attempt = 0; attempt < target * 3 && added < target; ++attempt)
    {
        int a = rng->GetInteger(0, n - 1);
        int b = rng->GetInteger(0, n - 1);
        if (a == b)
            continue;
        int lo = std::min(a, b);
        int hi = std::max(a, b);
        if (existingLinks.count({lo, hi}) == 0)
        {
            existingLinks.insert({lo, hi});
            links.push_back({a, b});
            ++added;
        }
    }

    return links;
}

// =====================================================================
//  Unit helpers — auto-append units if user provides plain numbers
// =====================================================================

/**
 * If the string is purely numeric (e.g. "30"), append defaultUnit (e.g. "Mbps").
 * If it already has letters (e.g. "30Mbps", "10kbps"), leave it unchanged.
 */
static std::string
EnsureUnit(const std::string& value, const std::string& defaultUnit)
{
    // Check if there's at least one letter in the string
    bool hasLetter = false;
    for (char c : value)
    {
        if (std::isalpha(c))
        {
            hasLetter = true;
            break;
        }
    }
    if (!hasLetter && !value.empty())
    {
        return value + defaultUnit;
    }
    return value;
}

// =====================================================================
//  Circle layout for NetAnim
// =====================================================================

static void
ApplyCircleLayout(AnimationInterface& anim, NodeContainer& nodes, double radius)
{
    int n = nodes.GetN();
    double centerX = radius + 20.0;
    double centerY = radius + 20.0;

    for (int i = 0; i < n; ++i)
    {
        double angle = 2.0 * M_PI * i / n;
        double x = centerX + radius * std::cos(angle);
        double y = centerY + radius * std::sin(angle);
        anim.SetConstantPosition(nodes.Get(i), x, y);
    }
}

// =====================================================================
//  Print configuration table
// =====================================================================

static void
PrintConfig(int routers, const std::string& topology, const std::string& dataRate,
            const std::string& linkDelay, int packets, int packetSize,
            int source, int dest, bool randomDelay, double delayMin, double delayMax,
            int numLinks)
{
    std::cout << "\n";
    std::cout << "+--------------------------------------------------+\n";
    std::cout << "|         NS-3 Simulation Configuration            |\n";
    std::cout << "+--------------------------------------------------+\n";
    std::cout << "|  Routers:           " << routers << "\n";
    std::cout << "|  Topology:          " << topology << "\n";
    std::cout << "|  Links:             " << numLinks << "\n";
    std::cout << "|  Data Rate:         " << dataRate << "\n";
    if (randomDelay)
    {
        std::cout << "|  Link Delay:        Random (" << delayMin << "ms - " << delayMax << "ms)\n";
    }
    else
    {
        std::cout << "|  Link Delay:        " << linkDelay << "\n";
    }
    std::cout << "|  Packets:           " << packets << "\n";
    std::cout << "|  Packet Size:       " << packetSize << " bytes\n";
    std::cout << "|  Source:            Node " << source << "\n";
    std::cout << "|  Destination:       Node " << dest << "\n";
    std::cout << "+--------------------------------------------------+\n\n";
}

// =====================================================================
//  Print results table (FlowMonitor stats)
// =====================================================================

static void
PrintFlowStats(Ptr<FlowMonitor> monitor, Ptr<Ipv4FlowClassifier> classifier,
               int totalPacketsSent)
{
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();

    std::cout << "\n+======================================================+\n";
    std::cout << "|            Simulation Results (FlowMonitor)           |\n";
    std::cout << "+======================================================+\n";

    int totalTx = 0, totalRx = 0, totalLost = 0;
    double totalDelay = 0.0;
    int flowsWithRx = 0;
    double minDelay = 1e9, maxDelay = 0.0;

    for (auto& entry : stats)
    {
        Ipv4FlowClassifier::FiveTuple ft = classifier->FindFlow(entry.first);
        FlowMonitor::FlowStats& fs = entry.second;

        totalTx += fs.txPackets;
        totalRx += fs.rxPackets;
        totalLost += fs.lostPackets;

        if (fs.rxPackets > 0)
        {
            double avgDelay = fs.delaySum.GetMilliSeconds() / (double)fs.rxPackets;
            totalDelay += fs.delaySum.GetMilliSeconds();
            flowsWithRx += fs.rxPackets;
            minDelay = std::min(minDelay, avgDelay);
            maxDelay = std::max(maxDelay, avgDelay);
        }

        // Per-flow line
        std::cout << "|  Flow " << entry.first << " ("
                  << ft.sourceAddress << " -> " << ft.destinationAddress << ")\n";
        std::cout << "|    Tx: " << fs.txPackets
                  << "  Rx: " << fs.rxPackets
                  << "  Lost: " << fs.lostPackets << "\n";
        if (fs.rxPackets > 0)
        {
            std::cout << std::fixed << std::setprecision(3);
            std::cout << "|    Avg Delay: "
                      << fs.delaySum.GetMilliSeconds() / (double)fs.rxPackets
                      << " ms\n";
            double throughput = fs.rxBytes * 8.0 /
                                (fs.timeLastRxPacket - fs.timeFirstTxPacket).GetSeconds() / 1e3;
            std::cout << "|    Throughput: " << std::setprecision(1) << throughput << " Kbps\n";
        }
    }

    // Aggregate summary
    std::cout << "+------------------------------------------------------+\n";
    std::cout << "|              Aggregate Summary                       |\n";
    std::cout << "+------------------------------------------------------+\n";
    std::cout << "|  Total Packets Sent:     " << totalTx << "\n";
    std::cout << "|  Total Packets Received: " << totalRx << "\n";
    std::cout << "|  Total Packets Lost:     " << totalLost << "\n";

    double successRate = (totalTx > 0) ? (100.0 * totalRx / totalTx) : 0.0;
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "|  Delivery Rate:          " << successRate << "%\n";

    if (flowsWithRx > 0)
    {
        std::cout << std::fixed << std::setprecision(3);
        std::cout << "|  Avg End-to-End Delay:   " << totalDelay / flowsWithRx << " ms\n";
        std::cout << "|  Min Delay:              " << minDelay << " ms\n";
        std::cout << "|  Max Delay:              " << maxDelay << " ms\n";
    }
    std::cout << "+======================================================+\n\n";
}

// =====================================================================
//  Main
// =====================================================================

int
main(int argc, char* argv[])
{
    // ── Command-line parameters ─────────────────────────────
    int         numRouters  = 20;
    std::string topology    = "random";
    std::string dataRate    = "10Mbps";
    std::string linkDelay   = "5ms";
    int         numPackets  = 100;
    int         packetSize  = 512;
    int         source      = 0;
    int         destination = -1;    // -1 = last node
    bool        randomDelay = false;
    double      delayMin    = 1.0;   // ms
    double      delayMax    = 10.0;  // ms
    uint32_t    seed        = 42;

    CommandLine cmd;
    cmd.AddValue("routers",     "Number of routers",             numRouters);
    cmd.AddValue("topology",    "Topology: random/ring/mesh/tree", topology);
    cmd.AddValue("dataRate",    "Link data rate (e.g. 10Mbps)",  dataRate);
    cmd.AddValue("linkDelay",   "Link delay (e.g. 5ms)",         linkDelay);
    cmd.AddValue("packets",     "Number of UDP packets to send", numPackets);
    cmd.AddValue("packetSize",  "Packet size in bytes",          packetSize);
    cmd.AddValue("source",      "Source node ID",                source);
    cmd.AddValue("destination", "Destination node ID (-1=last)", destination);
    cmd.AddValue("randomDelay", "Use random per-link delay",     randomDelay);
    cmd.AddValue("delayMin",    "Min random delay (ms)",         delayMin);
    cmd.AddValue("delayMax",    "Max random delay (ms)",         delayMax);
    cmd.AddValue("seed",        "RNG seed",                      seed);
    cmd.Parse(argc, argv);

    // Auto-append units if user provided plain numbers
    // e.g. --dataRate=30 becomes "30Mbps", --linkDelay=5 becomes "5ms"
    dataRate  = EnsureUnit(dataRate, "Mbps");
    linkDelay = EnsureUnit(linkDelay, "ms");

    // Apply defaults
    if (destination < 0 || destination >= numRouters)
        destination = numRouters - 1;
    if (source < 0 || source >= numRouters)
        source = 0;
    if (source == destination && numRouters > 1)
        destination = (source + 1) % numRouters;

    // Set global seed for reproducibility
    RngSeedManager::SetSeed(seed);

    std::cout << "========================================\n";
    std::cout << "  Network Routing Simulator (NS-3)\n";
    std::cout << "  NetAnim Visualization Enabled\n";
    std::cout << "========================================\n";

    // ── 1. Create nodes ─────────────────────────────────────
    NodeContainer nodes;
    nodes.Create(numRouters);

    // Install mobility model (suppresses NetAnim warnings)
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    // ── 2. Build topology (link list) ───────────────────────
    std::vector<std::pair<int, int>> links;
    std::string topoUpper = topology;
    std::transform(topoUpper.begin(), topoUpper.end(), topoUpper.begin(), ::tolower);

    if (topoUpper == "ring")
        links = BuildRing(numRouters);
    else if (topoUpper == "mesh")
        links = BuildMesh(numRouters);
    else if (topoUpper == "tree")
        links = BuildTree(numRouters);
    else
        links = BuildRandom(numRouters, seed);

    PrintConfig(numRouters, topology, dataRate, linkDelay, numPackets, packetSize,
                source, destination, randomDelay, delayMin, delayMax,
                (int)links.size());

    // ── 3. Install point-to-point links ─────────────────────
    Ptr<UniformRandomVariable> delayRng = CreateObject<UniformRandomVariable>();
    delayRng->SetAttribute("Min", DoubleValue(delayMin));
    delayRng->SetAttribute("Max", DoubleValue(delayMax));

    InternetStackHelper internet;
    internet.Install(nodes);

    Ipv4AddressHelper addressHelper;
    uint32_t subnetCounter = 1;

    std::cout << "[SETUP] Installing " << links.size() << " point-to-point links...\n";

    for (auto& link : links)
    {
        PointToPointHelper p2p;
        p2p.SetDeviceAttribute("DataRate", StringValue(dataRate));

        if (randomDelay)
        {
            double delay = delayRng->GetValue();
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2) << delay << "ms";
            p2p.SetChannelAttribute("Delay", StringValue(oss.str()));
        }
        else
        {
            p2p.SetChannelAttribute("Delay", StringValue(linkDelay));
        }

        NetDeviceContainer devices = p2p.Install(nodes.Get(link.first),
                                                  nodes.Get(link.second));

        // Assign unique /30 subnet to each link
        std::ostringstream base;
        uint32_t octet3 = (subnetCounter / 64);
        uint32_t octet4 = (subnetCounter % 64) * 4;
        base << "10." << (1 + octet3 / 256) << "." << (octet3 % 256) << "." << octet4;
        addressHelper.SetBase(Ipv4Address(base.str().c_str()), "255.255.255.252");
        addressHelper.Assign(devices);
        ++subnetCounter;
    }

    // ── 4. Populate routing tables ──────────────────────────
    std::cout << "[SETUP] Computing global routing tables...\n";
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // ── 5. Set up UDP applications ──────────────────────────
    uint16_t port = 9;

    // Server (sink) on destination
    UdpEchoServerHelper echoServer(port);
    ApplicationContainer serverApp = echoServer.Install(nodes.Get(destination));
    serverApp.Start(Seconds(0.5));
    serverApp.Stop(Seconds(30.0));

    // Client on source -> sends packets to destination
    // Get the destination's IP address on its first interface
    Ptr<Ipv4> destIpv4 = nodes.Get(destination)->GetObject<Ipv4>();
    Ipv4Address destAddr = destIpv4->GetAddress(1, 0).GetLocal();

    UdpEchoClientHelper echoClient(destAddr, port);
    echoClient.SetAttribute("MaxPackets", UintegerValue(numPackets));
    echoClient.SetAttribute("Interval", TimeValue(MilliSeconds(10)));
    echoClient.SetAttribute("PacketSize", UintegerValue(packetSize));

    ApplicationContainer clientApp = echoClient.Install(nodes.Get(source));
    clientApp.Start(Seconds(1.0));
    clientApp.Stop(Seconds(30.0));

    std::cout << "[SETUP] UDP Echo: Node " << source << " -> Node " << destination
              << " (" << numPackets << " packets, " << packetSize << " bytes each)\n";

    // ── 6. FlowMonitor ──────────────────────────────────────
    FlowMonitorHelper flowHelper;
    Ptr<FlowMonitor> flowMonitor = flowHelper.InstallAll();

    // ── 7. NetAnim setup ────────────────────────────────────
    std::string animFile = "animation.xml";
    AnimationInterface anim(animFile);

    // Circle layout: radius scales with number of routers
    double radius = std::max(50.0, numRouters * 2.0);
    ApplyCircleLayout(anim, nodes, radius);

    // Color source green, destination red, others blue
    for (int i = 0; i < numRouters; ++i)
    {
        if (i == source)
            anim.UpdateNodeColor(nodes.Get(i), 0, 200, 0);       // Green
        else if (i == destination)
            anim.UpdateNodeColor(nodes.Get(i), 220, 50, 50);     // Red
        else
            anim.UpdateNodeColor(nodes.Get(i), 60, 120, 200);    // Blue
    }

    // Node labels
    for (int i = 0; i < numRouters; ++i)
    {
        std::ostringstream label;
        label << "R" << i;
        anim.UpdateNodeDescription(nodes.Get(i), label.str());
    }
    anim.EnablePacketMetadata(true);

    std::cout << "[SETUP] NetAnim output: " << animFile << "\n";
    std::cout << "[SETUP] Layout: Circle (radius=" << radius << ")\n";
    std::cout << "[SETUP] Node colors: Green=Source, Red=Dest, Blue=Routers\n\n";

    // ── 8. Run simulation ───────────────────────────────────
    std::cout << "--- Running Simulation ---\n\n";
    Simulator::Stop(Seconds(30.0));
    Simulator::Run();

    // ── 9. Print results ────────────────────────────────────
    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowHelper.GetClassifier());
    PrintFlowStats(flowMonitor, classifier, numPackets);

    // ── 10. Cleanup ─────────────────────────────────────────
    Simulator::Destroy();

    std::cout << "========================================\n";
    std::cout << "  Simulation completed successfully.\n";
    std::cout << "  Open NetAnim and load: " << animFile << "\n";
    std::cout << "========================================\n";

    return 0;
}

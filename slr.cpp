#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/mobility-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SelectiveRepeatProtocol");

// Custom tag for sequence number
class SeqTag : public Tag
{
public:
  SeqTag () {}
  SeqTag (uint32_t s) : seq (s) {}

  void SetSeq (uint32_t s) { seq = s; }
  uint32_t GetSeq () const { return seq; }

  static TypeId GetTypeId (void)
  {
    static TypeId tid = TypeId ("SeqTag")
      .SetParent<Tag> ()
      .AddConstructor<SeqTag> ();
    return tid;
  }

  TypeId GetInstanceTypeId (void) const override { return GetTypeId (); }
  uint32_t GetSerializedSize (void) const override { return sizeof (seq); }
  void Serialize (TagBuffer i) const override { i.WriteU32 (seq); }
  void Deserialize (TagBuffer i) override { seq = i.ReadU32 (); }
  void Print (std::ostream &os) const override { os << "Seq=" << seq; }

private:
  uint32_t seq;
};

// Configuration
const uint32_t WINDOW_SIZE = 4;
const uint32_t TOTAL_PACKETS = 10;
const double TIMEOUT = 2.0;

uint32_t baseSeq = 0;
uint32_t nextSeq = 0;
std::vector<bool> acked(TOTAL_PACKETS, false);

Ptr<Socket> senderSocket;
Ptr<Socket> receiverSocket;

// Function declarations
void SendPacket (uint32_t seq);
void ScheduleNext ();
void ReceivePacket (Ptr<Socket> socket);
void Timeout (uint32_t seq);

void SendPacket (uint32_t seq)
{
  if (seq >= TOTAL_PACKETS) return;

  uint8_t type = 0; // DATA
  Ptr<Packet> packet = Create<Packet> ((uint8_t*)&type, sizeof(type));

  SeqTag tag(seq);
  packet->AddPacketTag(tag);

  senderSocket->Send (packet);
  NS_LOG_UNCOND ("Sender: Sent DATA Seq=" << seq
                 << " at " << Simulator::Now ().GetSeconds () << "s");

  Simulator::Schedule (Seconds (TIMEOUT), &Timeout, seq);
}

void Timeout (uint32_t seq)
{
  if (!acked[seq]) {
    NS_LOG_UNCOND ("Sender: Timeout for Seq=" << seq
                   << " → Retransmitting at "
                   << Simulator::Now ().GetSeconds () << "s");
    SendPacket (seq);
  }
}

void ScheduleNext ()
{
  while (nextSeq < baseSeq + WINDOW_SIZE && nextSeq < TOTAL_PACKETS) {
    SendPacket (nextSeq);
    nextSeq++;
  }
}

void ReceivePacket (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  while ((packet = socket->Recv ())) {
    uint8_t type;
    packet->CopyData (&type, sizeof(type));

    if (type == 0) {
      SeqTag tag;
      packet->PeekPacketTag(tag);
      uint32_t seq = tag.GetSeq ();

      NS_LOG_UNCOND ("Receiver: Got DATA Seq=" << seq
                     << " at " << Simulator::Now ().GetSeconds () << "s");

      uint8_t ackType = 1;
      Ptr<Packet> ack = Create<Packet> ((uint8_t*)&ackType, sizeof(ackType));
      SeqTag ackTag(seq);
      ack->AddPacketTag(ackTag);
      receiverSocket->Send (ack);

      NS_LOG_UNCOND ("Receiver: Sent ACK for Seq=" << seq
                     << " at " << Simulator::Now ().GetSeconds () << "s");

    } else if (type == 1) {
      SeqTag tag;
      packet->PeekPacketTag(tag);
      uint32_t ackSeq = tag.GetSeq ();

      NS_LOG_UNCOND ("Sender: Got ACK for Seq=" << ackSeq
                     << " at " << Simulator::Now ().GetSeconds () << "s");

      if (ackSeq < TOTAL_PACKETS) {
        acked[ackSeq] = true;

        while (baseSeq < TOTAL_PACKETS && acked[baseSeq]) {
          baseSeq++;
        }

        if (nextSeq < TOTAL_PACKETS)
          ScheduleNext ();
      }
    }
  }
}

int main (int argc, char *argv[])
{
  NodeContainer nodes;
  nodes.Create (2);

  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodes);

  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("2ms"));
  NetDeviceContainer devices = p2p.Install (nodes);

  InternetStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  senderSocket = Socket::CreateSocket (nodes.Get (0), UdpSocketFactory::GetTypeId ());
  senderSocket->Bind (InetSocketAddress (Ipv4Address::GetAny (), 8080));
  senderSocket->Connect (InetSocketAddress (interfaces.GetAddress (1), 8080));
  senderSocket->SetRecvCallback (MakeCallback (&ReceivePacket));

  receiverSocket = Socket::CreateSocket (nodes.Get (1), UdpSocketFactory::GetTypeId ());
  receiverSocket->Bind (InetSocketAddress (Ipv4Address::GetAny (), 8080));
  receiverSocket->Connect (InetSocketAddress (interfaces.GetAddress (0), 8080));
  receiverSocket->SetRecvCallback (MakeCallback (&ReceivePacket));

  Simulator::Schedule (Seconds (1.0), &ScheduleNext);

  AnimationInterface anim ("selectiverepeat.xml");
  anim.SetConstantPosition (nodes.Get (0), 10, 30);
  anim.SetConstantPosition (nodes.Get (1), 50, 30);

  Simulator::Stop (Seconds (30.0));
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}

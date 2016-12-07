/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

// ndn-simple.cpp

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include "apps/ndn-app.hpp"

namespace ns3 {

namespace ndn{

 void
 InterestTrace(shared_ptr< const Interest > interest, Ptr< App > app, shared_ptr< Face > face)
 {
	   std::cout<<"Interest APP ID : "<<app->GetId() <<"\t Node :" <<app->GetNode()<<std::endl;
     std::cout<<"OK : "<< interest->getName()<<std::endl;

 }

 void
 TransmittedTrace (shared_ptr< const Interest > interest, Ptr< App > app, shared_ptr< Face > face)

 {
  std::cout<<"Transmit APP ID : "<<app->GetId() <<"\t Node :" <<app->GetNode()<<std::endl;
  std::cout<<"Interest : "<< interest->getName()<<std::endl;

 }

 void
 DataTrace (shared_ptr< const Data > data, Ptr< App > app, shared_ptr< Face > face)
 {
	std::cout<<"Data APP ID : "<<app->GetId() <<"\t Node :" <<app->GetNode()<<std::endl;
  std::cout<<"DATA : "<<data->getFullName()<<std::endl;
 }
}

/**
 * This scenario simulates a very simple network topology:
 *
 *
 *      +----------+     1Mbps      +--------+     1Mbps      +----------+
 *      | consumer | <------------> | router | <------------> | producer |
 *      +----------+         10ms   +--------+          10ms  +----------+
 *
 *
 * Consumer requests data from producer with frequency 10 interests per second
 * (interests contain constantly increasing sequence number).
 *
 * For every received interest, producer replies with a data packet, containing
 * 1024 bytes of virtual payload.
 *
 * To run scenario and see what is happening, use the following command:
 *
 *     NS_LOG=ndn.Consumer:ndn.Producer ./waf --run=ndn-simple
 */


int
main(int argc, char* argv[])
{
  bool tracing = false;

  // setting default parameters for PointToPoint links and channels
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("1Mbps"));
  Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("10ms"));
  Config::SetDefault("ns3::DropTailQueue::MaxPackets", StringValue("20"));

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.AddValue ("tracing", "Enable pcap tracing", tracing);
  cmd.Parse(argc, argv);

  // Creating nodes
  NodeContainer nodes;
  nodes.Create(3);

  // Connecting nodes using two links
  PointToPointHelper p2p;
  p2p.Install(nodes.Get(0), nodes.Get(1));
  p2p.Install(nodes.Get(1), nodes.Get(2));

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.InstallAll();

  // Choosing forwarding strategy
  ndn::StrategyChoiceHelper::InstallAll("/prefix", "/localhost/nfd/strategy/multicast");

  // Installing applications

  // Consumer
  ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  // Consumer will request /prefix/0, /prefix/1, ...
  consumerHelper.SetPrefix("/prefix");
  consumerHelper.SetAttribute("Frequency", StringValue("10")); // 10 interests a second
  consumerHelper.Install(nodes.Get(0));                        // first node

  // Producer
  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  // Producer will reply to all requests starting with /prefix
  producerHelper.SetPrefix("/prefix");
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  producerHelper.Install(nodes.Get(2)); // last node

  Simulator::Stop(Seconds(20.0));

  if (tracing == true)
    {
      Config::ConnectWithoutContext("/NodeList/2/ApplicationList/*/$ns3::ndn::App/ReceivedInterests", MakeCallback(&ndn::InterestTrace));
//      Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$ns3::ndn::App/ReceivedDatas", MakeCallback(&ndn::DataTrace));
      Config::ConnectWithoutContext("/NodeList/0/ApplicationList/*/$ns3::ndn::App/TransmittedInterests", MakeCallback(&ndn::TransmittedTrace));

    }

  Simulator::Run();
  Simulator::Destroy();

  return 0;
}


} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}
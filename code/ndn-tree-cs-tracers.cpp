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

// ndn-tree-cs-tracers.cpp

#include "ns3/core-module.h"
#include "ns3/network-module.h"
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
 * This scenario simulates a tree topology (using topology reader module)
 *
 *    /------\      /------\      /------\      /------\
 *    |leaf-1|      |leaf-2|      |leaf-3|      |leaf-4|
 *    \------/      \------/      \------/      \------/
 *         ^          ^                ^           ^
 *         |          |                |           |
 *          \        /                  \         /
 *           \      /                    \       /    10Mbps / 1ms
 *            \    /                      \     /
 *             |  |                        |   |
 *             v  v                        v   v
 *          /-------\                    /-------\
 *          | rtr-1 |                    | rtr-2 |
 *          \-------/                    \-------/
 *                ^                        ^
 *                |                        |
 *                 \                      /  10 Mpbs / 1ms
 *                  +--------+  +--------+
 *                           |  |
 *                           v  v
 *                        /--------\
 *                        |  root  |
 *                        \--------/
 *
 *
 * To run scenario and see what is happening, use the following command:
 *
 *     ./waf --run=ndn-tree-cs-tracers
 */

int
main(int argc, char* argv[])
{
  bool tracing = false;

  CommandLine cmd;
  cmd.AddValue ("tracing", "Enable pcap tracing", tracing);
  cmd.Parse(argc, argv);

  AnnotatedTopologyReader topologyReader("", 1);
  topologyReader.SetFileName("src/ndnSIM/examples/topologies/topo-tree.txt");
  topologyReader.Read();

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.SetOldContentStore("ns3::ndn::cs::Lru", "MaxSize",
                               "100"); // default ContentStore parameters
  ndnHelper.InstallAll();

  // Choosing forwarding strategy
  ndn::StrategyChoiceHelper::InstallAll("/prefix", "/localhost/nfd/strategy/best-route");

  // Installing global routing interface on all nodes
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();

  // Getting containers for the consumer/producer
  Ptr<Node> consumers[4] = {Names::Find<Node>("leaf-1"), Names::Find<Node>("leaf-2"),
                            Names::Find<Node>("leaf-3"), Names::Find<Node>("leaf-4")};
  Ptr<Node> producer = Names::Find<Node>("root");

//  for (int i = 0; i < 4; i++) {
//    ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
//    consumerHelper.SetAttribute("Frequency", StringValue("10")); // 100 interests a second
//
//    // Each consumer will express the same data /root/<seq-no>
//    consumerHelper.SetPrefix("/root");
//    ApplicationContainer app = consumerHelper.Install(consumers[i]);
//    app.Start(Seconds(0.01 * i));
//  }

  ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  consumerHelper.SetAttribute("Frequency", StringValue("10")); // 100 interests a second
  consumerHelper.SetPrefix("/root");
  ApplicationContainer app = consumerHelper.Install(consumers[0]);
  app.Start(Seconds(0.01));

  ndn::AppHelper consumerHelper1("ns3::ndn::ConsumerCbr");
  consumerHelper1.SetAttribute("Frequency", StringValue("10")); // 100 interests a second
  consumerHelper1.SetPrefix("/root/rtr-2");
  ApplicationContainer app1 = consumerHelper1.Install(consumers[1]);
  app1.Start(Seconds(0.02));

  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));

  // Register /root prefix with global routing controller and
  // install producer that will satisfy Interests in /root namespace
  ndnGlobalRoutingHelper.AddOrigins("/root", producer);
  producerHelper.SetPrefix("/root");
  producerHelper.Install(producer);

  // Calculate and install FIBs
  ndn::GlobalRoutingHelper::CalculateRoutes();

  Simulator::Stop(Seconds(20.0));

  if (tracing == true)
    {
      Config::ConnectWithoutContext("/NodeList/6/ApplicationList/*/$ns3::ndn::App/ReceivedInterests", MakeCallback(&ndn::InterestTrace));
//       Config::ConnectWithoutContext("/NodeList/3/ApplicationList/*/$ns3::ndn::App/ReceivedDatas", MakeCallback(&ndn::DataTrace));
      Config::ConnectWithoutContext("/NodeList/0/ApplicationList/*/$ns3::ndn::App/TransmittedInterests", MakeCallback(&ndn::TransmittedTrace));
      Config::ConnectWithoutContext("/NodeList/1/ApplicationList/*/$ns3::ndn::App/TransmittedInterests", MakeCallback(&ndn::TransmittedTrace));

    }

  ndn::CsTracer::InstallAll("cs-trace.txt", Seconds(1));

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

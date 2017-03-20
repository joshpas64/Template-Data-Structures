#include <string>
#include <iostream>
#include <fstream>
#include "ics46goody.hpp"
#include "array_queue.hpp"
#include "hash_graph.hpp"
#include "dijkstra.hpp"

//Submitter jpascasc(Pascascio, Joshua)

std::string get_node_in_graph(const ics::DistGraph& g, std::string prompt, bool allow_QUIT) {
  std::string node;
  for(;;) {
    node = ics::prompt_string(prompt + " (must be in graph" + (allow_QUIT ? " or QUIT" : "") + ")");
    if ( (allow_QUIT && node == "QUIT") || g.has_node(node) )
      break;
  }
  return node;
}


int main() {
  try {
      std::ifstream in_graph;
      ics::HashGraph<int> flightGraph;
      ics::safe_open(in_graph,"Enter graph file name: ","flightdist.txt");
      flightGraph.load(in_graph,";");
      in_graph.close();
      std::cout << flightGraph << std::endl;
      std::string response;
      response = get_node_in_graph(flightGraph,"Enter start node ",false);
      ics::HashMap<std::string,ics::Info>CostMap(1,ics::str_hash);
      CostMap = ics::extended_dijkstra(flightGraph,response);
      std::cout << CostMap << std::endl;
      response = get_node_in_graph(flightGraph,"Enter stop node: ",true);
      while(response != "QUIT"){
          ics::ArrayQueue<std::string> path = ics::recover_path(CostMap,response);
          std::cout << "Cost is " << CostMap[response].cost <<"; path is " << path << std::endl;
          response = get_node_in_graph(flightGraph,"Enter stop node: ",true);
      }
  } catch (ics::IcsError& e) {
    std::cout << e.what() << std::endl;
  }
  return 0;
}

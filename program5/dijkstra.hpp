#ifndef DIJKSTRA_HPP_
#define DIJKSTRA_HPP_

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <limits>                    //Biggest int: std::numeric_limits<int>::max()
#include "array_queue.hpp"
#include "array_stack.hpp"
#include "heap_priority_queue.hpp"
#include "hash_graph.hpp"

// Submitter jpascasc(Pascascio, Joshua)
namespace ics {

int str_hash(const std::string& s){std::hash<std::string> hashStr; return hashStr(s);}
class Info {
  public:
    Info() { }

    Info(std::string a_node) : node(a_node) { }

    bool operator==(const Info &rhs) const { return cost == rhs.cost && from == rhs.from; }

    bool operator!=(const Info &rhs) const { return !(*this == rhs); }

    friend std::ostream &operator<<(std::ostream &outs, const Info &i) {
      outs << "Info[" << i.node << "," << i.cost << "," << i.from << "]";
      return outs;
    }

    //Public instance variable definitions
    std::string node = "?";
    int cost = std::numeric_limits<int>::max();
    std::string from = "?";
  };


  bool gt_info(const Info &a, const Info &b) { return a.cost < b.cost; }

  typedef ics::HashGraph<int>                  DistGraph;
  typedef ics::HeapPriorityQueue<Info, gt_info> CostPQ;
  typedef ics::HashMap<std::string, Info>       CostMap;
  typedef ics::pair<std::string, Info>          CostMapEntry;


//Return the final_map as specified in the lecture-note description of
//  extended Dijkstra algorithm
  CostMap extended_dijkstra(const DistGraph &g, std::string start_node) {
        CostMap answerMap(1,str_hash);
        CostMap infoMap(1,str_hash);
        for(const auto& entry : g.all_nodes()){
            infoMap.put(entry.first, Info(entry.first));
        }
        infoMap[start_node].cost = 0;
        infoMap[start_node].from = start_node;
        std::string min_node = start_node;
        Info currentInfo;
        std::string tempNode;
        CostPQ infoPq;
        int min_cost,currentCost,edge_cost;
        for(const CostMapEntry& cE : infoMap)
            infoPq.enqueue(cE.second);
        while(!infoMap.empty()){
            currentInfo = infoPq.dequeue();
            if(currentInfo.cost == std::numeric_limits<int>::max())
                break;
            while(answerMap.has_key(currentInfo.node) && !infoPq.empty())
                currentInfo = infoPq.dequeue();
            min_node = currentInfo.node;
            min_cost = currentInfo.cost;
            answerMap.put(currentInfo.node,currentInfo);
            infoMap.erase(currentInfo.node);
            for(const std::string& entry : g.out_nodes(min_node)){
                if(!answerMap.has_key(entry)){
                    currentCost = infoMap[entry].cost;
                    edge_cost = g.edge_value(min_node,entry) + answerMap[min_node].cost;
                    int min;
                    if(currentCost == std::numeric_limits<int>::max() || edge_cost < currentCost){
                        min = edge_cost;
                        infoMap[entry].cost = min;
                        infoMap[entry].from = min_node;
                        infoPq.enqueue(infoMap[entry]);
                    }
                }
            }
        }
        return answerMap;
  }


//Return a queue whose front is the start node (implicit in answer_map) and whose
//  rear is the end node
  ArrayQueue <std::string> recover_path(const CostMap &answer_map, std::string end_node) {
        ArrayQueue<std::string> returnQueue;
        ArrayStack<std::string> routeStack;
        std::string predecessor;
        predecessor = end_node;
        while(answer_map[predecessor].cost > 0){
            routeStack.push(predecessor);
            predecessor = answer_map[predecessor].from;
        }
       routeStack.push(predecessor);
       returnQueue.enqueue_all(routeStack);
       return returnQueue;
    }
}

#endif /* DIJKSTRA_HPP_ */

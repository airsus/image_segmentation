#ifndef SEG_H
#define SEG_H

#include <opencv2/core/core.hpp>
#include <opencv2/rgbd.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include "CIEDE2000/CIEDE2000.h"
#include <chrono>
namespace seg_helper {
class Timer
{
public:
    Timer() : beg_(clock_::now()) {}
    void reset() { beg_ = clock_::now(); }
    double elapsed() const {
        return std::chrono::duration_cast<second_>
            (clock_::now() - beg_).count(); }
    void out(std::string message = ""){
        double t = elapsed();
        std::cout << message << "  elasped time:" << t << "s" << std::endl;
        reset();
    }
private:
    typedef std::chrono::high_resolution_clock clock_;
    typedef std::chrono::duration<double, std::ratio<1> > second_;
    std::chrono::time_point<clock_> beg_;
};

namespace min_span_tree {
struct Vertex{
    int row;
    int col;
    int id;
};
struct Edge {
    double weight;
    Vertex v1;
    Vertex v2;
    bool operator<(const Edge& rhs) const{
      return weight < rhs.weight;
    }
};
struct DisjointSets {
    std::vector<int> parent, rnk;
    DisjointSets(int n){
        parent.resize(n);
        rnk.resize(n, 0);
        for(int i=0; i<parent.size();i++){
            parent[i] = i;
        }
    }
    int find(int u){
        if(u != parent[u]){
            parent[u] = find(parent[u]);
        }
        return parent[u];
    }
    void merge(int x, int y){
        x = find(x);
        y = find(y);
        if(rnk[x] > rnk[y]){
            parent[y] = x;
        }else{
            parent[x] = y;
        }
        if(rnk[x]==rnk[y]){
            rnk[y]++;
        }
    }
};
struct Graph {
    int V_size;
    std::vector<Edge> edges, mst_edges;
    double mst_cost;
    double kruskalMST(){
        double cost = 0;
        std::sort(edges.begin(), edges.end());
        DisjointSets ds(V_size);
        int remaining = V_size-1;
        for(auto& edge: edges){
            int v_id1 = edge.v1.id;
            int v_id2 = edge.v2.id;
            int v_parent1 = ds.find(v_id1);
            int v_parent2 = ds.find(v_id2);
            if(v_parent1 != v_parent2){
                ds.merge(v_parent1, v_parent2);
                cost += edge.weight;
                mst_edges.push_back(edge);
            }
            if (!--remaining) break;
        }
        if (remaining) return std::numeric_limits<double>::infinity();
        return cost;
    }
    Graph(cv::Mat lab){
        V_size = lab.rows*lab.cols;
        for(int i=1; i<lab.rows-1; i++)  // 1 padding
            for(int j=1; j<lab.cols-1; j++){
                std::vector<Vertex> vs;
                std::vector<cv::Vec3b> colors;
                for(int m=-1; m<=1; m++){
                    for(int n=-1; n<=1; n++){
                        int row = i+m;
                        int col = j+n;
                        Vertex v;
                        v.row = row;
                        v.col = col;
                        v.id = col + row*lab.rows;
                        vs.push_back(v);
                        colors.push_back(lab.at<cv::Vec3b>(row,col));
                    }
                }
                for(int m=0; m<9; m++){
                    if(m!=4){
                        auto v5 = vs[4];
                        auto v_ = vs[m];
                        CIEDE2000::LAB lab1, lab2;
                        auto c5 = colors[4];
                        auto c_ = colors[m];
                        lab1.l = double(c5[0])/255*100;
                        lab1.a = double(c5[1]) - 128;
                        lab1.b = double(c5[2]) - 128;
                        lab2.l = double(c_[0])/255*100;
                        lab2.a = double(c_[1]) - 128;
                        lab2.b = double(c_[2]) - 128;
                        double weight = CIEDE2000::CIEDE2000(lab1, lab2);

                        Edge edge;
                        edge.v1 = v5;
                        edge.v2 = v_;
                        edge.weight = weight;
                        edges.push_back(edge);
                    }
                }
            }
        mst_cost = kruskalMST();
    }
};

}


}

#endif
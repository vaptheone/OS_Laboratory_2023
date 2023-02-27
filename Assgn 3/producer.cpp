#include <bits/stdc++.h>
#include <sys/ipc.h>
#include <stdio.h>
#include <sys/shm.h>
#include <iostream>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fstream>

using namespace std;

typedef struct graphnode
{
    int src, dest;
} graphnode;

#define NUM_CONSUMERS 10
#define MAX_NODES 10000
#define MAX_EDGES 500000

// function to print the graph in "output.txt"
void print_graph(map <int, vector <int>> m)
{
    ofstream out("prod.txt");
    for (auto i = m.begin(); i != m.end(); ++i)
    {
        out << i->first << " : ";
        for (auto j = i->second.begin(); j != i->second.end(); ++j) out << *j << " ";
        out << endl;
    }
    out.close();
}

// function to make the graph
map <int, vector <int>> make_graph(graphnode * edges, int * info)
{
    map <int, vector <int>> graph;
    for (int i = 0; i < info[1]; ++i)
    {
        graph[edges[i].src].push_back(edges[i].dest);
        graph[edges[i].dest].push_back(edges[i].src);
    }
    info[0] = graph.size();
    return graph;
}

// function to add an edge to the graph in the shared memory
void add_edge(graphnode * edges, int u, int v, int *info)
{
    // add the edge to the graph
    graphnode a;
    a.src = u;
    a.dest = v;
    edges[info[1]] = a;
    info[1]++;
}

// function to generate a random number between min and max 
int rand_gen(int min, int max)
{
    return min + (rand() % (max - min + 1));
}

bool compare(const pair<int, vector<int>>& a, const pair<int, vector<int>>& b) {
    return a.second.size() > b.second.size();
}


int main(int argc, char *argv[])
{

    srand(time(NULL));
    printf("/********* Key 1: %s, Key 2: %s *******/\n", argv[1], argv[2]);

    key_t key1 = atoi(argv[1]);
    key_t key2 = atoi(argv[2]);

    int shmid_1 = shmget(key1, sizeof(graphnode) * MAX_NODES, IPC_CREAT | 0666);
    if (shmid_1 == -1)
    {
        perror("error in shmget1 producer");
        exit(EXIT_FAILURE);
    }
    // attach the shared memory
    graphnode * edges = (graphnode *)shmat(shmid_1, NULL, 0);
    if (edges == (graphnode *)-1)
    {
        perror("error in shmat");
        exit(EXIT_FAILURE);
    }

    int shmid_2 = shmget(key2, 2 * sizeof(int), IPC_CREAT | 0666);
    if (shmid_2 == -1)
    {
        perror("error in shmget2 producer");
        exit(EXIT_FAILURE);
    }
    int *info = (int*)shmat(shmid_2, NULL, 0);
    if (info == (int *)-1)
    {
        perror("error in shmat");
        exit(EXIT_FAILURE);
    }

    while(1){
        // make the graph
        map <int, vector <int>> graph = make_graph(edges, info);

        // print the graph
        //print_graph(graph);

        // print info
        cout << "Producer:: Number of nodes: " << info[0] << endl;
        cout << "Producer:: Number of edges: " << info[1] << endl;

        // get top 20 degreed nodes 

        int top[20] = {-1};
        vector<pair<int, vector<int>>> sorted_map(graph.begin(), graph.end());
        sort(sorted_map.begin(), sorted_map.end(), compare);

        int itr=0;
        for (auto it = sorted_map.begin(); it != sorted_map.end() && distance(sorted_map.begin(), it) < 20; ++it) {
            top[itr++] = it ->first;
        }

        // get the cumulative degree array

        vector<int> cum_dgre;
        cum_dgre.push_back(graph[0].size());
        for(int i=1; i < info[0] ; i++){
            cum_dgre.push_back(graph[i].size()+cum_dgre[i-1]);
            cout<<"dgre of "<<i<<":"<< cum_dgre[i] <<"\n";
        }
        

        //generate random number m 
        int m = rand_gen(10,30);
        cout<<"m:"<<m<<endl;
        for(int i=0; i < m ; i++ ){

            // generate random number k 
            int k = rand_gen(1,20);
            cout<<"k:"<<k<<endl;



            for(int j = 0; j < k; j++){
                if(top[j]!=-1){
                    add_edge(edges,info[0],top[j],info);
                }
            }
            info[0] = info[0]+1;

        }

        cout << "Producer:: Number of nodes after updation : " << info[0] << endl;

        // make the graph
        map <int, vector <int>> graph2 = make_graph(edges, info);


        // print the graph
        print_graph(graph2);
        sleep(50);
    }

    shmdt(info);
    shmdt(edges);
    return 0;
}
/*
    CS39002: Operating Systems Lab
    Assignment : 3
    File: main.cpp

    Authors: Group 12
        20CS10041 - Syam Manoj Kumar Paila
        20CS30002 - Anirudh Aitipamula
        20CS30018 - Ganjikunta Vijay Kumar
        20CS30058 - Vineet Amol Pippal
    
    Main file which loads a graph from a file to a shared memory segment and 
    spawns the Producer process and the Consumer processes.
*/
#include <bits/stdc++.h>
#include <sys/ipc.h>
#include <stdio.h>
#include <sys/shm.h>
#include <iostream>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>
#include <string>
#include <fstream>

using namespace std;

typedef struct graphedge
{
    int src, dest;
} graphedge;

#define NUM_CONSUMERS 10
#define MAX_NODES 10000
#define MAX_EDGES 500000

int pid[NUM_CONSUMERS + 1];

// signal handler to handle SIGINT signal (Ctrl+C)
// when main process is terminated by the user on pressing Ctrl+C, 
// all the child processes are terminated and the shared memory segments are eventually deleted
void signal_callback_handler(int signum) 
{
    cout << " Caught signal " << signum << "!" << endl;

    // Terminate child processes`
    for (int i = 0; i <= NUM_CONSUMERS; ++i)
        kill(pid[i], SIGKILL);

    // Move towards program termination
    return;
}

// function to print the graph in "output.txt"
void print_graph(map <int, vector <int>> m)
{
    ofstream out("output.txt");
    for (auto i = m.begin(); i != m.end(); ++i)
    {
        out << i->first << " : ";
        for (auto j = i->second.begin(); j != i->second.end(); ++j) out << *j << " ";
        out << endl;
    }
    out.close();
}

// function to make the graph
map <int, vector <int>> make_graph(graphedge * edges, int * info)
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
void add_edge(graphedge * edges, int u, int v, int *info)
{
    // add the edge to the graph
    graphedge a;
    a.src = u;
    a.dest = v;
    edges[info[1]] = a;
    info[1]++;
}

signed main(int argc, char const *argv[])
{
    // create shared memory segment for edges
    key_t key1 = 2000;
    int shmid_1 = shmget(key1, sizeof(graphedge) * MAX_EDGES, IPC_CREAT | 0666);
    if (shmid_1 == -1)
    {
        perror("Error in creating shared memory segment\n");
        exit(EXIT_FAILURE);
    }
    graphedge *edges = (graphedge *)shmat(shmid_1, NULL, 0);
    if (edges == (graphedge *)-1)
    {
        perror("Error in attaching shared memory segment\n");
        exit(EXIT_FAILURE);
    }

    // initialize the edges
    memset(edges, 0, sizeof(graphedge) * MAX_EDGES);

    // create shared memory segment for info[] = {NUM_NODES, NUM_EDGES} 
    key_t key2 = 2001;
    int shmid_2 = shmget(key2, sizeof(int) * 2, IPC_CREAT | 0666);
    if (shmid_2 == -1)
    {
        perror("Error in creating shared memory segment\n");
        exit(EXIT_FAILURE);
    }
    int *info = (int *)shmat(shmid_2, NULL, 0);
    if (info == (int *)-1)
    {
        perror("Error in attaching shared memory segment\n");
        exit(EXIT_FAILURE);
    }

    // initialize the info
    memset(info, 0, sizeof(int) * 2);

    // open input file
    FILE *fp = fopen("facebook_combined.txt", "r");
    if (fp == NULL)
    {
        perror("Error in opening file\n");
        exit(EXIT_FAILURE);
    }

    // string to read the file
    char str[100];

    // read the file and build the graph
    while ((fgets(str, 100, fp)) != NULL)
    {
        // read two nodes representing an edge
        int u, v;
        sscanf(str, "%d %d", &u, &v);

        // add the edge to the graph
        // increment the edges pointer by 2 to point to the next edge nodes
        add_edge(edges, u, v, info);
    }

    // close the file
    fclose(fp);
    
    // make the graph
    map <int, vector <int>> m = make_graph(edges, info);

    // print the graph
    // print_graph(m);

    // print info
    cout << "Initial number of nodes: " << info[0] << endl;
    cout << "Initial number of edges: " << info[1] << endl; 

    //key to string 
    char key1_str[20], key2_str[20];
    sprintf(key1_str,"%d",key1);
    sprintf(key2_str,"%d",key2);

    // fork for producer
    pid[0] = fork();
    if (pid[0] == 0)
    {
        char *args[] = {(char *)"./producer", key1_str, key2_str, (char *)NULL } ;
        execvp( *args , args ) ;
        exit(EXIT_SUCCESS);
    }
    // fork for consumers
    else
    {
        sleep(2);
        for (int i = 0; i < NUM_CONSUMERS; ++i)
        {
            pid[i + 1] = fork();
            if (pid[i + 1]  == 0)
            {
                // get index of the consumer
                char index[10];
                sprintf(index, "%d", i);

                // if a "-optimize" flag was passed, pass it to the consumer
                if (argc == 2 && strcmp(argv[1], "-optimize") == 0)
                {
                    char *args[] = {(char *)"./consumer", key1_str, key2_str, index, (char *)"1", (char *)NULL } ;
                    execvp(*args , args );
                    exit(EXIT_SUCCESS);
                }
                else 
                {
                    char *args[] = {(char *)"./consumer", key1_str, key2_str, index, (char *)"0", (char *)NULL } ;
                    execvp(*args , args ) ;
                    exit(EXIT_SUCCESS);
                }
            }
        }
    }

    signal(SIGINT, signal_callback_handler);

    // wait for all the processes to finish
    for (int i = 0; i < 11; ++i) waitpid(pid[i], NULL, 0);

    // detach the shared memory
    shmdt(info);
    shmdt(edges);
    shmctl(shmid_1, IPC_RMID, NULL);
    shmctl(shmid_2, IPC_RMID, NULL);

    return 0;
}

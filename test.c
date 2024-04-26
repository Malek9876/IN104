#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <gtk/gtk.h>
#include "osm-gps-map.h"

#define R 6371 // Earth's radius in kilometers

typedef struct Point {
    double lat;
    double lon;
} Point;

typedef struct Edge {
    int dest;
    double weight;
    struct Edge* next;
} Edge;


typedef struct Graph {
    int numVertices;
    Point* vertices;
    Edge** adjLists;
} Graph;

double toRadians(double degree) {
  return degree * M_PI / 180.0;
}

double haversine(double lat1, double lon1, double lat2, double lon2) {
  lat1 = toRadians(lat1);
  lon1 = toRadians(lon1);
  lat2 = toRadians(lat2);
  lon2 = toRadians(lon2);

  double dlon = lon2 - lon1;
  double dlat = lat2 - lat1;

  double a = pow(sin(dlat / 2), 2) + cos(lat1) * cos(lat2) * pow(sin(dlon / 2), 2);
  double c = 2 * atan2(sqrt(a), sqrt(1 - a));

  return R * c;
}
Graph* createGraph(int numVertices) {
    Graph* graph = malloc(sizeof(Graph));
    graph->numVertices = numVertices;
    graph->vertices = malloc(numVertices * sizeof(Point));
    graph->adjLists = malloc(numVertices * sizeof(Edge*));
    for (int i = 0; i < numVertices; i++) {
        graph->adjLists[i] = NULL;
        graph->vertices[i].lat = 0.000000 ;
    }
    return graph;
}
void addEdge(Graph* graph, int src, int dest) {
    double lat1 = graph->vertices[src].lat;
    double lon1 = graph->vertices[src].lon;
    double lat2 = graph->vertices[dest].lat;
    double lon2 = graph->vertices[dest].lon;

    double weight = haversine(lat1,lon1,lat2,lon2);

    Edge* newEdge = malloc(sizeof(Edge));
    newEdge->dest = dest;
    newEdge->weight = weight;
    newEdge->next = graph->adjLists[src];
    graph->adjLists[src] = newEdge;
}
void dijkstra(Graph* graph, int src, int dest,OsmGpsMap *map) {
    int numVertices = graph->numVertices;
    double dist[numVertices];
    int sptSet[numVertices];
    int predecessor[numVertices]; // Array to store predecessor of each node

    for (int i = 0; i < numVertices; i++) {
        dist[i] = INT_MAX;
        sptSet[i] = 0;
        predecessor[i] = -1; // Initialize predecessor array
    }

    dist[src] = 0;

    for (int count = 0; count < numVertices - 1; count++) {
        int u = -1;
        for (int i = 0; i < numVertices; i++) {
            if (!sptSet[i] && (u == -1 || dist[i] < dist[u])) {
                u = i;
            }
        }

        sptSet[u] = 1;

        Edge* pCrawl = graph->adjLists[u];
        while (pCrawl) {
            int v = pCrawl->dest;
            if (!sptSet[v] && dist[u] != INT_MAX && dist[u] + pCrawl->weight < dist[v]) {
                dist[v] = dist[u] + pCrawl->weight;
                predecessor[v] = u; // Update predecessor of v
            }
            pCrawl = pCrawl->next;
        }
    }

    printf("Shortest distance from point A to point B is %f\n", dist[dest]);

    // Reconstruct and print the path
    if (predecessor[dest] != -1) {
        printf("Path: ");
        int current = dest;
        while (current != src) {
            printf("%d  -> ", current);
            current = predecessor[current];
            osm_gps_map_gps_add(map, graph->vertices[current].lat ,graph->vertices[current].lon , g_random_double_range(0, 360));
            
        }
        printf("%d\n", src); // Print the source node
    } else {
        printf("No path exists.\n");
    }
}
// Function to link points and create a track
OsmGpsMapTrack * link_points(double lat1, double long1, double lat2, double long2) {
  OsmGpsMapTrack * track = osm_gps_map_track_new();
  OsmGpsMapPoint * p1, * p2;
  p1 = osm_gps_map_point_new_degrees(lat1, long1);
  p2 = osm_gps_map_point_new_degrees(lat2, long2);
  osm_gps_map_track_add_point(track, p1);
  osm_gps_map_track_add_point(track, p2);
  /// add edge will be here , now build the linked list of all the points

  return track;
}

// Function to check if point is available or not 
int get_index ( Graph* graph , double lat , double lon){
  for (int i = 0; i < graph->numVertices; i += 1)
  {
  	if ( graph->vertices[i].lat == lat && graph->vertices[i].lon == lon ) {
  	  return i ;
  	}
  }
  return -1 ;
}
int get_empty_index ( Graph* graph){
  for (int i = 0; i < graph->numVertices; i += 1)
  {
  	if ( graph->vertices[i].lat == 0.000000 ) {
  	  return i ;
  	}
  }
  return -1 ;


}


// Function to plot points from a file
void file_plot(char * filename, OsmGpsMap * map,Graph* graph) {
  double lat1, lon1, lat2, lon2;
  OsmGpsMapTrack * track;
  char line[100]; // Assuming lines in the file won't exceed 100 characters
  FILE * ptr = fopen(filename, "r");
  if (ptr == NULL) {
    printf("Error: Unable to open file.\n");
    return;
  }
  while (fgets(line, sizeof(line), ptr) != NULL) {
    if (sscanf(line, "%lf %lf %lf %lf %*s %*s", & lat1, & lon1, & lat2, & lon2) == 4) { 
      int indice1, indice2;
      indice1 = get_index(graph , lat1,lon1);
      indice2 = get_index(graph , lat2,lon2);
      if (indice1 == -1)
      {
        int link_index1 = get_empty_index(graph);    
        Point* p1 = malloc( sizeof(Point));
        p1->lat = lat1;
        p1->lon= lon1;
      	graph->vertices[link_index1]= *p1;
      	indice1 = link_index1;
      }
      if (indice2 == -1)
      {   
        int link_index2 = get_empty_index(graph);    
        Point* p2 = malloc( sizeof(Point));
        p2->lat = lat2;
        p2->lon= lon2;
      	graph->vertices[link_index2]= *p2;
      	indice2 = link_index2;
      }
      addEdge(graph,indice1,indice2);
      osm_gps_map_track_add(map, link_points(lat1, lon1, lat2, lon2)); //add track between 2 points
    } else {
      printf("Error: Failed to parse line: %s\n", line);
    }
  }
  fclose(ptr);
}
void show_vertices ( Graph* graph){
	for (int i = 0; i < graph->numVertices; i += 1)
	  {
	  	printf("lat : %f , lon : %f\n",graph->vertices[i].lat,graph->vertices[i].lon);
		}
		}
int main(int argc, char ** argv) {

    Graph* graph = createGraph(300);
    
    
    OsmGpsMap *map;
    GtkWidget *window;

    gtk_init (&argc, &argv);

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (window), "Window");
    g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);


	map = g_object_new (OSM_TYPE_GPS_MAP, NULL);
	gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(map));
    //g_object_set(G_OBJECT(map), "record-trip-history", FALSE, NULL); // No Record history
    
    file_plot("/home/malek/Desktop/TD_INFO/IN104/UCSD-Graphs-master/data/maps/hollywood_small.map",map,graph);
    show_vertices(graph);
    dijkstra(graph, 149, 3,map); // Find shortest path from point A to point B
    osm_gps_map_track_remove_all(map);
	gtk_widget_show (GTK_WIDGET(map));
    gtk_widget_show (window);

    gtk_main ();


    return 0;
}

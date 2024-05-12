/*
 * Copyright (C) Malek Belkahla * Aziz Ben Amira * Elyes Bouaziz 2024 
 *
 * This is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2.
 */
#include <stdio.h>

#include <stdlib.h>

#include <math.h>

#include <glib.h>

#include <gtk/gtk.h>

#include <gdk/gdkkeysyms.h>

#include "osm-gps-map.h"

#include <assert.h>

#define R 6371 // Earth's radius in kilometers

typedef struct Point {

  double lat;

  double lon;

}
Point;

typedef struct Edge {

  int dest;

  double weight;

  struct Edge * next;

}
Edge;


typedef struct Graph {

  int numVertices;

  Point * vertices;

  Edge ** adjLists;

}
Graph;




typedef struct {
  Graph * graph;
  int * list;
  int void_pos;
  int* path;

  // Add other variables you need to pass here
}
CallbackData;


static OsmGpsMapSource_t opt_map_provider = OSM_GPS_MAP_SOURCE_GOOGLE_STREET;double min_distance = DBL_MAX;
static gboolean opt_friendly_cache = TRUE;
static gboolean opt_no_cache = FALSE;
static char * opt_cache_base_dir = NULL;
static gboolean opt_editable_tracks = FALSE;
static GOptionEntry entries[] = {
  {
    "friendly-cache",
    'f',
    0,
    G_OPTION_ARG_NONE,
    &
    opt_friendly_cache,
    "Store maps using friendly cache style (source name)",
    NULL
  },
  {
    "no-cache",
    'n',
    0,
    G_OPTION_ARG_NONE,
    &
    opt_no_cache,
    "Disable cache",
    NULL
  },
  {
    "cache-basedir",
    'b',
    0,
    G_OPTION_ARG_FILENAME,
    &
    opt_cache_base_dir,
    "Cache basedir",
    NULL
  },
  {
    "map",
    'm',
    0,
    G_OPTION_ARG_INT,
    &
    opt_map_provider,
    "Map source",
    "N"
  },
  {
    "editable-tracks",
    'e',
    0,
    G_OPTION_ARG_NONE,
    &
    opt_editable_tracks,
    "Make the tracks editable",
    NULL
  },
  {
    NULL
  }
};
// Function to covert Rad to Degrees 
double toRadians(double degree) {
  return degree * M_PI / 180.0;
}

// Function Haversine to calculate the distance between two points
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
// Function to plot all points
void show_vertices(Graph * graph) {
  for (int i = 0; i < graph -> numVertices; i += 1) {
    printf("lat : %f , lon : %f\n", graph -> vertices[i].lat, graph -> vertices[i].lon);
  }
}

Graph * createGraph(int numVertices) {

  Graph * graph = malloc(sizeof(Graph));

  graph -> numVertices = numVertices;

  graph -> vertices = malloc(numVertices * sizeof(Point));

  graph -> adjLists = malloc(numVertices * sizeof(Edge * ));

  for (int i = 0; i < numVertices; i++) {

    graph -> adjLists[i] = NULL;

    graph -> vertices[i].lat = 0.000000;

  }

  return graph;

}

void addEdge(Graph * graph, int src, int dest) {

  double lat1 = graph -> vertices[src].lat;

  double lon1 = graph -> vertices[src].lon;

  double lat2 = graph -> vertices[dest].lat;

  double lon2 = graph -> vertices[dest].lon;

  double weight = haversine(lat1, lon1, lat2, lon2);

  Edge * newEdge = malloc(sizeof(Edge));

  newEdge -> dest = dest;

  newEdge -> weight = weight;

  newEdge -> next = graph -> adjLists[src];

  graph -> adjLists[src] = newEdge;

}

void dijkstra(Graph * graph, int src, int dest, OsmGpsMap * map,int* Path,double* distance) {

  int numVertices = graph -> numVertices;

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

    Edge * pCrawl = graph -> adjLists[u];

    while (pCrawl) {

      int v = pCrawl -> dest;

      if (!sptSet[v] && dist[u] != INT_MAX && dist[u] + pCrawl -> weight < dist[v]) {

        dist[v] = dist[u] + pCrawl -> weight;

        predecessor[v] = u; // Update predecessor of v

      }

      pCrawl = pCrawl -> next;

    }

  }

  printf("Shortest distance from point A to point B is %f\n", dist[dest]);
  if (distance != NULL ) * distance = dist[dest];
  

  // Reconstruct and print the path

  if (predecessor[dest] != -1) {

    printf("Path: ");

    int current = dest;
	int j = 0;
    while (current != src) {

      printf("%d  -> ", current);
      if (Path != NULL ) Path[j] = current ;
      
      current = predecessor[current];
      j++;

      }
      if (Path != NULL ) Path[j] = current ; // Case of first point which is missing !!
      j++ ;
    printf("%d end\n", src); // Print the source node
     if (Path != NULL ){
    for (int i = j-1 ; i >= 0 ; i--)
    {
    	osm_gps_map_gps_add(map, graph -> vertices[Path[i]].lat, graph -> vertices[Path[i]].lon, g_random_double_range(0, 360));
    	printf("%d  ||",Path[i]);
    }
    }

  } else {

    printf("No path exists.\n");

  }

}

#if!GTK_CHECK_VERSION(3, 22, 0)
// use --gtk-debug=updates instead on newer GTK
static gboolean opt_debug = FALSE;

static GOptionEntry debug_entries[] = {
  {
    "debug",
    'd',
    0,
    G_OPTION_ARG_NONE,
    &
    opt_debug,
    "Enable debugging",
    NULL
  },
  {
    NULL
  }
};
#endif

static GdkPixbuf * g_star_image = NULL;
static OsmGpsMapImage * g_last_image = NULL;

// Function to find the closest available point in map
void find_closest_point(double given_lat, double given_lon,
  const char * points_file, double * lat_result, double * lon_result) {
  FILE * file = fopen(points_file, "r");
  if (file == NULL) {
    printf("Cannot open file %s\n", points_file);
    return;
  }

  double min_distance = INFINITY;
  double closest_lat = 0, closest_lon = 0;

  char line[100];
  while (fgets(line, sizeof(line), file)) {
    double lat1, lon1, lat2, lon2;
    if (sscanf(line, "%lf %lf %lf %lf %*s %*s", & lat1, & lon1, & lat2, & lon2) == 4) {
      double distance1 = haversine(given_lat, given_lon, lat1, lon1);
      double distance2 = haversine(given_lat, given_lon, lat2, lon2);
      if (distance1 < min_distance) {
        min_distance = distance1;
        closest_lat = lat1;
        closest_lon = lon1;
      }
      if (distance2 < min_distance) {
        min_distance = distance2;
        closest_lat = lat2;
        closest_lon = lon2;
      }

    }
  }

  fclose(file);

  * lat_result = closest_lat;
  * lon_result = closest_lon;
}

// Function to link points and create a track
OsmGpsMapTrack * link_points(double lat1, double long1, double lat2, double long2) {
  OsmGpsMapTrack * track = osm_gps_map_track_new();
  OsmGpsMapPoint * p1, * p2;
  p1 = osm_gps_map_point_new_degrees(lat1, long1);
  p2 = osm_gps_map_point_new_degrees(lat2, long2);
  osm_gps_map_track_add_point(track, p1);
  osm_gps_map_track_add_point(track, p2);

  return track;
}

// Function to check if point is available or not 
int get_index(Graph * graph, double lat, double lon) {
  for (int i = 0; i < graph -> numVertices; i += 1) {
    if (graph -> vertices[i].lat == lat && graph -> vertices[i].lon == lon) {
      return i;
    }
  }
  return -1;
}
int get_empty_index(Graph * graph) {
  for (int i = 0; i < graph -> numVertices; i += 1) {
    if (graph -> vertices[i].lat == 0.000000) {
      return i;
    }
  }
  return -1;

}

// Function to plot points from a file
void file_plot(char * filename, OsmGpsMap * map, Graph * graph) {
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
      indice1 = get_index(graph, lat1, lon1);
      indice2 = get_index(graph, lat2, lon2);
      if (indice1 == -1) {
        int link_index1 = get_empty_index(graph);
        Point * p1 = malloc(sizeof(Point));
        p1 -> lat = lat1;
        p1 -> lon = lon1;
        graph -> vertices[link_index1] = * p1;
        indice1 = link_index1;
      }
      if (indice2 == -1) {
        int link_index2 = get_empty_index(graph);
        Point * p2 = malloc(sizeof(Point));
        p2 -> lat = lat2;
        p2 -> lon = lon2;
        graph -> vertices[link_index2] = * p2;
        indice2 = link_index2;
      }
      addEdge(graph, indice1, indice2);
      osm_gps_map_track_add(map, link_points(lat1, lon1, lat2, lon2)); //add track between 2 points
    } else {
      printf("Error: Failed to parse line: %s\n", line);
    }
  }
  fclose(ptr);
}

void free_path(int * Path){
	for (int i  = 0; i < 500; i += 1)
	{
		Path[i] = 0 ;
	}


}

void print_list(int * list,int size){
	for (int i  = 0; i < size; i += 1)
	{
	    printf("\n") ;
		printf(" || %d",list[i])  ;
	}
printf("\n") ;

}
// Function to sort the user given list
void sort_list(Graph* graph ,int* list,OsmGpsMap * map,int size){
	
	double* dist ;
	dist = malloc(sizeof(double));
	int index ;
	for (int i = 0; i < size-2; i += 1)
	{
		double min_distance = DBL_MAX;
		int j ;	
		for (j = i+1; j < size; j += 1)
		{
			dijkstra(graph, list[i], list[j], map ,NULL,dist) ;
			
			
			if ( *dist < min_distance ){
			
				min_distance = *dist;
				index = j ;
			
					}
		}
		printf("%f",min_distance);
		int tmp  = list[i+1];
		list[i+1] = list[index];
		list[index] = tmp ;
	}


free(dist);
}

// Function to manage all the mouse intercations in the map (test phase)
static gboolean on_map_click(GtkWidget * widget, GdkEventButton * event, gpointer user_data) {
  int left_button = (event -> button == 1);
  int right_button = (event -> button == 3) || ((event -> button == 1) && (event -> state & GDK_CONTROL_MASK));
  if (event -> type == GDK_2BUTTON_PRESS && (left_button || right_button)) {
    CallbackData * data = (CallbackData * ) user_data;
    OsmGpsMap * map = OSM_GPS_MAP(widget);
    OsmGpsMapPoint coord;
    Graph * graph = data -> graph;
    int void_pos = data -> void_pos;
    int * list = data -> list;
    float lat, lon;
    int* Path = data -> path;
    double * lat_result;
    double * lon_result;

    lat_result = malloc(sizeof(double));
    lon_result = malloc(sizeof(double));
    osm_gps_map_convert_screen_to_geographic(map, event -> x, event -> y, & coord);
    osm_gps_map_point_get_degrees( & coord, & lat, & lon);

    // Lat and Lon Captured and ready to be used
    // Display the closest coordinates on the map using the function find_closest_point
    g_print("Coordinates: Latitude %f, Longitude %f\n", lat, lon); //print the coordinates for test only
    // if mouse right click clicked 2 times , we process the track and clear all the existing ones 
    if (right_button) {
      // The graph and all the algos will be here      
      osm_gps_map_track_remove_all(map);
      osm_gps_map_gps_clear(map);
      print_list(list,void_pos);
      sort_list(graph ,list,map,void_pos);
      print_list(list,void_pos);
      for (int j = 0; j < void_pos-1; j += 1)
      {
        g_last_image = osm_gps_map_image_add (map,
                                                  graph->vertices[list[j]].lat,
                                                  graph->vertices[list[j]].lon,
                                                  g_star_image);
      	dijkstra(graph, list[j], list[j+1], map,Path,NULL);
      	
      	
      	free_path(Path);
      	
      }
      

    }

    if (left_button) {
      
      
        find_closest_point(lat, lon, "/home/malek/Desktop/TD_INFO/IN104/UCSD-Graphs-master/data/maps/new_york.map", lat_result, lon_result);
        osm_gps_map_gps_add(map, * lat_result, * lon_result, g_random_double_range(0, 360));
        list[void_pos] = get_index(graph, * lat_result, * lon_result);
        data -> void_pos ++;
      
      

    }
    free(lat_result);
    free(lon_result);
  }

  return FALSE; // incase nothing happens , provide the usual functionalities
}
static gboolean
on_map_changed_event(GtkWidget * widget, gpointer user_data) {
  float lat, lon;
  GtkEntry * entry = GTK_ENTRY(user_data);
  OsmGpsMap * map = OSM_GPS_MAP(widget);

  g_object_get(map, "latitude", & lat, "longitude", & lon, NULL);
  gchar * msg = g_strdup_printf("Map Centre: lattitude %f longitude %f", lat, lon);
  gtk_entry_set_text(entry, msg);
  g_free(msg);

  return FALSE;
}

static gboolean
on_zoom_in_clicked_event(GtkWidget * widget, gpointer user_data) {
  int zoom;
  OsmGpsMap * map = OSM_GPS_MAP(user_data);
  g_object_get(map, "zoom", & zoom, NULL);
  osm_gps_map_set_zoom(map, zoom + 1);
  return FALSE;
}

static gboolean
on_zoom_out_clicked_event(GtkWidget * widget, gpointer user_data) {
  int zoom;
  OsmGpsMap * map = OSM_GPS_MAP(user_data);
  g_object_get(map, "zoom", & zoom, NULL);
  osm_gps_map_set_zoom(map, zoom - 1);
  return FALSE;
}

static gboolean
on_home_clicked_event(GtkWidget * widget, gpointer user_data) {
  OsmGpsMap * map = OSM_GPS_MAP(user_data);
  osm_gps_map_set_center_and_zoom(map, 36.837135, 10.140263, 17);
  return FALSE;
}

/*static gboolean
on_cache_clicked_event (GtkWidget *widget, gpointer user_data)
{
    OsmGpsMap *map = OSM_GPS_MAP(user_data);
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) {
        int zoom,max_zoom;
        OsmGpsMapPoint pt1, pt2;
        osm_gps_map_get_bbox(map, &pt1, &pt2);
        g_object_get(map, "zoom", &zoom, "max-zoom", &max_zoom, NULL);
        osm_gps_map_download_maps(map, &pt1, &pt2, zoom, max_zoom);
    } else {
        osm_gps_map_download_cancel_all(map);
    }
    return FALSE;
}*/

static void
on_tiles_queued_changed(OsmGpsMap * image, GParamSpec * pspec, gpointer user_data) {
  gchar * s;
  int tiles;
  GtkLabel * label = GTK_LABEL(user_data);
  g_object_get(image, "tiles-queued", & tiles, NULL);
  s = g_strdup_printf("%d", tiles);
  gtk_label_set_text(label, s);
  g_free(s);
}

static void
on_gps_alpha_changed(GtkAdjustment * adjustment, gpointer user_data) {
  OsmGpsMap * map = OSM_GPS_MAP(user_data);
  OsmGpsMapTrack * track = osm_gps_map_gps_get_track(map);
  float f = gtk_adjustment_get_value(adjustment);
  g_object_set(track, "alpha", f, NULL);
}

static void
on_gps_width_changed(GtkAdjustment * adjustment, gpointer user_data) {
  OsmGpsMap * map = OSM_GPS_MAP(user_data);
  OsmGpsMapTrack * track = osm_gps_map_gps_get_track(map);
  float f = gtk_adjustment_get_value(adjustment);
  g_object_set(track, "line-width", f, NULL);
}

static void
on_star_align_changed(GtkAdjustment * adjustment, gpointer user_data) {
  const char * propname = user_data;
  float f = gtk_adjustment_get_value(adjustment);
  if (g_last_image)
    g_object_set(g_last_image, propname, f, NULL);
}

#if GTK_CHECK_VERSION(3, 4, 0)
static void
on_gps_color_changed(GtkColorChooser * widget, gpointer user_data) {
  GdkRGBA c;
  OsmGpsMapTrack * track = OSM_GPS_MAP_TRACK(user_data);
  gtk_color_chooser_get_rgba(widget, & c);
  osm_gps_map_track_set_color(track, & c);
}
#else
static void
on_gps_color_changed(GtkColorButton * widget, gpointer user_data) {
  GdkRGBA c;
  OsmGpsMapTrack * track = OSM_GPS_MAP_TRACK(user_data);
  gtk_color_button_get_rgba(widget, & c);
  osm_gps_map_track_set_color(track, & c);
}

#endif

static void
on_close(GtkWidget * widget, gpointer user_data) {
  gtk_widget_destroy(widget);
  gtk_main_quit();
}

static void
usage(GOptionContext * context) {
  int i;

  puts(g_option_context_get_help(context, TRUE, NULL));

  printf("Valid map sources:\n");
  for (i = OSM_GPS_MAP_SOURCE_NULL; i <= OSM_GPS_MAP_SOURCE_LAST; i++) {
    const char * name = osm_gps_map_source_get_friendly_name(i);
    const char * uri = osm_gps_map_source_get_repo_uri(i);
    if (uri != NULL)
      printf("\t%d:\t%s\n", i, name);
  }
}

int
main(int argc, char ** argv) {
  GtkBuilder * builder;
  GtkWidget * widget;
  GtkAccelGroup * ag;
  OsmGpsMap * map;
  OsmGpsMapLayer * osd;
  OsmGpsMapTrack * rightclicktrack;
  const char * repo_uri;
  char * cachedir, * cachebasedir;
  GError * error = NULL;
  GOptionContext * context;

  gtk_init( & argc, & argv);

  context = g_option_context_new("- Map browser");
  g_option_context_set_help_enabled(context, FALSE);
  g_option_context_add_main_entries(context, entries, NULL);
  #if!GTK_CHECK_VERSION(3, 22, 0)
  g_option_context_add_main_entries(context, debug_entries, NULL);
  #endif

  if (!g_option_context_parse(context, & argc, & argv, & error)) {
    usage(context);
    return 1;
  }

  /* Only use the repo_uri to check if the user has supplied a
  valid map source ID */
  repo_uri = osm_gps_map_source_get_repo_uri(opt_map_provider);
  if (repo_uri == NULL) {
    usage(context);
    return 2;
  }

  cachebasedir = osm_gps_map_get_default_cache_directory();

  if (opt_cache_base_dir && g_file_test(opt_cache_base_dir, G_FILE_TEST_IS_DIR)) {
    cachedir = g_strdup(OSM_GPS_MAP_CACHE_AUTO);
    cachebasedir = g_strdup(opt_cache_base_dir);
  } else if (opt_friendly_cache) {
    cachedir = g_strdup(OSM_GPS_MAP_CACHE_FRIENDLY);
  } else if (opt_no_cache) {
    cachedir = g_strdup(OSM_GPS_MAP_CACHE_DISABLED);
  } else {
    cachedir = g_strdup(OSM_GPS_MAP_CACHE_AUTO);
  }

  #if!GTK_CHECK_VERSION(3, 22, 0)
  // use --gtk-debug=updates on newer gtk
  if (opt_debug)
    gdk_window_set_debug_updates(TRUE);
  #endif

  g_debug("Map Cache Dir: %s", cachedir);
  g_debug("Map Provider: %s (%d)", osm_gps_map_source_get_friendly_name(opt_map_provider), opt_map_provider);

  map = g_object_new(OSM_TYPE_GPS_MAP,
    "map-source", opt_map_provider,
    "tile-cache", cachedir,
    "tile-cache-base", cachebasedir,
    "proxy-uri", g_getenv("http_proxy"),
    "user-agent", "mapviewer.c", // Always set user-agent, for better tile-usage compliance
    NULL);
  // g_object_set(G_OBJECT(map), "record-trip-history", FALSE, NULL); // No Record history

  osd = g_object_new(OSM_TYPE_GPS_MAP_OSD,
    "show-scale", TRUE,
    "show-coordinates", TRUE,
    "show-crosshair", TRUE,
    "show-dpad", TRUE,
    "show-zoom", TRUE,
    "show-gps-in-dpad", TRUE,
    "show-gps-in-zoom", FALSE,
    "dpad-radius", 30,
    NULL);
  osm_gps_map_layer_add(OSM_GPS_MAP(map), osd);
  g_object_unref(G_OBJECT(osd));

  g_free(cachedir);
  g_free(cachebasedir);

  //Enable keyboard navigation
  osm_gps_map_set_keyboard_shortcut(map, OSM_GPS_MAP_KEY_FULLSCREEN, GDK_KEY_F11);
  osm_gps_map_set_keyboard_shortcut(map, OSM_GPS_MAP_KEY_UP, GDK_KEY_Up);
  osm_gps_map_set_keyboard_shortcut(map, OSM_GPS_MAP_KEY_DOWN, GDK_KEY_Down);
  osm_gps_map_set_keyboard_shortcut(map, OSM_GPS_MAP_KEY_LEFT, GDK_KEY_Left);
  osm_gps_map_set_keyboard_shortcut(map, OSM_GPS_MAP_KEY_RIGHT, GDK_KEY_Right);
  osm_gps_map_set_keyboard_shortcut(map, OSM_GPS_MAP_KEY_ZOOMIN, GDK_KEY_KP_Add);
  osm_gps_map_set_keyboard_shortcut(map, OSM_GPS_MAP_KEY_ZOOMOUT, GDK_KEY_KP_Subtract);
  //Build the UI
  g_star_image = gdk_pixbuf_new_from_file_at_size("poi.png", 24, 24, NULL);

  builder = gtk_builder_new();
  gtk_builder_add_from_file(builder, "mapviewer.ui", & error);
  if (error)
    g_error("ERROR: %s\n", error -> message);

  gtk_box_pack_start(
    GTK_BOX(gtk_builder_get_object(builder, "map_box")),
    GTK_WIDGET(map), TRUE, TRUE, 0);

  //Init values
  float lw, a;
  GdkRGBA c;
  OsmGpsMapTrack * gpstrack = osm_gps_map_gps_get_track(map);
  g_object_get(gpstrack, "line-width", & lw, "alpha", & a, NULL);
  osm_gps_map_track_get_color(gpstrack, & c);
  gtk_adjustment_set_value(
    GTK_ADJUSTMENT(gtk_builder_get_object(builder, "gps_width_adjustment")),
    lw);
  gtk_adjustment_set_value(
    GTK_ADJUSTMENT(gtk_builder_get_object(builder, "gps_alpha_adjustment")),
    a);
  gtk_adjustment_set_value(
    GTK_ADJUSTMENT(gtk_builder_get_object(builder, "star_xalign_adjustment")),
    0.5);
  gtk_adjustment_set_value(
    GTK_ADJUSTMENT(gtk_builder_get_object(builder, "star_yalign_adjustment")),
    0.5);
  #if GTK_CHECK_VERSION(3, 4, 0)
  gtk_color_chooser_set_rgba(
    GTK_COLOR_CHOOSER(gtk_builder_get_object(builder, "gps_colorbutton")), &
    c);
  #else
  gtk_color_button_set_rgba(
    GTK_COLOR_BUTTON(gtk_builder_get_object(builder, "gps_colorbutton")), &
    c);
  #endif

  //Connect to signals
  g_signal_connect(
    gtk_builder_get_object(builder, "gps_colorbutton"), "color-set",
    G_CALLBACK(on_gps_color_changed), (gpointer) gpstrack);
  g_signal_connect(
    gtk_builder_get_object(builder, "zoom_in_button"), "clicked",
    G_CALLBACK(on_zoom_in_clicked_event), (gpointer) map);
  g_signal_connect(
    gtk_builder_get_object(builder, "zoom_out_button"), "clicked",
    G_CALLBACK(on_zoom_out_clicked_event), (gpointer) map);
  g_signal_connect(
    gtk_builder_get_object(builder, "home_button"), "clicked",
    G_CALLBACK(on_home_clicked_event), (gpointer) map);
  /* g_signal_connect (
               gtk_builder_get_object(builder, "cache_button"), "clicked",
               G_CALLBACK (on_cache_clicked_event), (gpointer) map);*/
  g_signal_connect(
    gtk_builder_get_object(builder, "gps_alpha_adjustment"), "value-changed",
    G_CALLBACK(on_gps_alpha_changed), (gpointer) map);
  g_signal_connect(
    gtk_builder_get_object(builder, "gps_width_adjustment"), "value-changed",
    G_CALLBACK(on_gps_width_changed), (gpointer) map);
  g_signal_connect(
    gtk_builder_get_object(builder, "star_xalign_adjustment"), "value-changed",
    G_CALLBACK(on_star_align_changed), (gpointer)
    "x-align");
  g_signal_connect(
    gtk_builder_get_object(builder, "star_yalign_adjustment"), "value-changed",
    G_CALLBACK(on_star_align_changed), (gpointer)
    "y-align");
  Graph * graph = createGraph(1960);
  int list[20];
  int void_pos = 0;
  int Path[500] ;
  
  
  file_plot("/home/malek/Desktop/TD_INFO/IN104/UCSD-Graphs-master/data/maps/new_york.map", map, graph);

  CallbackData * data = g_new(CallbackData, 1);
  data -> graph = graph;
  data -> list = list;
  data -> void_pos = void_pos;
  data -> path = Path;
  g_signal_connect(map, "button-press-event", G_CALLBACK(on_map_click), data);
  g_signal_connect(G_OBJECT(map), "changed",
    G_CALLBACK(on_map_changed_event),
    (gpointer) gtk_builder_get_object(builder, "text_entry"));
  /*g_signal_connect (G_OBJECT (map), "notify::tiles-queued",
              G_CALLBACK (on_tiles_queued_changed),
              (gpointer) gtk_builder_get_object(builder, "cache_label"));*/

  widget = GTK_WIDGET(gtk_builder_get_object(builder, "window1"));
  g_signal_connect(widget, "destroy",
    G_CALLBACK(on_close), (gpointer) map);
  //Setup accelerators.
  ag = gtk_accel_group_new();
  gtk_accel_group_connect(ag, GDK_KEY_w, GDK_CONTROL_MASK, GTK_ACCEL_MASK,
    g_cclosure_new(gtk_main_quit, NULL, NULL));
  gtk_accel_group_connect(ag, GDK_KEY_q, GDK_CONTROL_MASK, GTK_ACCEL_MASK,
    g_cclosure_new(gtk_main_quit, NULL, NULL));
  gtk_window_add_accel_group(GTK_WINDOW(widget), ag);

  gtk_widget_show_all(widget);

  g_log_set_handler("OsmGpsMap", G_LOG_LEVEL_MASK, g_log_default_handler, NULL);
  gtk_main();
  return 0;
}

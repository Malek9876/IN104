#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <gtk/gtk.h>
#include "osm-gps-map.h"


#define R 6371 // Earth's radius in kilometers


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

// Function to find the closest available point in map
void find_closest_point(double given_lat, double given_lon, const char* points_file,double* lat_result,double* lon_result) {
    FILE* file = fopen(points_file, "r");
    if (file == NULL) {
        printf("Cannot open file %s\n", points_file);
        return;
    }

    double min_distance = INFINITY;
    double closest_lat = 0, closest_lon = 0;

    char line[100];
    while (fgets(line, sizeof(line), file)) {
        double lat1, lon1, lat2, lon2;
        if (sscanf(line, "%lf %lf %lf %lf %*s %*s", &lat1, &lon1, &lat2, &lon2) == 4) {
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

    *lat_result = closest_lat;
    *lon_result = closest_lon;
}




// Function to link points and create a track
OsmGpsMapTrack* link_points(double lat1 ,double long1 ,double lat2 ,double long2){
    OsmGpsMapTrack* track = osm_gps_map_track_new();
    OsmGpsMapPoint *p1, *p2;
    p1 = osm_gps_map_point_new_degrees(lat1,long1);
    p2 = osm_gps_map_point_new_degrees(lat2, long2);
    osm_gps_map_track_add_point(track, p1);
    osm_gps_map_track_add_point(track, p2);
    
    return track;
}

// Function to plot points from a file
void file_plot(char * filename, OsmGpsMap *map) {
	double lat1, long1, lat2, long2;
		OsmGpsMapTrack* track ;
		char line[100];  // Assuming lines in the file won't exceed 100 characters
		FILE* ptr = fopen(filename, "r");
		if (ptr == NULL) {
		    printf("Error: Unable to open file.\n");
		    return;
		}
		while (fgets(line, sizeof(line), ptr) != NULL) {
		    if (sscanf(line, "%lf %lf %lf %lf %*s %*s", &lat1, &long1, &lat2, &long2) == 4) {
		        //printf("%f %f %f %f\n", lat1, long1,lat2,long2);  
		        osm_gps_map_track_add(map, link_points(lat1 ,long1 ,lat2 ,long2)); //add track between 2 points
		    } 
		    else {
		        printf("Error: Failed to parse line: %s\n", line);
		    }
		}
		fclose(ptr);
		}

// Callback function for mouse click events
static gboolean on_map_click(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
// double click to remove the tracks ( test phase )
    if (event->type == GDK_2BUTTON_PRESS) {
		OsmGpsMap *map = OSM_GPS_MAP(widget);
		OsmGpsMapPoint coord;
		float lat, lon;
		double* lat_result;
		double* lon_result;
		lat_result = malloc (sizeof(double));
		lon_result = malloc (sizeof(double));
		osm_gps_map_convert_screen_to_geographic(map, event->x, event->y, &coord);
		osm_gps_map_point_get_degrees(&coord, &lat, &lon);
		
		// Lat and Lon Captured and ready to be used
		// Display the closest coordinates on the map
		g_print("Coordinates: Latitude %f, Longitude %f\n", lat, lon);
		osm_gps_map_track_remove_all (map);
		find_closest_point(lat, lon, "/home/malek/Desktop/TD_INFO/IN104/UCSD-Graphs-master/data/maps/Full_Max.map",lat_result,lon_result);
		
		osm_gps_map_gps_add (map,*lat_result,*lon_result,g_random_double_range(0,360));
		
	   }
	   return FALSE; // incase nothing happens , provide the usual functionalities
}




// Callback function for mouse hover events
static gboolean on_map_hover(GtkWidget *widget, GdkEventMotion *event, gpointer user_data) {
    OsmGpsMap *map = OSM_GPS_MAP(widget);
    OsmGpsMapPoint coord;
    float lat, lon;

    osm_gps_map_convert_screen_to_geographic(map, event->x, event->y, &coord);
    osm_gps_map_point_get_degrees(&coord, &lat, &lon);

    // Display the coordinates on the map
    // For simplicity, let's print them to the console
    g_print("Coordinates: Latitude %f, Longitude %f\n", lat, lon);

    return FALSE;
}

int main (int argc, char *argv[]) {
    OsmGpsMap *map;
    GtkWidget *window;

    gtk_init (&argc, &argv);

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (window), "Window");
    g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

    map = g_object_new (OSM_TYPE_GPS_MAP, NULL);
    gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(map));

	// plot all the tracks from the given file
	file_plot("/home/malek/Desktop/TD_INFO/IN104/UCSD-Graphs-master/data/maps/Full_Max.map", map);
    // Connect to mouse click and hover events
    g_signal_connect(map, "button-press-event", G_CALLBACK(on_map_click), NULL);
    g_signal_connect(map, "motion-notify-event", G_CALLBACK(on_map_hover), NULL);

    
    
    gtk_widget_show (GTK_WIDGET(map));
    gtk_widget_show (window);

    gtk_main ();

    

	
    return 0;
}


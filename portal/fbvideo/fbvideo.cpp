
#include <gst/gst.h>  
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <wiringPi.h>

GstElement *textoverlay;
GstElement *inputselector;
GstPad *inputpads[2];

char user_name[10];

unsigned int millis (void){
  struct  timespec ts ;

  clock_gettime (CLOCK_MONOTONIC_RAW, &ts) ;
  uint64_t now  = (uint64_t)ts.tv_sec * (uint64_t)1000 + (uint64_t)(ts.tv_nsec / 1000000L) ;

  return (uint32_t)now ;
}

unsigned int micros (void){
  struct  timespec ts ;

  clock_gettime (CLOCK_MONOTONIC_RAW, &ts) ;
  uint64_t now  = (uint64_t)ts.tv_sec * (uint64_t)1000000 + (uint64_t)(ts.tv_nsec / 1000) ;

  return (uint32_t)now;
}

struct this_gun_struct {
	
	int mode = -1;
	int state_duo = 0; 
	int state_solo = 0; 

	int connected = 0; 
	
	int effect_solo_next = -1;
	int effect_solo_current = -1;

	int effect_duo_next = -1;
	int effect_duo_current = -1;
	
	float latency = 0.0;
	float coretemp = 0.0;
	
	float  battery_level_pretty=0;
	float  temperature_pretty=0;
	int web_packet_counter;
	
} this_gun;  

static gboolean bus_call (GstBus *bus, GstMessage *msg, gpointer data)
{
	GMainLoop *loop = (GMainLoop*)data;

	switch (GST_MESSAGE_TYPE (msg))
	{
	case GST_MESSAGE_EOS:
		g_print ("End-of-stream\n");	
		break;	
	case GST_MESSAGE_ERROR: //normal debug callback
		{
			gchar *debug = NULL;
			GError *err = NULL;

			gst_message_parse_error (msg, &err, &debug);

			g_print ("Error: %s\n", err->message);
			g_error_free (err);

			if (debug)
			{
				g_print ("Debug deails: %s\n", debug);
				g_free (debug);
			}

			g_main_loop_quit (loop);
			break;
		}
	default:
		break;
	}

	return TRUE;
}


static gboolean idle_loop (gpointer data) {
	
	static uint_fast8_t fps = 0;
	static uint_fast32_t fpstimer = 0;
	fps++;
	if (millis()- fpstimer > 1000){
		printf("FDVIDEO FPS: %d\n",fps);
		fpstimer = millis();
		fps = 0;
	}
	
	FILE *webin_fp = fopen("/var/www/html/tmp/fd1.txt", "r");
	
	if (webin_fp){
		

		
		fscanf (webin_fp, "%d %d %d %d %d %d %d %f %f %f %f %d %d\n" ,\
		&this_gun.state_solo,&this_gun.state_duo,&this_gun.connected,\
		&this_gun.effect_solo_next,&this_gun.effect_solo_current,\
		&this_gun.effect_duo_next,&this_gun.effect_duo_current,\
		&this_gun.battery_level_pretty,&this_gun.temperature_pretty,&this_gun.coretemp,\
		&this_gun.latency,&this_gun.web_packet_counter,&this_gun.mode);
		fclose(webin_fp);
		
		
		if(this_gun.mode == 1 ){
			char const * con_good = "Sycned";
			char const * con_bad ="Sync Err";
			char * connection_status = (char *)con_bad; //assume bad
			if (this_gun.connected) connection_status = (char *)con_good;

			uint_fast32_t current_time = millis();
			uint_fast32_t milliseconds = (current_time % 1000);
			uint_fast32_t seconds      = (current_time / 1000) % 60;
			uint_fast32_t minutes      =((current_time / (1000*60)) % 60);
			uint_fast32_t hours        =((current_time / (1000*60*60)) % 24);

			char temp[200];
			
			sprintf(temp,"<b>%s</b>\n%s\n16.3V\n90&#8457;/160&#8457;\n2.2ms/23.4dB\nDuo Mode\n\n%.2d:%.2d:%.2d.%.3d",user_name,connection_status,hours,minutes ,seconds,milliseconds);	
			g_object_set (textoverlay, "text",  temp, NULL);
			g_object_set (inputselector, "active-pad",  inputpads[0], NULL);
		}else{
			g_object_set (textoverlay, "text",  "", NULL);
			g_object_set (inputselector, "active-pad",  inputpads[1], NULL);
		}
		
	}
	
	

	//return true to automatically have this function called again when gstreamer is idle.
	return true;
}

int main(int argc, char *argv[]) {
	
	if(getenv("GORDON")){
		strcpy(user_name, "Gordon");
	}else if(getenv("Chell")){
		strcpy(user_name, "CHELL");
	}else{
		strcpy(user_name, "UNKNOWN");
	}
	
	const char *arg1_gst[]  = {"gstvideo"};
	const char *arg2_gst[]  = {"--gst-disable-registry-update"};
	const char *arg3_gst[]  = {"--gst-debug-level=2"};
	char ** argv_gst[3] = {(char **)arg1_gst,(char **)arg2_gst,(char **)arg3_gst};
	int argc_gst = 3;
	/* Initialize GStreamer */
	gst_init (&argc_gst, argv_gst );

	GMainLoop *loop = g_main_loop_new (NULL, FALSE);

	GstPipeline *pipeline = GST_PIPELINE (gst_parse_launch((char *)"videotestsrc pattern=black ! video/x-raw,width=320,height=240,format=RGB,framerate=10/1 ! queue ! videoin. "
	"videotestsrc ! video/x-raw,width=640,height=480,format=RGB,framerate=10/1 ! queue ! videorate ! "
	"video/x-raw,framrate=10/1 ! videoscale ! video/x-raw,width=320,height=240 ! queue ! videoin. "
	"input-selector name=videoin ! textoverlay vertical-render=true  shaded-background=true valignment=center line-alignment=center halignment=center font-desc=\"DejaVu Sans Mono,48\" name=textinput "
	"! videoflip method=2 ! videoconvert ! fbdevsink device=/dev/fb1", NULL));
	
	GstBus *bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
	gst_bus_add_watch (bus, bus_call, loop);
	gst_object_unref (bus);
	
	//get the output-selector element
	inputselector = gst_bin_get_by_name (GST_BIN (pipeline), "videoin");
	//save each output pad for later
	inputpads[0] = gst_element_get_static_pad(inputselector,"sink_0");
	inputpads[1] = gst_element_get_static_pad(inputselector,"sink_1");
	
	textoverlay = gst_bin_get_by_name (GST_BIN (pipeline), "textinput");
	
	gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_PLAYING);	

	g_timeout_add (23,idle_loop ,pipeline); 
	g_main_loop_run (loop); //let gstreamer's GLib event loop take over
	
	return 0;
}

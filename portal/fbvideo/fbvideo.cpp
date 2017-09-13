
#include <gst/gst.h>  
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <wiringPi.h>

GstElement *textoverlay;
GstElement *inputselector;
GstPad *inputpads[2];

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
};  


struct this_gun_struct this_gun;

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
	
	static uint16_t fps = 0;
	static uint32_t timer = 0;
	fps++;
	if (millis()- timer > 1000){
		printf("FDVIDEO FPS: %d\n",fps);
		timer = millis();
		fps = 0;
	}
	
	
	static uint32_t timer2 = 0;
	if (millis()- timer2 > 20){
		
		FILE *webin_fp;
	
		webin_fp = fopen("/var/www/html/tmp/fd1.txt", "r");
		fscanf (webin_fp, "%d %d %d %d %d %d %d %f %f %f %f %d %d\n" ,\
		&this_gun.state_solo,&this_gun.state_duo,&this_gun.connected,\
		&this_gun.effect_solo_next,&this_gun.effect_solo_current,\
		&this_gun.effect_duo_next,&this_gun.effect_duo_current,\
		&this_gun.battery_level_pretty,&this_gun.temperature_pretty,&this_gun.coretemp,\
		&this_gun.latency,&this_gun.web_packet_counter,&this_gun.mode);
		fclose(webin_fp);
	
	
		uint32_t current_time= millis();
		uint32_t milliseconds =  (current_time % 1000) ;
		uint32_t seconds =  (current_time / 1000) % 60 ;
		uint32_t minutes =  ((current_time / (1000*60)) % 60);
		uint32_t hours   =  ((current_time / (1000*60*60)) % 24);

		char temp[100];
		sprintf(temp,"<b>Gordon</b>     &#8646;\nVolts:  16.3\nTemp 90/160F\nLag:     2ms\nMode:    Duo\nUptime\n%.2d:%.2d:%.2d.%.3d",hours,minutes ,seconds,milliseconds);
		g_object_set (textoverlay, "text",  temp, NULL);
		
		
				if(this_gun.mode == 1 ){
			g_object_set (inputselector, "active-pad",  inputpads[1], NULL);
		}else{
			g_object_set (inputselector, "active-pad",  inputpads[0], NULL);
		}

		
		timer2 = millis();
	}
	
	//return true to automatically have this function called again when gstreamer is idle.
	return true;
}

int main(int argc, char *argv[]) {
		
	
	const char *arg1_gst[]  = {"gstvideo"};
	const char *arg2_gst[]  = {"--gst-disable-registry-update"};
	const char *arg3_gst[]  = {"--gst-debug-level=2"};
	char ** argv_gst[3] = {(char **)arg1_gst,(char **)arg2_gst,(char **)arg3_gst};
	int argc_gst = 3;
	/* Initialize GStreamer */
	gst_init (&argc_gst, argv_gst );

	GMainLoop *loop = g_main_loop_new (NULL, FALSE);

	GstPipeline *pipeline=GST_PIPELINE (gst_parse_launch((char *)"videotestsrc pattern=smpte ! video/x-raw,width=640,height=480,framerate=10/1 ! queue ! videorate ! "
	"video/x-raw,framrate=10/1 ! videoscale ! video/x-raw,width=320,height=240 ! videoin. "
	"videotestsrc pattern=snow ! video/x-raw,width=320,height=240,framerate=10/1 ! queue ! videoin. "
	"input-selector name=videoin ! textoverlay vertical-render=true  shaded-background=true valignment=center line-alignment=center halignment=center font-desc=\"DejaVu Sans Mono,40\" name=textinput "
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
	
	gpointer data = NULL;
	gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_PLAYING);	

	g_timeout_add (25,idle_loop ,data);
	g_main_loop_run (loop); //let gstreamer's GLib event loop take over
	
	
	return 0;
}

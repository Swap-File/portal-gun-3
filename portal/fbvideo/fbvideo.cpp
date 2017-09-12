
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

int hits = 0;
int xoffset = 0;
int dir = 0;

int  flopper = 0;
static gboolean idle_loop (gpointer data) {
	
	hits++;
	
	static uint32_t timer3 = 0;
	if (millis()- timer3 > 10000){
		if(flopper == 0 ){
			g_object_set (inputselector, "active-pad",  inputpads[1], NULL);
			flopper = 1;
		}else{
			g_object_set (inputselector, "active-pad",  inputpads[0], NULL);
			flopper = 0;
		}
		timer3 = millis();		
	}
	
	
	static uint32_t timer = 0;
	if (millis()- timer > 1000){
		printf("Idle 5sec %d\n",hits);
		timer = millis();
		hits = 0;
	}
	
	static uint32_t timer2 = 0;
	if (millis()- timer2 > 20){
		
		if (dir == 0)  xoffset++; 
		else           xoffset--;
		
		if (xoffset > 50) dir=1;
		if (xoffset <=0 ) dir=0;
		
		g_object_set (textoverlay, "deltax",  -xoffset, NULL);
		uint32_t current_time= millis();
		uint32_t milliseconds =  (current_time % 1000) ;
		uint32_t seconds =  (current_time / 1000) % 60 ;
		uint32_t minutes =  ((current_time / (1000*60)) % 60);
		uint32_t hours   =  ((current_time / (1000*60*60)) % 24);

		char temp[100];
		sprintf(temp,"<b>Gordon</b>  &#8646; Volts: 16.3\nTemp 90/160F\nLag: 2ms\nMode: Duo\nUptime\n%.2d:%.2d:%.2d.%.3d",hours,minutes ,seconds,milliseconds);
		g_object_set (textoverlay, "text",  temp, NULL);
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
	"input-selector name=videoin ! textoverlay shaded-background=true valignment=center line-alignment=center halignment=center font-desc=Sans,40 name=textinput vertical-render=true "
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

	g_timeout_add (20,idle_loop ,data);
	g_main_loop_run (loop); //let gstreamer's GLib event loop take over
	
	
	return 0;
}

#define GST_USE_UNSTABLE_API  //if you're not out of control you're not in control

#include "gstvideo.h"

#include <gst/gst.h>
#include <gst/gl/gl.h>
#include <gst/gl/x11/gstgldisplay_x11.h>

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <X11/Xlib.h>

#include <GL/gl.h>
#include <GL/glx.h>

#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <iostream>
#include <string>

#include "model_board/model_board.h"
int parentpid = 0;
int input_command_pipe;
Display *dpy;
Window win;
GLXContext ctx;

GstPad *outputpads[6];	
GstElement *outputselector;
GstPipeline *pipeline[75],*pipeline_active;

GLuint normal_texture;
volatile GLuint gst_shared_texture;

GMainLoop *loop;

#ifndef GLX_MESA_swap_control
#define GLX_MESA_swap_control 1
typedef int (*PFNGLXGETSWAPINTERVALMESAPROC)(void);
#endif

/* return current time (in seconds) */
static double current_time(void) {
	struct timeval tv;
	struct timezone tz;
	(void) gettimeofday(&tv, &tz);
	return (double) tv.tv_sec + tv.tv_usec / 1000000.0;
}

/* new window size or exposure */
static void reshape(int screen_width, int screen_height)
{
	float nearp = 1, farp = 500.0f, hht, hwd;

	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	glViewport(0, 0, (GLsizei)screen_width, (GLsizei)screen_height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	hht = nearp * tan(45.0 / 2.0 / 180.0 * M_PI);
	hwd = hht * screen_width / screen_height;

	glFrustum(-hwd, hwd, -hht, hht, nearp, farp);
	
}

GstContext *x11context;
GstContext *ctxcontext;

static gboolean bus_call (GstBus *bus, GstMessage *msg, gpointer data)
{
	GMainLoop *loop = (GMainLoop*)data;

	switch (GST_MESSAGE_TYPE (msg))
	{
	case GST_MESSAGE_EOS:
		g_print ("End-of-stream\n");
		if (parentpid != 0){
			kill(parentpid,SIGUSR2);
		}
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
	case GST_MESSAGE_NEED_CONTEXT:  //THIS IS THE IMPORTANT PART
		{
			const gchar *context_type;
			gst_message_parse_context_type (msg, &context_type);
			if (g_strcmp0 (context_type, "gst.gl.app_context") == 0) 
			{
				g_print("OpenGL Context Request Intercepted! %s\n", context_type);	
				gst_element_set_context (GST_ELEMENT (msg->src), ctxcontext);  			
			}
			if (g_strcmp0 (context_type, GST_GL_DISPLAY_CONTEXT_TYPE) == 0) 
			{
				g_print("X11 Display Request Intercepted! %s\n", context_type);			
				gst_element_set_context (GST_ELEMENT (msg->src), x11context);			
			}

			break;
		}
	case GST_MESSAGE_HAVE_CONTEXT:
		{
			g_print("This should never happen! Don't let the elements set their own context!\n");
			
		}
	default:
		break;
	}

	return TRUE;
}


/*
* Create an RGB, double-buffered window.
* Return the window and context handles.
*/
static void make_window( Display *dpy, const char *name,int x, int y, int width, int height,Window *winRet, GLXContext *ctxRet, VisualID *visRet) {
	int attribs[64];
	int i = 0;

	int scrnum;
	XSetWindowAttributes attr;
	unsigned long mask;
	Window root;
	Window win;

	XVisualInfo *visinfo;

	/* Singleton attributes. */
	attribs[i++] = GLX_RGBA;
	attribs[i++] = GLX_DOUBLEBUFFER;

	/* Key/value attributes. */
	attribs[i++] = GLX_RED_SIZE;
	attribs[i++] = 1;
	attribs[i++] = GLX_GREEN_SIZE;
	attribs[i++] = 1;
	attribs[i++] = GLX_BLUE_SIZE;
	attribs[i++] = 1;
	attribs[i++] = GLX_DEPTH_SIZE;
	attribs[i++] = 1;

	attribs[i++] = None;

	scrnum = DefaultScreen( dpy );
	root = RootWindow( dpy, scrnum );

	visinfo = glXChooseVisual(dpy, scrnum, attribs);
	if (!visinfo) {
		printf("Error: couldn't get an RGB, Double-buffered");
		exit(1);
	}

	/* window attributes */
	attr.background_pixel = 0;
	attr.border_pixel = 0;
	attr.colormap = XCreateColormap( dpy, root, visinfo->visual, AllocNone);
	attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
	/* XXX this is a bad way to get a borderless window! */
	mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

	win = XCreateWindow( dpy, root, x, y, width, height,0, visinfo->depth, InputOutput,	visinfo->visual, mask, &attr );

	/* set hints and properties */
	{
		XSizeHints sizehints;
		sizehints.x = x;
		sizehints.y = y;
		sizehints.width  = width;
		sizehints.height = height;
		sizehints.flags = USSize | USPosition;
		XSetNormalHints(dpy, win, &sizehints);
		XSetStandardProperties(dpy, win, name, name,
		None, (char **)NULL, 0, &sizehints);
	}

	ctx = glXCreateContext( dpy, visinfo, NULL, True );
	if (!ctx) {
		printf("Error: glXCreateContext failed\n");
		exit(1);
	}

	*winRet = win;
	*ctxRet = ctx;
	*visRet = visinfo->visualid;

	XFree(visinfo);
}


/**
* Determine whether or not a GLX extension is supported.
*/
static int is_glx_extension_supported(Display *dpy, const char *query)
{
	const int scrnum = DefaultScreen(dpy);
	const char *glx_extensions = NULL;
	const size_t len = strlen(query);
	const char *ptr;

	if (glx_extensions == NULL) {
		glx_extensions = glXQueryExtensionsString(dpy, scrnum);
	}

	ptr = strstr(glx_extensions, query);
	return ((ptr != NULL) && ((ptr[len] == ' ') || (ptr[len] == '\0')));
}


/**
* Attempt to determine whether or not the display is synched to vblank.
*/
static void query_vsync(Display *dpy, GLXDrawable drawable)
{
	int interval = 0;

#if defined(GLX_EXT_swap_control)
	if (is_glx_extension_supported(dpy, "GLX_EXT_swap_control")) {
		unsigned int tmp = -1;
		glXQueryDrawable(dpy, drawable, GLX_SWAP_INTERVAL_EXT, &tmp);
		interval = tmp;
	} else
#endif
	if (is_glx_extension_supported(dpy, "GLX_MESA_swap_control")) {
		PFNGLXGETSWAPINTERVALMESAPROC pglXGetSwapIntervalMESA =
		(PFNGLXGETSWAPINTERVALMESAPROC)
		glXGetProcAddressARB((const GLubyte *) "glXGetSwapIntervalMESA");

		interval = (*pglXGetSwapIntervalMESA)();
	} else if (is_glx_extension_supported(dpy, "GLX_SGI_swap_control")) {
		/* The default swap interval with this extension is 1.  Assume that it
	* is set to the default.
	*
	* Many Mesa-based drivers default to 0, but all of these drivers also
	* export GLX_MESA_swap_control.  In that case, this branch will never
	* be taken, and the correct result should be reported.
	*/
		interval = 1;
	}


	if (interval > 0) {
		printf("Running synchronized to the vertical refresh.  The framerate should be\n");
		if (interval == 1) {
			printf("approximately the same as the monitor refresh rate.\n");
		} else if (interval > 1) {
			printf("approximately 1/%d the monitor refresh rate.\n",
			interval);
		}
	}
}

//grabs the texture via the glfilterapp element
gboolean drawCallback (GstElement* object, guint id , guint width ,guint height, gpointer user_data){

	static GTimeVal current_time;
	static glong last_sec = current_time.tv_sec;
	static gint nbFrames = 0;
	
	//printf("Texture #:%d  X:%d  Y:%d!\n",id,width,height);
	//snapshot(normal_texture);
	
	gst_shared_texture = id;
	g_get_current_time (&current_time);
	nbFrames++;

	if ((current_time.tv_sec - last_sec) >= 1)
	{
		std::cout << "GSTREAMER FPS = " << nbFrames << std::endl;
		nbFrames = 0;
		last_sec = current_time.tv_sec;
	}
	
	return true;  //not sure why?
}

void load_pipeline(int i, char * text){
	
	printf("Loading pipeline %d\n",i);
	
	pipeline[i] = GST_PIPELINE (gst_parse_launch(text, NULL));

	//set the bus watcher for error handling and to pass the x11 display and opengl context when the elements request it
	//must be BEFORE setting the client-draw callback
	GstBus *bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline[i]));
	gst_bus_add_watch (bus, bus_call, loop);
	gst_object_unref (bus);
	
	//set the glfilterapp callback that will capture the textures
	//do this AFTER attaching the bus handler so context can be set
	if (i != GST_RPICAMSRC){  //the camera bus has no texture output
		GstElement *grabtexture = gst_bin_get_by_name (GST_BIN (pipeline[i]), "grabtexture");
		g_signal_connect (grabtexture, "client-draw",  G_CALLBACK (drawCallback), NULL);
		gst_object_unref (grabtexture);	
	}
}


static int video_mode_requested = 0;
static int video_mode_current = -1;
static int portal_mode_requested = 9;

void load_video_pipe(void){
	load_pipeline(GST_MOVIE_FIRST ,(char *) "filesrc name=movieplayer ! qtdemux name=dmux ! queue ! avdec_h264 ! queue ! "
	"glupload ! glcolorconvert ! glcolorscale ! video/x-raw(memory:GLMemory),width=640,height=480 ! "
	"glfilterapp name=grabtexture ! fakesink sync=true "
	"dmux. ! queue ! aacparse !  avdec_aac ! audioconvert ! alsasink");
}

void start_pipeline(){
	
	double start_time = current_time();
	
	//stop the old pipeline
	if (GST_IS_ELEMENT(pipeline_active)) {
		
		//if we are leaving a audio effect mode 
		if (video_mode_current >= GST_LIBVISUAL_FIRST && video_mode_current <= GST_LIBVISUAL_LAST){
			//only pause  if we are going to a non audio mode from a audio mode, otherwise switch it live
			if (portal_mode_requested < GST_LIBVISUAL_FIRST && portal_mode_requested > GST_LIBVISUAL_LAST){
				gst_element_set_state (GST_ELEMENT (pipeline_active), GST_STATE_PAUSED);
			}
		}else{	
			//null everything else		
			if(video_mode_current== GST_RPICAMSRC ){
				gst_element_set_state (GST_ELEMENT (pipeline_active), GST_STATE_PAUSED);	
			}else{
				gst_element_set_state (GST_ELEMENT (pipeline_active), GST_STATE_NULL);
			}
		}
		
		//if its a video stream, unload it completely
		if (video_mode_current >= GST_MOVIE_FIRST && video_mode_current <= GST_MOVIE_LAST){
			gst_object_unref (GST_ELEMENT (pipeline_active));  
		}
	}
	
	//prep the new pipeline
	if (video_mode_requested >= GST_MOVIE_FIRST && video_mode_requested <= GST_MOVIE_LAST){
		
		//dirty hack to reload pipeline, fixes qtdemux crash
		load_video_pipe();
		
		pipeline_active = pipeline[GST_MOVIE_FIRST]; //all share the same pipeline

		//set the file name
		GstElement *movieplayer = gst_bin_get_by_name (GST_BIN (pipeline_active), "movieplayer");
		char filename[100];
		sprintf(filename,"/home/pi/assets/movies/%d.mp4",video_mode_requested - GST_MOVIE_FIRST + 1);		
		g_object_set (movieplayer, "location",  filename, NULL);
		gst_object_unref (movieplayer);	
	}
	else if (video_mode_requested >= GST_LIBVISUAL_FIRST && video_mode_requested <= GST_LIBVISUAL_LAST){
		g_object_set (outputselector, "active-pad",  outputpads[video_mode_requested - GST_LIBVISUAL_FIRST], NULL);
		pipeline_active = pipeline[GST_LIBVISUAL_FIRST]; 
	}else{
		pipeline_active = pipeline[video_mode_requested]; 
	}
	
	//if we dont se the callbacks here, the bus request handler can do it
	//but explicitly setting it seems to be required when swapping pipelines
	gst_element_set_context (GST_ELEMENT (pipeline_active), ctxcontext);  			
	gst_element_set_context (GST_ELEMENT (pipeline_active), x11context);		
	
	//start the show
	gst_element_set_state (GST_ELEMENT (pipeline_active), GST_STATE_PLAYING);	
	
	video_mode_current = video_mode_requested;
	
	printf("Pipeline %d changed to in %f seconds!\n",video_mode_current,current_time() - start_time);
}


static gboolean idle_loop (gpointer data) {
	
	static int accleration[3];
	
	//opengl rendering update
	static int frames = 0;
	static double tRate0 = -1.0;
	double t = current_time();

	int count = 1;
	char buffer[100];
	//stdin is line buffered so we can cheat a little bit
	while (count > 0){ // dump entire buffer
		count = read(input_command_pipe, buffer, sizeof(buffer)-1);
		if (count > 1){ //ignore blank lines
			buffer[count-1] = '\0';
			//keep most recent line
			int temp[5];
			int result = sscanf(buffer,"%d %d %d %d %d", &temp[0],&temp[1],&temp[2],&temp[3],&temp[4]);
			if (result != 5){
				fprintf(stderr, "Unrecognized input with %d items.\n", result);
			}else{
				portal_mode_requested = temp[0];
				video_mode_requested  = temp[1];
				accleration[0]        = temp[2];
				accleration[1]        = temp[3];
				accleration[2]        = temp[4];
			}
		}
	}
	
	model_board_animate(accleration,portal_mode_requested);
	model_board_redraw(gst_shared_texture,portal_mode_requested);
	
	glXSwapBuffers(dpy, win);
	
	frames++;

	if (tRate0 < 0.0)
	tRate0 = t;
	if (t - tRate0 >= 5.0) {
		GLfloat seconds = t - tRate0;
		GLfloat fps = frames / seconds;
		printf("Rendering %d frames in %3.1f seconds = %6.3f FPS\n", frames, seconds,fps);
		fflush(stdout);
		tRate0 = t;
		frames = 0;
	}
	
	if (video_mode_requested != video_mode_current) start_pipeline();
	
	//return true to automatically have this function called again when gstreamer is idle.
	return true;
}


int main(int argc, char *argv[]){
	
	if (argc < 2){
		printf("GSTVIDEO: No Parent Pid Supplied, EoS signaling disabled!");
	}else{
		parentpid = atoi(argv[1]);
	}
	printf("GSTVIDEO: Using PID %d!\n",parentpid);
	
	mkfifo ("/home/pi/GSTVIDEO_IN_PIPE", 0777 );
	system("chown pi /home/pi/GSTVIDEO_IN_PIPE");
	
	//OPEN PIPE WITH READ ONLY
	if ((input_command_pipe = open ("/home/pi/GSTVIDEO_IN_PIPE",  ( O_RDONLY | O_NONBLOCK ) ))<0){
		perror("GSTVIDEO: Could not open named pipe for reading.");
		exit(-1);
	}
	printf("GSTVIDEO: /home/pi/GSTVIDEO_IN_PIPE has been opened.\n");

	fcntl(input_command_pipe, F_SETFL, fcntl(input_command_pipe, F_GETFL, 0) | O_NONBLOCK);
	
	/* Initialize X11 */
	unsigned int winWidth = 720, winHeight = 480;
	int x = 0, y = 0;

	char *dpyName = NULL;
	GLboolean printInfo = GL_FALSE;
	VisualID visId;
	
	dpy = XOpenDisplay(dpyName);
	if (!dpy) {
		printf("Error: couldn't open display %s\n",
		dpyName ? dpyName : getenv("DISPLAY"));
		return -1;
	}

	make_window(dpy, "gstvideo", x, y, winWidth, winHeight, &win, &ctx, &visId);
	XMapWindow(dpy, win);
	glXMakeCurrent(dpy, win, ctx);
	query_vsync(dpy, win);

	/* Inspect the texture */
	//snapshot(normal_texture);
	
	if (printInfo) {
		printf("GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
		printf("GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
		printf("GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
		printf("GL_EXTENSIONS = %s\n", (char *) glGetString(GL_EXTENSIONS));
		printf("VisualID %d, 0x%x\n", (int) visId, (int) visId);
	}

	/* Set initial projection/viewing transformation.
	* We can't be sure we'll get a ConfigureNotify event when the window
	* first appears.
	*/
	reshape(winWidth, winHeight);
	
	/* Initialize GStreamer */
	const char *arg1_gst[]  = {"gstvideo"}; 
	const char *arg2_gst[]  = {"--gst-disable-registry-update"};  //dont rescan the registry to load faster.
	const char *arg3_gst[]  = {"--gst-debug-level=1"};  //dont show debug messages
	char ** argv_gst[3] = {(char **)arg1_gst,(char **)arg2_gst,(char **)arg3_gst};
	int argc_gst = 3;
	gst_init (&argc_gst, argv_gst );
	
	//get x11context ready for handing off to elements in the callback
	GstGLDisplay * gl_display = GST_GL_DISPLAY (gst_gl_display_x11_new_with_display (dpy));//dpy is the glx OpenGL display			
	x11context = gst_context_new (GST_GL_DISPLAY_CONTEXT_TYPE, TRUE);
	gst_context_set_gl_display (x11context, gl_display);
	
	//get ctxcontext ready for handing off to elements in the callback
	GstGLContext *gl_context = gst_gl_context_new_wrapped ( gl_display, (guintptr) ctx,GST_GL_PLATFORM_GLX,GST_GL_API_OPENGL); //ctx is the glx OpenGL context
	ctxcontext = gst_context_new ("gst.gl.app_context", TRUE);
	gst_structure_set (gst_context_writable_structure (ctxcontext), "context", GST_GL_TYPE_CONTEXT, gl_context, NULL);
	
	//preload all pipelines we will be switching between.  This allows faster switching than destroying and recrearting the pipelines
	//Also, if too many pipelines get destroyed and recreated I have noticed gstreamer or x11 will eventually crash with context errors
	//this can switch between pipelines in 5-20ms on a Pi3.
	
	//test patterns
	load_pipeline(GST_BLANK ,(char *)"videotestsrc pattern=2 ! video/x-raw,width=640,height=480,framerate=(fraction)30/1 ! queue ! glupload ! video/x-raw(memory:GLMemory),width=640,height=480,format=RGBA ! glfilterapp name=grabtexture ! fakesink sync=true");
	load_pipeline(GST_VIDEOTESTSRC ,(char *)"videotestsrc ! video/x-raw,width=640,height=480,framerate=(fraction)30/1 ! queue ! glupload ! video/x-raw(memory:GLMemory),width=640,height=480,format=RGBA ! glfilterapp name=grabtexture ! fakesink sync=true");
	load_pipeline(GST_VIDEOTESTSRC_CUBED ,(char *)"videotestsrc ! video/x-raw,width=640,height=480,framerate=(fraction)30/1 ! queue ! glupload ! glfiltercube ! video/x-raw(memory:GLMemory),width=640,height=480,format=RGBA ! glfilterapp name=grabtexture ! fakesink sync=true");

	//camera launch 192.168.1.22 gordon    192.168.1.23 chell
	if(getenv("GORDON")){
	load_pipeline(GST_RPICAMSRC ,(char *)"rpicamsrc preview=0 ! image/jpeg,width=640,height=480,framerate=30/1 ! "
	"queue max-size-time=50000000 leaky=upstream ! jpegparse ! tee name=t "
	"t. ! queue ! rtpjpegpay ! udpsink host=192.168.1.23 port=9000 sync=false "
	"t. ! queue ! rtpjpegpay ! udpsink host=127.0.0.1     port=8999 sync=false "
	"t. ! queue ! multifilesink location=/var/www/html/tmp/snapshot.jpg");
	}else if(getenv("CHELL")){
	load_pipeline(GST_RPICAMSRC ,(char *)"rpicamsrc preview=0 ! image/jpeg,width=640,height=480,framerate=30/1 ! "
	"queue max-size-time=50000000 leaky=upstream ! jpegparse ! tee name=t "
	"t. ! queue ! rtpjpegpay ! udpsink host=192.168.1.22 port=9000 sync=false "
	"t. ! queue ! rtpjpegpay ! udpsink host=127.0.0.1     port=8999 sync=false "
	"t. ! queue ! multifilesink location=/var/www/html/tmp/snapshot.jpg");
	}
	else {
		printf("SET THE GORDON OR CHELL ENVIRONMENT VARIABLE!");
		exit(1);
	}

	//normal
	load_pipeline(GST_NORMAL ,(char *)"udpsrc port=9000 caps=application/x-rtp retrieve-sender-address=false ! rtpjpegdepay ! jpegdec ! queue ! glupload ! glcolorconvert ! video/x-raw(memory:GLMemory),width=640,height=480,format=RGBA ! glfilterapp name=grabtexture ! fakesink sync=false");
	
	//cpu effects
	load_pipeline(GST_RADIOACTV    ,(char *)"udpsrc port=9000 caps=application/x-rtp retrieve-sender-address=false ! rtpjpegdepay ! jpegdec ! queue ! videoconvert ! queue ! radioactv     ! glupload ! glcolorconvert ! video/x-raw(memory:GLMemory),width=640,height=480,format=RGBA ! glfilterapp name=grabtexture ! fakesink sync=false");
	load_pipeline(GST_REVTV        ,(char *)"udpsrc port=9000 caps=application/x-rtp retrieve-sender-address=false ! rtpjpegdepay ! jpegdec ! queue ! videoconvert ! queue ! revtv         ! glupload ! glcolorconvert ! video/x-raw(memory:GLMemory),width=640,height=480,format=RGBA ! glfilterapp name=grabtexture ! fakesink sync=false");
	load_pipeline(GST_AGINGTV      ,(char *)"udpsrc port=9000 caps=application/x-rtp retrieve-sender-address=false ! rtpjpegdepay ! jpegdec ! queue ! videoconvert ! queue ! agingtv       ! glupload ! glcolorconvert ! video/x-raw(memory:GLMemory),width=640,height=480,format=RGBA ! glfilterapp name=grabtexture ! fakesink sync=false");
	load_pipeline(GST_DICETV       ,(char *)"udpsrc port=9000 caps=application/x-rtp retrieve-sender-address=false ! rtpjpegdepay ! jpegdec ! queue ! videoconvert ! queue ! dicetv        ! glupload ! glcolorconvert ! video/x-raw(memory:GLMemory),width=640,height=480,format=RGBA ! glfilterapp name=grabtexture ! fakesink sync=false");
	load_pipeline(GST_WARPTV       ,(char *)"udpsrc port=9000 caps=application/x-rtp retrieve-sender-address=false ! rtpjpegdepay ! jpegdec ! queue ! videoconvert ! queue ! warptv        ! glupload ! glcolorconvert ! video/x-raw(memory:GLMemory),width=640,height=480,format=RGBA ! glfilterapp name=grabtexture ! fakesink sync=false");
	load_pipeline(GST_SHAGADELICTV ,(char *)"udpsrc port=9000 caps=application/x-rtp retrieve-sender-address=false ! rtpjpegdepay ! jpegdec ! queue ! videoconvert ! queue ! shagadelictv  ! glupload ! glcolorconvert ! video/x-raw(memory:GLMemory),width=640,height=480,format=RGBA ! glfilterapp name=grabtexture ! fakesink sync=false");
	load_pipeline(GST_VERTIGOTV    ,(char *)"udpsrc port=9000 caps=application/x-rtp retrieve-sender-address=false ! rtpjpegdepay ! jpegdec ! queue ! videoconvert ! queue ! vertigotv     ! glupload ! glcolorconvert ! video/x-raw(memory:GLMemory),width=640,height=480,format=RGBA ! glfilterapp name=grabtexture ! fakesink sync=false");
	load_pipeline(GST_RIPPLETV     ,(char *)"udpsrc port=9000 caps=application/x-rtp retrieve-sender-address=false ! rtpjpegdepay ! jpegdec ! queue ! videoconvert ! queue ! rippletv      ! glupload ! glcolorconvert ! video/x-raw(memory:GLMemory),width=640,height=480,format=RGBA ! glfilterapp name=grabtexture ! fakesink sync=false");
	load_pipeline(GST_EDGETV       ,(char *)"udpsrc port=9000 caps=application/x-rtp retrieve-sender-address=false ! rtpjpegdepay ! jpegdec ! queue ! videoconvert ! queue ! edgetv        ! glupload ! glcolorconvert ! video/x-raw(memory:GLMemory),width=640,height=480,format=RGBA ! glfilterapp name=grabtexture ! fakesink sync=false");
	load_pipeline(GST_STREAKTV     ,(char *)"udpsrc port=9000 caps=application/x-rtp retrieve-sender-address=false ! rtpjpegdepay ! jpegdec ! queue ! videoconvert ! queue ! streaktv      ! glupload ! glcolorconvert ! video/x-raw(memory:GLMemory),width=640,height=480,format=RGBA ! glfilterapp name=grabtexture ! fakesink sync=false");
	
	//gl effects - jpegdec then queue, or queue then jpegdec?  TEST IT
	load_pipeline(GST_GLCUBE   ,(char *)"udpsrc port=9000 caps=application/x-rtp retrieve-sender-address=false ! rtpjpegdepay ! jpegdec ! queue ! glupload ! glcolorconvert ! glfiltercube      ! video/x-raw(memory:GLMemory),width=640,height=480,format=RGBA ! glfilterapp name=grabtexture ! fakesink sync=false");
	load_pipeline(GST_GLMIRROR ,(char *)"udpsrc port=9000 caps=application/x-rtp retrieve-sender-address=false ! rtpjpegdepay ! queue ! jpegdec ! glupload ! glcolorconvert ! gleffects_mirror  ! video/x-raw(memory:GLMemory),width=640,height=480,format=RGBA ! glfilterapp name=grabtexture ! fakesink sync=false");
	load_pipeline(GST_GLSQUEEZE,(char *)"udpsrc port=9000 caps=application/x-rtp retrieve-sender-address=false ! rtpjpegdepay ! queue ! jpegdec ! glupload ! glcolorconvert ! gleffects_squeeze ! video/x-raw(memory:GLMemory),width=640,height=480,format=RGBA ! glfilterapp name=grabtexture ! fakesink sync=false");
	load_pipeline(GST_GLSTRETCH,(char *)"udpsrc port=9000 caps=application/x-rtp retrieve-sender-address=false ! rtpjpegdepay ! queue ! jpegdec ! glupload ! glcolorconvert ! gleffects_stretch ! video/x-raw(memory:GLMemory),width=640,height=480,format=RGBA ! glfilterapp name=grabtexture ! fakesink sync=false");
	load_pipeline(GST_GLTUNNEL ,(char *)"udpsrc port=9000 caps=application/x-rtp retrieve-sender-address=false ! rtpjpegdepay ! queue ! jpegdec ! glupload ! glcolorconvert ! gleffects_tunnel  ! video/x-raw(memory:GLMemory),width=640,height=480,format=RGBA ! glfilterapp name=grabtexture ! fakesink sync=false");
	load_pipeline(GST_GLTWIRL  ,(char *)"udpsrc port=9000 caps=application/x-rtp retrieve-sender-address=false ! rtpjpegdepay ! queue ! jpegdec ! glupload ! glcolorconvert ! gleffects_twirl   ! video/x-raw(memory:GLMemory),width=640,height=480,format=RGBA ! glfilterapp name=grabtexture ! fakesink sync=false");
	load_pipeline(GST_GLBULGE  ,(char *)"udpsrc port=9000 caps=application/x-rtp retrieve-sender-address=false ! rtpjpegdepay ! queue ! jpegdec ! glupload ! glcolorconvert ! gleffects_bulge   ! video/x-raw(memory:GLMemory),width=640,height=480,format=RGBA ! glfilterapp name=grabtexture ! fakesink sync=false");
	load_pipeline(GST_GLHEAT   ,(char *)"udpsrc port=9000 caps=application/x-rtp retrieve-sender-address=false ! rtpjpegdepay ! queue ! jpegdec ! glupload ! glcolorconvert ! gleffects_heat    ! video/x-raw(memory:GLMemory),width=640,height=480,format=RGBA ! glfilterapp name=grabtexture ! fakesink sync=false");
	
	//audio effects - alsasrc takes a second or so to init, so here a output-selector is used
	//effect order matters since pads on the output selector can't easily be named in advance 
	load_pipeline(GST_LIBVISUAL_FIRST,(char *)"alsasrc buffer-time=10000 ! queue max-size-time=10000000 leaky=downstream ! "
	"output-selector name=audioin pad-negotiation-mode=Active "
	"audioin. ! libvisual_jess     ! videosink. "
	"audioin. ! libvisual_infinite ! videosink. "
	"audioin. ! libvisual_jakdaw   ! videosink. "
	"audioin. ! libvisual_oinksie  ! videosink. "
	"audioin. ! goom               ! videosink. "
	"audioin. ! goom2k1            ! videosink. "
	"funnel name=videosink ! video/x-raw,width=320,height=240,framerate=30/1 ! queue ! "
	"glupload ! glcolorconvert ! glcolorscale ! video/x-raw(memory:GLMemory),width=640,height=480 ! "
	"glfilterapp name=grabtexture ! fakesink sync=false async=false");
	
	//save the output pads from the visualization pipelines
	
	//get the output-selector element
	outputselector = gst_bin_get_by_name (GST_BIN (pipeline[GST_LIBVISUAL_FIRST]), "audioin");
	
	//save each output pad for later
	outputpads[0] = gst_element_get_static_pad(outputselector,"src_0");
	outputpads[1] = gst_element_get_static_pad(outputselector,"src_1");
	outputpads[2] = gst_element_get_static_pad(outputselector,"src_2");
	outputpads[3] = gst_element_get_static_pad(outputselector,"src_3");
	outputpads[4] = gst_element_get_static_pad(outputselector,"src_4");
	outputpads[5] = gst_element_get_static_pad(outputselector,"src_5");

	//videos -  all share the same pipeline, location property set at load
	load_video_pipe(); //look in this function for the pipeline!
	
	model_board_init();
	
	//start the idle and main loops
	loop = g_main_loop_new (NULL, FALSE);
	gpointer data = NULL;
	g_idle_add (idle_loop, data);
	g_main_loop_run (loop); //let gstreamer's GLib event loop take over
	
	glXMakeCurrent(dpy, None, NULL);
	glXDestroyContext(dpy, ctx);
	XDestroyWindow(dpy, win);
	XCloseDisplay(dpy);
	
	return 0;
}
#define GST_USE_UNSTABLE_API

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <unistd.h>
#include <stdexcept>


#include <gst/gst.h>
#include <gst/gl/gl.h>


#include "png_texture.h"

#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))

int last_acceleration[2] = {0,0};
static GLuint orange_1,blue_n,orange_n,orange_0,blue_0,blue_1,texture_orange,texture_blue,circle64;
float portal_spin = 0;
float event_horizon_spin = 0;

float event_horizon_vertex_list_shimmer = 0;
bool event_horizon_vertex_list_shimmer_direction = true;

float event_horizon_transparency_level = 0;

float global_zoom = 0;

int lastcolor = -1;
GLfloat donut_texture_scrolling = 0.0;


float offset_thing[360];
float running_magnitude;
float angle_target;
float angle_target_delayed;

/* Borrowed from glut, adapted */

static void draw_torus(GLfloat r, GLfloat R, GLint nsides, GLint rings){
	int i, j;
	GLfloat r_using, R_using;
	GLfloat theta, phi, theta1;
	GLfloat cosTheta, sinTheta;
	GLfloat cosTheta1, sinTheta1;
	GLfloat ringDelta, sideDelta;

	ringDelta = 2.0 * M_PI / rings;  //x and y 
	sideDelta = 2.0 * M_PI / nsides; //z

	theta = 0.0;
	cosTheta = 1.0;
	sinTheta = 0.0;
	for (i = rings - 1; i >= 0; i--) {
		theta1 = theta + ringDelta; //from 0 to 6.28
		
		cosTheta1 = cos(theta1);
		sinTheta1 = sin(theta1);

		glBegin(GL_QUAD_STRIP); 

		phi = 0.0;
		
		int index_deg = (360 - portal_spin + ( theta1 * 360  / (2*M_PI)));
		
		while (index_deg >= 360) index_deg -=360;
		
		r_using = r+ offset_thing[index_deg];
		R_using = R- offset_thing[index_deg];
		
		for (j = nsides; j >= 0; j--) {
			GLfloat s0, s1, t;
			GLfloat cosPhi, sinPhi, dist;

			phi += sideDelta;
			
			cosPhi = cos(phi);
			cosPhi = cos(phi);
			
			sinPhi = sin(phi);
			dist = R_using + r_using * cosPhi;
			
			s0 = 20.0 * theta / (2.0 * M_PI);
			s1 = 20.0 * theta1 / (2.0 * M_PI);
			t = 2.0 * phi / (2.0 * M_PI);  //this seems to control texture wrap around the nut

			//glNormal3f(cosTheta1 * cosPhi, -sinTheta1 * cosPhi, sinPhi);
			glTexCoord2f( s0, t - donut_texture_scrolling);
			glVertex3f( cosTheta1 * dist, -sinTheta1 * dist, r_using * sinPhi);

			//glNormal3f(cosTheta * cosPhi, -sinTheta * cosPhi, sinPhi);
			glTexCoord2f(s1, t - donut_texture_scrolling);
			glVertex3f(cosTheta * dist, -sinTheta * dist,  r_using * sinPhi);
		}

		glEnd();

		theta = theta1;
		cosTheta = cosTheta1;
		sinTheta = sinTheta1;
	}
}


void model_board_animate(int acceleration[], int frame){	

	static int running_acceleration[2] = {0,0};
	
	for (int i = 0; i <  360; i ++){
		offset_thing[i] *= .9;
	}
	
	int relative_acceleration[2];

	float alpha = .5;
	
	//filter acceleration
	for (int i = 0; i < 2; i++){
		
		relative_acceleration[i] = (last_acceleration[i] - acceleration[i]);

		//decay values
		//running_acceleration[i] *= .95;
		
		//ignore small disturbances
		if (abs(relative_acceleration[i] ) < 2 && abs(running_acceleration[i]) < 2  ){
			relative_acceleration[i] = 0;
		}
		
		//if ((relative_acceleration[i] >= 0 && running_acceleration[i] >= 0 )|| (relative_acceleration[0] <= 0 && running_acceleration[i]<= 0 )){
		//	running_acceleration[i] = running_acceleration[i] * alpha + (1-alpha)*relative_acceleration[i];
		//}
		running_acceleration[i] = running_acceleration[i] * alpha + (1-alpha)*relative_acceleration[i];
	}

	//scale magnitude to 0 to 1
	running_magnitude *= .95;
	float temp_mag = sqrt( running_acceleration[0]  * running_acceleration[0]  + running_acceleration[1] * running_acceleration[1]);
	if (temp_mag > running_magnitude){
		running_magnitude = running_magnitude * .5 + .5 *temp_mag;
		
	}
	
	angle_target =  atan2(-running_acceleration[1],-running_acceleration[0] ) ;

	angle_target_delayed = angle_target_delayed * .5 + angle_target * .5;
	
	//printf( "%f %f\n", running_magnitude,angle_target);
	
	//spins the portal rim (uncontrolled)
	portal_spin -= .50;
	if (portal_spin < 0) portal_spin += 360;	
	
	//slowly spins the portal background (uncontrolled)
	event_horizon_spin -= .01;
	if (event_horizon_spin < 360) event_horizon_spin += 360;

	//slowly shimmers the portal background (uncontrolled)
	if (event_horizon_vertex_list_shimmer > .99) event_horizon_vertex_list_shimmer_direction = false;
	if (event_horizon_vertex_list_shimmer < .25) event_horizon_vertex_list_shimmer_direction = true;
	if (event_horizon_vertex_list_shimmer_direction)	event_horizon_vertex_list_shimmer += .01;
	else												event_horizon_vertex_list_shimmer -= .01;
	

	if CHECK_BIT(frame,1){
		if (lastcolor != 1){
			global_zoom = 0.0;
			event_horizon_transparency_level = 1.0; //close portal for a moment on rapid color change
			lastcolor = 1;
		} 
	}else{
		if (lastcolor != 2){
			global_zoom = 0.0;
			event_horizon_transparency_level = 1.0;  //close portal for a moment on rapid color change
			lastcolor = 2;
		} 
	}

	//this controls global zoom (0 is blanked)
	if CHECK_BIT(frame,3){
		//turn on display
		if (global_zoom == 0.0){
			global_zoom = 0.01; //bump global_zoom from it's safe spot
		}
	}else{
		//turn off display
		global_zoom = 0.0;
		lastcolor = -1;
	}

	//let blank fader fall to normal size
	if( global_zoom  > 0 && global_zoom < 1.0){
		global_zoom += 0.05 ;
		if ( global_zoom > 1.0 ) global_zoom = 1.0;
	}
	
	//this controls making the portal fade into the backgroud video
	if CHECK_BIT(frame,0){
		event_horizon_transparency_level = 1.0;
	}else{
		//wait for zoom to finish before fading to background
		if (event_horizon_transparency_level == 1.0 and global_zoom >= 0.5){ //wait for portal to be open a bit before unfading
			event_horizon_transparency_level = .99; //bump the transparency and let it fall the rest of the way
		}
	}
	
	//let transparency fall to maximum
	if (event_horizon_transparency_level > 0.0 and event_horizon_transparency_level < 1.0 ){
		event_horizon_transparency_level = event_horizon_transparency_level - 0.01;
		if (event_horizon_transparency_level < 0) event_horizon_transparency_level = 0;
	}
	

	last_acceleration[0] = acceleration[0];
	last_acceleration[1] = acceleration[1];
}


GLuint video_quad_vertex_list,event_horizon_vertex_list,event_horizon2_vertex_list,portal_vertex_list;

void model_board_init(void)
{
	

	for (int i = 0; i <  360; i ++){
		//offset_thing[i] = sin(((float)i)/360.0 * M_PI);
		offset_thing[i] = 3;
	}
	
	orange_n = png_texture_load( "assets/orange_n.png", NULL, NULL);
	//override default of clamp
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	
	blue_n = png_texture_load( "assets/blue_n.png", NULL, NULL);
	//override default of clamp
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	
	orange_0 = png_texture_load( "assets/orange_0.png", NULL, NULL);
	orange_1 = png_texture_load( "assets/orange_1.png", NULL, NULL);

	blue_0 = png_texture_load( "assets/blue_0.png", NULL, NULL);
	blue_1 = png_texture_load( "assets/blue_1.png", NULL, NULL);
	
	texture_orange = png_texture_load( "assets/orange_portal.png", NULL, NULL);
	texture_blue   = png_texture_load( "assets/blue_portal.png",   NULL, NULL);
	
	circle64 = png_texture_load( "assets/circle64.png", NULL, NULL);
	
	if (orange_n == 0 || blue_n == 0 || texture_orange == 0 || texture_blue == 0 || orange_0 == 0 || orange_1 == 0 || blue_0 == 0 || blue_1 == 0 || circle64 == 0)
	{
		throw std::runtime_error("Loading textures failed.");
	}


	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND); 
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE0);	
	glTexEnvi(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE);
	glActiveTexture(GL_TEXTURE0);
	
	video_quad_vertex_list = glGenLists( 1 );
	glNewList( video_quad_vertex_list, GL_COMPILE );
	#define VIDEO_DEPTH -0.003
	#define VIDEO_SCALE 11
	glBegin( GL_QUADS );
	glTexCoord2f( 0.0f, 0.0f );	glVertex3f( -VIDEO_SCALE * 4/3, VIDEO_SCALE,VIDEO_DEPTH);//top left
	glTexCoord2f( 0.0f, 1.0f ); glVertex3f( -VIDEO_SCALE * 4/3,-VIDEO_SCALE,VIDEO_DEPTH);//bottom left
	glTexCoord2f( 1.0f, 1.0f );	glVertex3f(  VIDEO_SCALE * 4/3,-VIDEO_SCALE,VIDEO_DEPTH);//bottom right
	glTexCoord2f( 1.0f, 0.0f );	glVertex3f(  VIDEO_SCALE * 4/3, VIDEO_SCALE,VIDEO_DEPTH);//top right
	glEnd();
	glEndList();

	event_horizon_vertex_list = glGenLists( 1 );
	glNewList( event_horizon_vertex_list, GL_COMPILE );
	#define EVENT_HORIZON_DEPTH -0.002
	#define EVENT_HORIZON_SCALE 11
	glBegin( GL_QUADS );
	glTexCoord2f( 0.0f, 0.0f );	glVertex3f( -EVENT_HORIZON_SCALE, EVENT_HORIZON_SCALE,EVENT_HORIZON_DEPTH);//top left
	glTexCoord2f( 0.0f, 1.0f ); glVertex3f( -EVENT_HORIZON_SCALE,-EVENT_HORIZON_SCALE,EVENT_HORIZON_DEPTH);//bottom left
	glTexCoord2f( 1.0f, 1.0f );	glVertex3f(  EVENT_HORIZON_SCALE,-EVENT_HORIZON_SCALE,EVENT_HORIZON_DEPTH);//bottom right
	glTexCoord2f( 1.0f, 0.0f );	glVertex3f(  EVENT_HORIZON_SCALE, EVENT_HORIZON_SCALE,EVENT_HORIZON_DEPTH);//top right
	glEnd( );
	glEndList();
	
	event_horizon2_vertex_list = glGenLists( 1 );
	glNewList( event_horizon2_vertex_list, GL_COMPILE );
	#define EVENT_HORIZON2_DEPTH -0.001
	#define EVENT_HORIZON2_SCALE 11
	glBegin( GL_QUADS );
	glTexCoord2f( 0.0f, 0.0f );	glVertex3f( -EVENT_HORIZON2_SCALE, EVENT_HORIZON2_SCALE,EVENT_HORIZON2_DEPTH);//top left
	glTexCoord2f( 0.0f, 1.0f ); glVertex3f( -EVENT_HORIZON2_SCALE,-EVENT_HORIZON2_SCALE,EVENT_HORIZON2_DEPTH);//bottom left
	glTexCoord2f( 1.0f, 1.0f );	glVertex3f(  EVENT_HORIZON2_SCALE,-EVENT_HORIZON2_SCALE,EVENT_HORIZON2_DEPTH);//bottom right
	glTexCoord2f( 1.0f, 0.0f );	glVertex3f(  EVENT_HORIZON2_SCALE, EVENT_HORIZON2_SCALE,EVENT_HORIZON2_DEPTH);//top right
	glEnd( );
	glEndList();
	
	portal_vertex_list = glGenLists( 1 );
	glNewList( portal_vertex_list, GL_COMPILE );
	//#define PORTAL_DEPTH -1.6
	#define PORTAL_DEPTH 0
	#define PORTAL_SCALE 28.5
	glBegin( GL_QUADS );
	glTexCoord2f( 0.0f, 0.0f );	glVertex3f( -PORTAL_SCALE, PORTAL_SCALE,PORTAL_DEPTH);//top left
	glTexCoord2f( 0.0f, 1.0f ); glVertex3f( -PORTAL_SCALE,-PORTAL_SCALE,PORTAL_DEPTH);//bottom left
	glTexCoord2f( 1.0f, 1.0f ); glVertex3f(  PORTAL_SCALE,-PORTAL_SCALE,PORTAL_DEPTH);//bottom right
	glTexCoord2f( 1.0f, 0.0f );	glVertex3f(  PORTAL_SCALE, PORTAL_SCALE,PORTAL_DEPTH);//top right
	glEnd( );
	glEndList();
}


static float zshuffle = -3;
void model_board_redraw(GLuint video_texture, int frame){	

	//RESTART - CHECK IF I NEED TO SET ALL OF THESE EACH CYCLE!
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0, 0, -30);
	
	glPushMatrix();
	
	//global scale
	glScalef(global_zoom,global_zoom,1.0);   //maybe use a Z zoom?
	
	//VIDEO QUAD
	glBindTexture (GL_TEXTURE_2D, video_texture); //video frame from gstreamer
	glColor4f(1.0,1.0,1.0,1.0); //video is not transparent at all
	glCallList(video_quad_vertex_list);
	
	//EVENT HORIZON QUAD
	glPushMatrix(); //save positions pre-rotation
	
	glScalef(1.0*720/480,1.0,1.0);  //stretch portal to an oval
	
	CHECK_BIT(frame,1) ? glBindTexture(GL_TEXTURE_2D, orange_0) : glBindTexture(GL_TEXTURE_2D, blue_0); //base event horizon texture	
	glColor4f(1.0,1.0,1.0,MIN(1.0,event_horizon_transparency_level)); //not transparent until forced open
	glRotatef(event_horizon_spin, 0, 0,1.0); //rotate background	
	glCallList(event_horizon_vertex_list);

	CHECK_BIT(frame,1) ? glBindTexture(GL_TEXTURE_2D, orange_1) : glBindTexture(GL_TEXTURE_2D, blue_1); //base event horizon texture	
	glColor4f(1.0,1.0,1.0, MIN( event_horizon_vertex_list_shimmer ,event_horizon_transparency_level)); //shimmer transparency until forced open
	glRotatef(event_horizon_spin, 0, 0,1.0); //rotate background more
	glCallList(event_horizon2_vertex_list);
	
	glPopMatrix(); //un-rotate and unstretch

	//PORTAL RIM QUAD
	glPushMatrix(); //save positions pre-rotation and scaling
	
	CHECK_BIT(frame,2) ? glBindTexture(GL_TEXTURE_2D, texture_orange) : glBindTexture(GL_TEXTURE_2D, texture_blue); //portal texture
	glColor4f(1.0,1.0,1.0,1.0); //portal texture is not transparent
	glScalef(1.0*720/480,1.0,1.0);  //stretch portal to an oval
	glRotatef(portal_spin, 0, 0, 1); //make it spin
	glCallList(portal_vertex_list);
	
	//TEXTURED TORUS
	CHECK_BIT(frame,1) ? glBindTexture(GL_TEXTURE_2D, orange_n) : glBindTexture(GL_TEXTURE_2D, blue_n);
	draw_torus(1.4 , 9.8 , 30, 60);
	donut_texture_scrolling+=.05;
	
	//POINT SPRITES
	glBindTexture(GL_TEXTURE_2D, circle64);
	

	glPointSize(20.0f);
	glEnable(GL_POINT_SPRITE_ARB);

	glBegin(GL_POINTS);
	glVertex3f(zshuffle,zshuffle,zshuffle);
	glEnd();
	
	
	glDisable( GL_POINT_SPRITE_ARB );
	glPopMatrix(); //un-rotate and unscale the portal
	glPopMatrix(); //unscale the global
	
	
	zshuffle += .05;
	if (zshuffle > 3) (zshuffle = -3);
}

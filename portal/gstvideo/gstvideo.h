#ifndef _GSTVIDEO_H 
#define _GSTVIDEO_H

#define AHRS_CLOSED 0
#define AHRS_OPEN_BLUE 8
#define AHRS_OPEN_ORANGE 14
#define AHRS_CLOSED_BLUE 9
#define AHRS_CLOSED_ORANGE 15

#define GST_BLANK 0
#define GST_VIDEOTESTSRC 1 
#define GST_VIDEOTESTSRC_CUBED 2 
#define GST_RPICAMSRC 3
#define GST_NORMAL 4

#define GST_LIBVISUAL_FIRST 10
#define GST_LIBVISUAL_JESS 10
#define GST_LIBVISUAL_INFINITE 11
#define GST_LIBVISUAL_JAKDAW 12
#define GST_LIBVISUAL_OINKSIE 13
#define GST_GOOM 14
#define GST_GOOM2K1 15
#define GST_LIBVISUAL_LAST 15

#define GST_FIRST_EFFECT 18
#define GST_FIRST_TV_EFFECT 18
#define GST_STREAKTV 18
#define GST_RADIOACTV 19 
#define GST_REVTV 20
#define GST_AGINGTV 21
#define GST_DICETV 22
#define GST_WARPTV 23
#define GST_SHAGADELICTV 24
#define GST_VERTIGOTV 25
#define GST_RIPPLETV 28 
#define GST_EDGETV 29
#define GST_LAST_TV_EFFECT 29

#define GST_FIRST_GL_EFFECT 30
#define GST_GLCUBE 30
#define GST_GLMIRROR 31
#define GST_GLSQUEEZE 32
#define GST_GLSTRETCH 33
#define GST_GLTUNNEL 34
#define GST_GLTWIRL 35
#define GST_GLBULGE 36
#define GST_GLHEAT 37
#define GST_LAST_GL_EFFECT 37
#define GST_LAST_EFFECT 37			
				
#define GST_MOVIE_FIRST 50
#define GST_MOVIE1 50
#define GST_MOVIE2 51
#define GST_MOVIE3 52
#define GST_MOVIE4 53
#define GST_MOVIE5 54
#define GST_MOVIE6 55
#define GST_MOVIE7 56
#define GST_MOVIE8 57
#define GST_MOVIE9 58
#define GST_MOVIE10 59
#define GST_MOVIE11 60
#define GST_MOVIE12 61
#define GST_MOVIE_LAST 61

//these are the magic numbers for start times, in nanoseconds
// 11000000000 is 11 seconds
const long long int movie_start_times[12] = { 11000000000, 13000000000, 16000000000, 18000000000, 19000000000, 20000000000, 23000000000, 24000000000, 11000000000, 11000000000, 11000000000, 11000000000 };
const long long int movie_end_times[12] =   { 13000000000, 16000000000, 18000000000, 19000000000, 20000000000, 22000000000, 24000000000, 25000000000, 11000000000, 11000000000, 11000000000, 11000000000 };
		
#endif
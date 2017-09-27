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

#define GST_MOVIE1_LENGTH 24491
#define GST_MOVIE2_LENGTH 16783
#define GST_MOVIE3_LENGTH 12079
#define GST_MOVIE4_LENGTH 82000
#define GST_MOVIE5_LENGTH 30430
#define GST_MOVIE6_LENGTH 22923
#define GST_MOVIE7_LENGTH 19086
#define GST_MOVIE8_LENGTH 70000
#define GST_MOVIE9_LENGTH 94000
#define GST_MOVIE10_LENGTH 53186
#define GST_MOVIE11_LENGTH 184000
#define GST_MOVIE12_LENGTH 140000

const long long int movie_1_start = 0;
const long long int movie_2_start = GST_MOVIE1_LENGTH;
const long long int movie_3_start = movie_2_start + GST_MOVIE2_LENGTH;
const long long int movie_4_start = movie_3_start + GST_MOVIE3_LENGTH;
const long long int movie_5_start = movie_4_start + GST_MOVIE4_LENGTH;
const long long int movie_6_start = movie_5_start + GST_MOVIE5_LENGTH;;
const long long int movie_7_start = movie_6_start + GST_MOVIE6_LENGTH;;
const long long int movie_8_start = movie_7_start + GST_MOVIE7_LENGTH;
const long long int movie_9_start = movie_8_start + GST_MOVIE8_LENGTH;;
const long long int movie_10_start = movie_9_start + GST_MOVIE9_LENGTH;
const long long int movie_11_start = movie_10_start + GST_MOVIE10_LENGTH;
const long long int movie_12_start = movie_11_start + GST_MOVIE11_LENGTH;

#define MOVIE_OFFSET_START 10000
#define MOVIE_OFFSET_END 10000

const long long int movie_start_times[12] = { movie_1_start + MOVIE_OFFSET_START, movie_2_start + MOVIE_OFFSET_START, movie_3_start + MOVIE_OFFSET_START, movie_4_start + MOVIE_OFFSET_START, movie_5_start + MOVIE_OFFSET_START, movie_6_start + MOVIE_OFFSET_START, movie_7_start + MOVIE_OFFSET_START, movie_8_start + MOVIE_OFFSET_START, movie_9_start + MOVIE_OFFSET_START, movie_10_start + MOVIE_OFFSET_START, movie_11_start + MOVIE_OFFSET_START, movie_12_start + MOVIE_OFFSET_START };
const long long int movie_end_times[12] =   { movie_2_start - MOVIE_OFFSET_END  , movie_3_start - MOVIE_OFFSET_END  ,  movie_4_start - MOVIE_OFFSET_END ,  movie_5_start - MOVIE_OFFSET_END ,  movie_6_start - MOVIE_OFFSET_END ,  movie_7_start - MOVIE_OFFSET_END ,  movie_8_start - MOVIE_OFFSET_END ,  movie_9_start - MOVIE_OFFSET_END , movie_10_start - MOVIE_OFFSET_END , movie_11_start - MOVIE_OFFSET_END  , movie_12_start - MOVIE_OFFSET_END ,  movie_2_start + GST_MOVIE12_LENGTH - MOVIE_OFFSET_END  };
		
#endif
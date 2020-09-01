#ifndef _TEXTRTCPCLIENT_H_
#define _TEXTRTCPCLIENT_H_

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/mem.h>
#include <libavutil/dict.h>
}
#include <stdio.h>
#include <SDL.h>
#include <SDL_video.h>
#include <SDL_render.h>
#include <SDL_rect.h>
#include <SDL_thread.h>
#include <SDL_mutex.h>
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"

#define REFRESH_EVENT (SDL_USEREVENT+1)
#define BREAK_EVENT  (SDL_USEREVENT+2)

#define RTSP_CLIENT_VERBOSITY_LEVEL 1 // by default, print verbose output from each "RTSPClient"
#define DEBUG_PRINT_EACH_RECEIVED_FRAME 0
#define DUMMY_SINK_RECEIVE_BUFFER_SIZE 100000
#define REQUEST_STREAMING_OVER_TCP False     
char eventLoopWatchVariable = 0;
static unsigned rtspClientCount = 0; // Counts how many streams (i.e., "RTSPClient"s) are currently in use.
typedef struct structData
{
	AVFormatContext*    p_fmt_ctx;
	AVPacket p_packet;
	AVCodecContext *p_codec_ctx;
	AVCodec      *p_codec ;
	AVFrame *p_frm_raw ;        // 帧，由包解码得到原始帧
	AVFrame *p_frm_yuv;        // 帧，由原始帧色彩转换得到
	AVCodecParserContext *p_CodecParserCtx;
	struct SwsContext*  sws_ctx ;
	SDL_Window*         screen; 
	SDL_Renderer*       sdl_renderer;
	SDL_Texture*        sdl_texture;
	SDL_Rect            sdl_rect;
	structData()
	{
		p_fmt_ctx = NULL;
		p_codec_ctx=NULL;
		p_codec = NULL;
		p_frm_raw = NULL;       
		p_frm_yuv = NULL;        
		p_CodecParserCtx=NULL;
		sws_ctx = NULL;
	}
}structData;

structData *pstructData;
int                 ret;
int                 buf_size;
uint8_t*            buffer = NULL;

SDL_Event mEvent;
int thread_exit=0;
// RTSP 'response handlers':
void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString);
void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString);
void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString);

// Other event handler functions:
void subsessionAfterPlaying(void* clientData); // called when a stream's subsession (e.g., audio or video substream) ends

void subsessionByeHandler(void* clientData, char const* reason);// called when a RTCP "BYE" is received for a subsession

void streamTimerHandler(void* clientData);// called at the end of a stream's expected duration (if the stream has not already signaled its end using a RTCP "BYE")

void openURL(UsageEnvironment& env, char const* progName, char const* rtspURL);// The main streaming routine (for each "rtsp://" URL):

void setupNextSubsession(RTSPClient* rtspClient);// Used to iterate through each stream's 'subsessions', setting up each one:

void shutdownStream(RTSPClient* rtspClient, int exitCode = 1);// Used to shut down and close a stream (including its "RTSPClient" object):

void FFmepgInit();

void SDLInit();
int SDLThread(void *data);

// Define a class to hold per-stream state that we maintain throughout each stream's lifetime:
class StreamClientState {
public:
	StreamClientState();
	virtual ~StreamClientState();

public:
	MediaSubsessionIterator* iter;
	MediaSession* session;
	MediaSubsession* subsession;
	TaskToken streamTimerTask;
	double duration;
};


class ourRTSPClient: public RTSPClient {
public:
	static ourRTSPClient* createNew(UsageEnvironment& env, char const* rtspURL,int verbosityLevel = 0,char const* applicationName = NULL,portNumBits tunnelOverHTTPPortNum = 0);

protected:
	ourRTSPClient(UsageEnvironment& env, char const* rtspURL,int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum);
	// called only by createNew();
	virtual ~ourRTSPClient();

public:
	StreamClientState scs;
};

// Define a data sink (a subclass of "MediaSink") to receive the data for each subsession (i.e., each audio or video 'substream').
// In practice, this might be a class (or a chain of classes) that decodes and then renders the incoming audio or video.
// Or it might be a "FileSink", for outputting the received data into a file (as is done by the "openRTSP" application).
// In this example code, however, we define a simple 'dummy' sink that receives incoming data, but does nothing with it.

class DummySink: public MediaSink {

public:
	static DummySink* createNew(UsageEnvironment& env,MediaSubsession& subsession, /* identifies the kind of data that's being received*/char const* streamId = NULL); /*identifies the stream itself (optional)*/

private:
	DummySink(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId);
	// called only by "createNew()"
	virtual ~DummySink();

	static void afterGettingFrame(void* clientData,unsigned frameSize,unsigned numTruncatedBytes,struct timeval presentationTime,unsigned durationInMicroseconds);
	void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,struct timeval presentationTime, unsigned durationInMicroseconds);

private:
	// redefined virtual functions:
	virtual Boolean continuePlaying();

private:
	u_int8_t* fReceiveBuffer;
	u_int8_t*  decBuffer;
	bool firstFrame;
	MediaSubsession& fSubsession;
	char* fStreamId;
};
#endif // !_TEXTRTCPCLIENT_H_

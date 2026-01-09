#include "USBNETVideo.hpp"
#include "PracticalSocket.h"
#include "opencv2/opencv.hpp"
#include "opencv2/imgcodecs.hpp"
#include "KIPRnet.hpp"

#include <poll.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#define DEST_PIXEL_FMT AV_PIX_FMT_BGR24
#define AI_CAMERA_H 720
#define AI_CAMERA_W 1280
#define UDP_RCV_TIMEOUT 500000 // in microseconds

#define FRAME_TIMEOUT 5000 // in milliseconds

#define FRAME_HEIGHT 720
#define FRAME_WIDTH

#define BUF_LEN 65540
#define PACK_SIZE 4096

#define AI_CAMERA_VIDEO_PORT "9000"
#define AI_CAMERA_COMMAND_PORT 5556
#define AI_CAMERA_IP "192.168.1.2"

//#define DEBUG_USBNET 1
#include <ostream>
using namespace std;
using namespace cv;

//char * UsbnetVideo::m_object_list;

UsbFrameProcessor::UsbFrameProcessor(
    const char *drone_ip_address, const short unsigned int drone_port,
    const int destw, const int desth)
{
  // todo: allow caller to set ip address
  m_udp_video_up = true;
  m_vdr_running = false;

  m_ldestw = destw;
  m_ldesth = desth;

  m_next_object_list = nullptr;

  // start up the UDP packet receiver thread
  m_vdr_thread = new std::thread([this]()
                                 { run_vdr(); });

  while (!m_vdr_running)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }

  std::cout << "UsbFrameProcessor initialized" << std::endl;
}

UsbFrameProcessor::~UsbFrameProcessor() throw()
{
  std::cout << "UsbFrameProcessor - terminating" << std::endl;

  m_udp_video_up = false;
  try
  {
    m_vdr_thread->join();
  }

  catch (int ex)
  {
    std::cout << "~UsbFrameProcessor - exception :" << ex
              << std::endl
              << std::flush;
  }
  // Cleanup
  std::cout << "iUsbFrameProcessor - terminated" << std::endl;
  fflush(NULL);
}

void UsbFrameProcessor::run_vdr()
{

  std::cout << "UsbFrameProcessor started - run_vdr" << std::endl;

  // Video packet processing loop
  // receive UDP packets, reassemble frame packets and send them on
  m_vdr_running = true;

    unsigned short servPort = atoi(AI_CAMERA_VIDEO_PORT);

	UDPSocket sock( "192.168.1.1", servPort);
        char buffer[BUF_LEN]; // Buffer for echo string
        int recvMsgSize; // Size of received message
        string sourceAddress; // Address of datagram source
        unsigned short sourcePort; // Port of datagram source

        clock_t last_cycle = clock();
        while (m_udp_video_up) {
            // Block until receive message from a client

start_block:
            do {
                recvMsgSize = sock.recvFrom(buffer, BUF_LEN, sourceAddress, sourcePort);
            } while (recvMsgSize == PACK_SIZE);

            int total_pack = ((int *) buffer)[0];
            cout << "expected number of packets: " << total_pack << " HeaderSize " << recvMsgSize << endl;
            char * longbuf = new char[PACK_SIZE * total_pack];
	    char * object_list;
		int object_count;
		if(recvMsgSize > (2*sizeof(int)))
		{
			memcpy(&object_count, buffer + sizeof(int), sizeof(int));
			object_list = new char[sizeof(int) + (object_count * sizeof(object_detection))];
			memcpy(object_list, buffer + sizeof(int), sizeof(int) + (object_count * sizeof(object_detection)));
		
		}
		else object_list = nullptr;

            for (int i = 0; i < total_pack; i++) {
                recvMsgSize = sock.recvFrom(buffer, BUF_LEN, sourceAddress, sourcePort);
                if (recvMsgSize != PACK_SIZE) {
                    cerr << "Received unexpected size pack:" << recvMsgSize << endl;
fflush(NULL);
		   free(longbuf);
                   goto start_block;   // somethings wrong - find next frame 
                }
                memcpy( & longbuf[i * PACK_SIZE], buffer, PACK_SIZE);
            }

            cout << "run-vdt Received packet from " << sourceAddress << ":" << sourcePort << endl;
            Mat rawData = Mat(1, PACK_SIZE * total_pack, CV_8UC1, longbuf);
#ifdef VDTDEBUG
/*DEBUG*/ cout << "run-vdt - rawData.size().width: " << rawData.size().width << endl;
/*DEBUG*/ cout << "run-vdt rawData.rows: " << rawData.rows << " rawData.cols: " << rawData.cols << endl;
#endif
// DEBUG dig out the codec signature string

size_t maxlen = 3;
            Mat frame = imdecode(rawData, IMREAD_COLOR|IMREAD_ANYDEPTH|IMREAD_IGNORE_ORIENTATION);
#ifdef VDTDEBUG
/*DEBUG*/ cout << "run-vdt - frame.size().width: " << frame.size().width << endl;
/*DEBUG*/ cout << "run-vdt - frame.depth(): " << frame.depth() << " frame.channels(): " << frame.channels() << endl;
#endif
	    free(longbuf);
            if (frame.size().width == 0) {
                cerr << "decode failure!" << endl;
                continue;
            }
        
            clock_t next_cycle = clock();
            double duration = (next_cycle - last_cycle) / (double) CLOCKS_PER_SEC;
/*            cout << " run-vdt effective FPS: " << (1 / duration) << " kbps: " << (PACK_SIZE * total_pack / duration / 1024 * 8) << endl;
*/
            cout << next_cycle - last_cycle;
            last_cycle = next_cycle;
            add_pframe_to_list(frame, object_list);
     }
  std::cout << "UsbFrameProcessor - run_vdr stopped" << std::endl;
}

#define FRAME_SLEEP_TIME 10 // in milliseconds
bool UsbFrameProcessor::get_pframe_from_list(cv::OutputArray image, char* & object_list)
{
  unsigned long wait_time = 0;
  while (!m_new_frame)
  {
    // wait until we get something in the vector
    std::this_thread::sleep_for(std::chrono::milliseconds(FRAME_SLEEP_TIME));
    if (!m_udp_video_up)
      return false;

    wait_time += FRAME_SLEEP_TIME;
    if (wait_time > FRAME_TIMEOUT)
    {
      std::cout << "Frame timeout" << std::endl
                << std::flush;
      return false;
    }
  }

  m_vid_frame_mutex.lock();

#ifdef DEBUGGETPFRAME
/*DEBUG*/ cout << "get_pframe_from_list  m_next_frame.channels: " << m_next_frame.channels() << endl; 
/*DEBUG*/ cout << "get_pframe_from_list  image.channels: " << image.channels() << endl;
/*DEBUG*/ cout << "get_pframe_from_list  m_next_frame.depth: " << m_next_frame.depth() << endl;
/*DEBUG*/ cout << "get_pframe_from_list image.depth: " << image.depth() << endl;
#endif

	image.create(m_next_frame.rows, m_next_frame.cols, CV_8UC3);
	m_next_frame.copyTo(image.getMatRef());
  object_list = m_next_object_list;
  m_next_object_list = nullptr;
  m_new_frame = false;
  m_vid_frame_mutex.unlock();
  return true;
}

void UsbFrameProcessor::add_pframe_to_list(Mat &newFrame, char * object_list)
{
  m_vid_frame_mutex.lock();

  // make sure the requestor receives the most recent frame
  // so we have only a one frame queue
  m_next_frame = newFrame;
  if(m_next_object_list != nullptr)
	free(m_next_object_list);
  m_next_object_list = object_list;
	std::cout << "UsbFrameProcessor::add_pframe_to_list h: " << newFrame.rows
    << " w: " << newFrame.cols << endl;
  m_new_frame = true;
  m_vid_frame_mutex.unlock();
}

UsbnetVideo::UsbnetVideo(const char *drone_ip_address,
                   const short unsigned int drone_port, const int destw,
                   const int desth)
{
  m_udp_opened = true;
  m_data_receiver =
      new UsbFrameProcessor(drone_ip_address, drone_port, destw, desth);
  m_object_list = nullptr;

  std::cout << "USB/UDP Video started" << std::endl;
}

UsbnetVideo::~UsbnetVideo()
{
  m_udp_opened = false;
  delete m_data_receiver;
  std::cout << "~UsbnetVideo done" << std::endl << std::flush;
}

bool UsbnetVideo::isOpened() const { return m_udp_opened; }

bool UsbnetVideo::read(cv::OutputArray image)
{
  bool retval = false;
  char * object_list;
  if (isOpened())
  {
    retval = m_data_receiver->get_pframe_from_list(image, object_list);

    m_object_list = object_list;
  }
  return retval;
}

#define MAXRECVBUFF 256
#define AICAM_POLL_TIMEOUT 10000 /* in milliseconds */

char * ai_camera_command(char * command, int bufferlength)
{
	int flag = 1;
	int sockfd;
	int connectreturn;
	struct sockaddr_in servaddr;
	int commandLen = strlen(command);   // Determine input length
	int bytes_sent;

	// Establish connection with the AI Camera server

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd == -1)
	{
		cout << "socket creation failed" << endl;
		return NULL;
	}

	// assign IP, PORT

	memset(&servaddr, 0, sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(AI_CAMERA_IP);
	servaddr.sin_port = htons(AI_CAMERA_COMMAND_PORT);

	// connect  to the AI Camera command port

	connectreturn = connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));

	if(connectreturn != 0)
	{
		cout << "connection with AI server failed with error " << connectreturn << endl;
		perror("ai_camera_command connect");
		close(sockfd);
		return NULL;
	}

	// Send the string to the AI camera server

	setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));

	bytes_sent = send(sockfd, command, commandLen, 0);

	cout << "ai_camera_command - command sent: " << command << endl;

	int bytesReceived = 0;              // Bytes read on each recv()
	int totalBytesReceived = 0;         // Total bytes read
	cout << "ai_camera_command-Received: ";               // Setup to print the echoed string
        char * returnBuffer = (char *) malloc(MAXRECVBUFF);    // Buffer for echo string + \0

	bytesReceived = recv(sockfd, returnBuffer, MAXRECVBUFF-1, 0);

	if (bytesReceived < 0) {
		perror("ai_camera_command - recv");
		close(sockfd);
		free(returnBuffer);
		return NULL;
	}
	returnBuffer[bytesReceived] = '\0';        // Terminate the string!
	cout << "->"<< returnBuffer << "<-" << std::endl << std::flush ;

	close(sockfd);
	return returnBuffer;
}

bool ai_camera_check()
{
	bool rtn;
	char * status = (char *) "status";

	char * returnBuffer = ai_camera_command(status, 7);

	if(returnBuffer == NULL)
	{
		return false;
	}

	if(strncmp(returnBuffer, "ready", 6) == 0)
		rtn = true;
	else
		rtn = false;

	free(returnBuffer);
	return rtn;
}

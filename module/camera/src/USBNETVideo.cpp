#include "USBNETVideo.hpp"
#include "PracticalSocket.h"
#include "opencv2/opencv.hpp"
#include "opencv2/imgcodecs.hpp"

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

UsbFrameProcessor::UsbFrameProcessor(
    const char *drone_ip_address, const short unsigned int drone_port,
    const int destw, const int desth)
{
  // todo: allow caller to set ip address
  m_udp_video_up = true;
  m_vdr_running = false;

  m_ldestw = destw;
  m_ldesth = desth;

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
  av_parser_close(m_parser);
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
            do {
                recvMsgSize = sock.recvFrom(buffer, BUF_LEN, sourceAddress, sourcePort);
            } while (recvMsgSize > sizeof(int));
            int total_pack = ((int * ) buffer)[0];
            cout << "expecting length of packs:" << total_pack << endl;
            char * longbuf = new char[PACK_SIZE * total_pack];
            for (int i = 0; i < total_pack; i++) {
                recvMsgSize = sock.recvFrom(buffer, BUF_LEN, sourceAddress, sourcePort);
                if (recvMsgSize != PACK_SIZE) {
                    cerr << "Received unexpected size pack:" << recvMsgSize << endl;
                    continue;
                }
                memcpy( & longbuf[i * PACK_SIZE], buffer, PACK_SIZE);
            }

            cout << "run-vdt Received packet from " << sourceAddress << ":" << sourcePort << endl;

            Mat rawData = Mat(1, PACK_SIZE * total_pack, CV_8UC1, longbuf);
/*DEBUG*/ cout << "run-vdt - rawData.size().width: " << rawData.size().width << endl;
/*DEBUG*/ cout << "run-vdt rawData.rows: " << rawData.rows << " rawData.cols: " << rawData.cols << endl;

// DEBUG dig out the codec signature string

size_t maxlen = 3;

            Mat frame = imdecode(rawData, IMREAD_COLOR|IMREAD_ANYDEPTH|IMREAD_IGNORE_ORIENTATION);
/*DEBUG*/ cout << "run-vdt - frame.size().width: " << frame.size().width << endl;
/*DEBUG*/ cout << "run-vdt - frame.depth(): " << frame.depth() << " frame.channels(): " << frame.channels() << endl;
            if (frame.size().width == 0) {
                cerr << "decode failure!" << endl;
                continue;
            }




//////
//  	    Mat * output_frame = new Mat(frame.channels(), frame.rows * frame.cols*frame.channels(), frame.type(), frame.data);//, /*copyData=*/true);
	    free(longbuf);

	    //frame.copyTo(&output_frame);
        
            clock_t next_cycle = clock();
            double duration = (next_cycle - last_cycle) / (double) CLOCKS_PER_SEC;
            cout << "\t run-vdt effective FPS:" << (1 / duration) << " \tkbps:" << (PACK_SIZE * total_pack / duration / 1024 * 8) << endl;

            cout << next_cycle - last_cycle;
            last_cycle = next_cycle;
		add_pframe_to_list(frame);
           }
  std::cout << "UsbFrameProcessor - run_vdr stopped" << std::endl;
}

#define FRAME_SLEEP_TIME 10 // in milliseconds
bool UsbFrameProcessor::get_pframe_from_list(cv::OutputArray image)
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

cout << "get_pframe_from_list - lock wait  " << endl;
  m_vid_frame_mutex.lock();


/*DEBUG*/ cout << "get_pframe_from_list  m_next_frame.channels: " << m_next_frame.channels() << endl; 
/*DEBUG*/ cout << "get_pframe_from_list  image.channels: " << image.channels() << endl;
/*DEBUG*/ cout << "get_pframe_from_list  m_next_frame.depth: " << m_next_frame.depth() << endl;
/*DEBUG*/ cout << "get_pframe_from_list image.depth: " << image.depth() << endl;
//	m_next_frame->copyTo(image, CV_8UC3);
        //cv::Mat imaget = m_next_frame;
        //m_next_frame.copyTo(image, CV_8UC3);
	//av_free(m_next_frame);

	image.create(m_next_frame.rows, m_next_frame.cols, CV_8UC3);
        //cv::Mat& image2 = image.getMat();

	m_next_frame.copyTo(image.getMatRef());
        

  //m_next_frame = NULL;
  m_new_frame = false;
  m_vid_frame_mutex.unlock();
  return true;
}

void UsbFrameProcessor::add_pframe_to_list(Mat &newFrame)
{
  m_vid_frame_mutex.lock();

  // make sure the requestor receives the most recent frame
  // so we have only a one frame queue
  //if (m_next_frame != NULL)
  //{
    //av_free(m_next_frame);
  //}
  m_next_frame = newFrame;
	std::cout << "UsbFrameProcessor::add_pframe_to_list h: " << newFrame.rows
    << " w: " << newFrame.cols << endl;
  //newFrame = NULL;
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
  std::cout << "USB/UDP Video started" << std::endl;
}

UsbnetVideo::~UsbnetVideo()
{
  m_udp_opened = false;
  delete m_data_receiver;
  std::cout << "~UsbnetVideo done\n";
}

bool UsbnetVideo::isOpened() const { return m_udp_opened; }

bool UsbnetVideo::read(cv::OutputArray image)
{
  bool retval = false;
  if (isOpened())
    retval = m_data_receiver->get_pframe_from_list(image);
  return retval;
}

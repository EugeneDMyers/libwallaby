#include "boyd_p.hpp"

#include <iostream>
#include <daylite/spinner.hpp>
#include <daylite/bson.hpp>
#include <battlecreek/settings.hpp>

#include <thread>
#include <chrono>

Private::Camera *Private::Camera::instance()
{
  static Private::Camera camera;
  return &camera;
}

Private::Camera::Camera()
  : m_newFrameAvailable(false),
  m_userFrameValid(false),
  m_frameSub(nullptr)
{
  using namespace daylite;
  
  m_node = node::create_node("libwallabycam");
  if(!m_node->start("127.0.0.1", 8374))
  {
    // TODO: What should we do if this fails?
    std::cout << "Failed to contact daylite master" << std::endl;
    return;
  }
  
  m_settingsPub = m_node->advertise("camera/set_settings");
}

bool Private::Camera::open(const int deviceNumber, const int width, const int height)
{
  // TODO: Opening other devices not yet supported
  
  if(this->isOpen())
    return false;
  
  if(!m_node)
    return false;
  
  m_newFrameAvailable = false;
  // TODO: Callback directly?
  m_frameSub = m_node->subscribe("camera/frame_data", [this](const daylite::bson &msg, void *arg)
    {
      this->receivedFrame(msg, arg);
    });
  
  this->setWidth(width);
  this->setHeight(height);
  
  return true;
}

bool Private::Camera::isOpen()
{
  return m_frameSub ? true : false;
}

bool Private::Camera::close()
{
  // TODO: Is this the right way to stop subscribing?
  m_frameSub.reset();
  m_newFrameAvailable = false;
  
  return true;
}

void Private::Camera::loadConfig(const std::string &name)
{
  using namespace daylite;
  
  boyd::settings s;
  s.config_name = name;
  m_settingsPub->publish(bson(s.bind()));
}

void Private::Camera::setWidth(const int width)
{
  if(width <= 0) {
    std::cout << "Camera width must be greater than 0." << std::endl;
    return;
  }
  
  using namespace daylite;
  
  boyd::settings s;
  s.width = width;
  m_settingsPub->publish(bson(s.bind()));
  daylite::spinner::spin_once();
  
  // TODO: Do we have to send multiple times?
  /*for(int i = 0; i < 5; ++i) {
    m_settingsPub->publish(bson(s.bind()));
    daylite::spinner::spin_once();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }*/
}

void Private::Camera::setHeight(const int height)
{
  if(height <= 0) {
    std::cout << "Camera height must be greater than 0." << std::endl;
    return;
  }
  
  using namespace daylite;
  
  boyd::settings s;
  s.height = height;
  m_settingsPub->publish(bson(s.bind()));
  daylite::spinner::spin_once();
  
  // TODO: Do we have to send multiple times?
  /*for(int i = 0; i < 10; ++i) {
    m_settingsPub->publish(bson(s.bind()));
    daylite::spinner::spin_once();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }*/
}

int Private::Camera::width()
{
  if(!m_userFrameValid)
    return -1;
  
  return m_userFrame.width;
}

int Private::Camera::height()
{
  if(!m_userFrameValid)
    return -1;
  
  return m_userFrame.height;
}

bool Private::Camera::update()
{
  if(!this->isOpen())
    return false;
  
  // TODO: update() is blocking for now
  while(!m_newFrameAvailable) {
    daylite::spinner::spin_once();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
  
  if(!m_newFrameAvailable)
    return false;
  
  using namespace boyd;
  
  // Update m_userFrame (contains raw pixels)
  m_userFrame = frame_data::unbind(m_latestFrameBson);
  
  // Update m_channelBlobs
  // TODO: Only do this when we actually need m_channelBlobs
  m_channelBlobs.clear();
  const std::vector<channel_data> &channelData = m_userFrame.ch_data;
  for(const channel_data &cd : channelData) {
    const std::vector<struct blob> &blobs = cd.blobs;
    std::vector<Object> objs;
    for(const struct blob &b : blobs) {
      const Point2<unsigned> centroid(b.centroidX, b.centroidY);
      const Rect<unsigned> bbox(b.bBoxX, b.bBoxY, b.bBoxWidth, b.bBoxHeight);
      const Object obj(centroid, bbox, b.confidence);
      objs.push_back(obj);
    }
    m_channelBlobs.push_back(objs);
  }
  
  m_userFrameValid = true;
  m_newFrameAvailable = false;
  
  return true;
}

void Private::Camera::setConfigBasePath(const std::string &path)
{
  using namespace daylite;
  
  boyd::settings s;
  s.config_base_path = path;
  m_settingsPub->publish(bson(s.bind()));
}

bool Private::Camera::checkChannel(const int channel)
{
  if(!m_userFrameValid)
    return false;
  
  if(channel < 0 || channel >= m_channelBlobs.size())
    return false;
  
  return true;
}

bool Private::Camera::checkChannelObject(const int channel, const int object)
{
  if(!this->checkChannel(channel))
    return false;
  
  if(object < 0 || object >= m_channelBlobs[channel].size())
    return false;
  
  return true;
}

const std::vector<uint8_t> *Private::Camera::rawPixels()
{
  if(!m_userFrameValid)
    return NULL;
  
  return &m_userFrame.data;
}

const std::vector<std::vector<Object>> *Private::Camera::channelBlobs()
{
  if(!m_userFrameValid)
    return NULL;
  
  return &m_channelBlobs;
}

void Private::Camera::receivedFrame(const daylite::bson &msg, void *arg)
{
  // TODO: Clone msg?
  m_latestFrameBson = msg;
  m_newFrameAvailable = true;
}
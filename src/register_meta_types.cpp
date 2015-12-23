#include <QMetaType>
#include <rosgraph_msgs/Log.h>

namespace swri_console
{
void registerMetaTypes()
{
  qRegisterMetaType<size_t>("size_t");
  qRegisterMetaType<rosgraph_msgs::LogConstPtr>("rosgraph_msgs::LogConstPtr");
}
}  // namespace swri_console

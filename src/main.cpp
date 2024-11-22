#include "rtmp_pushstream.h"

#include "desktop_share.h"
#include <QtWidgets/QApplication>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);


    /*ThreadPool thread_pool;
    thread_pool.Start(4);
    EventLoop event_loop(&thread_pool);
    event_loop.Poll(INFINITY);
    RtmpPush push(&event_loop);
    
    std::string url = "rtmp://192.168.56.1:1935/live/stream";
    push.StartPush(url, 500);*/



    DesktopShare w;
	w.show();

    return a.exec();
}

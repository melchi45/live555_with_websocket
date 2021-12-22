#include "live5555/RTSPClientBase.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

static auto LOGGER = spdlog::stdout_color_st( "RTSPClientBase" );

static const char *getResultString( char *resultString ) {
    return resultString ? resultString : "N/A";
}

// 父构造函数的第三个参数是调试信息冗余级别
// The third parameter of the parent constructor is the level of debug information redundancy
RTSPClientBase::RTSPClientBase( UsageEnvironment &env, const char *rtspURL ) :
    RTSPClient( env, rtspURL, 0, NULL, 0, -1 ) {
}

void RTSPClientBase::start() {
    LOGGER->trace( "Starting RTSP client..." );
    this->rtspURL = rtspURL;
    LOGGER->trace( "Send RTSP command: DESCRIBE" );
    sendDescribeCommand( onDescribeResponse );
    LOGGER->trace( "Startup live555 eventloop" );
    envir().taskScheduler().doEventLoop( &eventLoopWatchVariable );
}


void RTSPClientBase::onDescribeResponse( RTSPClient *client, int resultCode, char *resultString ) {
    LOGGER->trace( "DESCRIBE response received, resultCode: {}", resultCode );
    RTSPClientBase *clientBase = (RTSPClientBase *) client;
    bool ok = false;
    if ( resultCode == 0 ) {
        clientBase->onDescribeResponse( resultCode, resultString );
    } else {
        LOGGER->trace( "Stopping due to DESCRIBE failure" );
        clientBase->stop();
    };
    delete[] resultString;
}

void RTSPClientBase::onDescribeResponse( int resultCode, const char *sdp ) {
    LOGGER->debug( "SDP received: \n{}", sdp );
    UsageEnvironment &env = envir();
    LOGGER->trace( "Create new media session according to SDP" );
    session = MediaSession::createNew( env, sdp );
    if ( session && session->hasSubsessions()) {
        MediaSubsessionIterator *it = new MediaSubsessionIterator( *session );
        // 遍历子会话，SDP中的每一个媒体行（m=***）对应一个子会话
        // Traversing sub-sessions, each media line (m = ***) in SDP corresponds to a sub-session
        while ( MediaSubsession *subsess = it->next()) {
            const char *mediumName = subsess->mediumName();
            // 初始化子会话，导致相应的RTPSource被创建
            // Initialize the child session, causing the corresponding RTPSource to be created
            LOGGER->trace( "Initialize sub session {}", mediumName );
            if ( !acceptSubSession( mediumName, subsess->codecName())) {
                continue;
            }
            acceptedSubSessionCount++;
            bool ok = subsess->initiate();
            if ( !ok ) {
                LOGGER->error( "Failed to initialize sub session: {}", mediumName );
                stop();
                break;
            }
            const Boolean muxed = subsess->rtcpIsMuxed();
            const char *codec = subsess->codecName();
            const int port = subsess->clientPortNum();
            LOGGER->debug( "Initialized sub session... \nRTCP Muxed: {}\nPort: {}\nMedium : {}\nCodec: {}", muxed, port, mediumName, codec );

            LOGGER->trace( "Send RTSP command: SETUP for subsession {}", mediumName );
            sendSetupCommand( *subsess, onSetupResponse, False, False );
        }
    } else {
        stop();
    }
}

void RTSPClientBase::onSetupResponse( RTSPClient *client, int resultCode, char *resultString ) {
    LOGGER->trace( "SETUP response received, resultCode: {}, resultString: {}", resultCode, getResultString( resultString ));
    RTSPClientBase *clientBase = (RTSPClientBase *) client;
    if ( resultCode == 0 ) {
        clientBase->preparedSubSessionCount++;
        clientBase->onSetupResponse( resultCode, resultString );
    } else {
        LOGGER->trace( "Stopping due to SETUP failure" );
        clientBase->stop();
    }
    delete[] resultString;
}

void RTSPClientBase::onSetupResponse( int resultCode, const char *resultString ) {
    if ( preparedSubSessionCount == acceptedSubSessionCount ) {
        MediaSubsessionIterator *it = new MediaSubsessionIterator( *session );
        while ( MediaSubsession *subsess = it->next()) {
            const char *mediumName = subsess->mediumName();
            const char *codec = subsess->codecName();
            if ( acceptSubSession( mediumName, codec )) {
                MediaSink *sink = createSink( mediumName, codec, subsess );
                // 让Sink回调能够感知Client对象
                // Make Sink callbacks aware of Client objects
                subsess->miscPtr = this;
                // 导致Sink的continuePlaying被调用，准备接受数据推送
                // Cause Sink's continuePlaying to be called, ready to accept data push
                sink->startPlaying( *subsess->readSource(), NULL, subsess );
                // 此时数据推送不会立即开始，直到调用STSP命令PLAY
                // At this time, the data push will not start immediately until the STSP command PLAY is called.
                RTCPInstance *rtcp = subsess->rtcpInstance();
                if ( rtcp ) {
                    // 正确处理针对此子会话的RTCP命令
                    // Correctly handle RTCP commands for this sub-session
                    rtcp->setByeHandler( onSubSessionClose, subsess );
                }
                LOGGER->trace( "Send RTSP command: PLAY" );
                // PLAY命令可以针对整个会话，也可以针对每个子会话
                // The PLAY command can target the entire session or each sub-session
                sendPlayCommand( *session, onPlayResponse );
            }
        }
    }
}

void RTSPClientBase::onPlayResponse( RTSPClient *client, int resultCode, char *resultString ) {
    LOGGER->trace( "PLAY response received, resultCode: {}, resultString: {}", resultCode, getResultString( resultString ));
    RTSPClientBase *clientBase = (RTSPClientBase *) client;
    if ( resultCode == 0 ) {
        clientBase->onPlayResponse( resultCode, resultString );
    } else {
        LOGGER->trace( "Stopping due to PLAY failure" );
        clientBase->stop();
    }
    delete[] resultString;
}

void RTSPClientBase::onPlayResponse( int resultCode, char *resultString ) {
    // 此时服务器应该开始推送流过来
    // 如果播放的是定长的录像，这里应该注册回调，在时间到达后关闭客户端
    // At this point, the server should start to push the stream over.
    // If a fixed-length video is playing, you should register a callback here,
    // and close the client after the time is up.
    double &startTime = session->playStartTime();
    double &endTime = session->playEndTime();
    if(startTime == endTime) LOGGER->debug("Session is infinite" );
}

void RTSPClientBase::onSubSessionClose( void *clientData ) {
    MediaSubsession *subsess = (MediaSubsession *) clientData;
    RTSPClientBase *clientBase = (RTSPClientBase *) subsess->miscPtr;
    clientBase->onSubSessionClose( subsess );
}

void RTSPClientBase::onSubSessionClose( MediaSubsession *subsess ) {
    LOGGER->debug( "Stopping subsession..." );
    // 首先关闭子会话的SINK
    // First close the SINK of the sub-session
    Medium::close( subsess->sink );
    subsess->sink = NULL;

    // 检查是否所有兄弟子会话均已经结束
    // Check if all sibling child sessions have ended
    MediaSession &session = subsess->parentSession();
    MediaSubsessionIterator iter( session );
    while (( subsess = iter.next()) != NULL ) {
        // 存在未结束的子会话，不能关闭当前客户端
        // There is an unterminated sub-session, the current client cannot be closed
        if ( subsess->sink != NULL ) return;
    }
    // 关闭客户端
    // Close client
    LOGGER->debug( "All subsession closed" );
    stop();
}

void RTSPClientBase::stop() {
    LOGGER->debug( "Stopping RTSP client..." );
    // 修改事件循环监控变量
    // Modify event loop monitoring variables
    eventLoopWatchVariable = 0;
    UsageEnvironment &env = envir();
    if ( session != NULL ) {
        Boolean someSubsessionsWereActive = False;
        MediaSubsessionIterator iter( *session );
        MediaSubsession *subsession;
        // 检查是否存在需要处理的子会话
        // Check if there are child sessions that need to be processed
        while (( subsession = iter.next()) != NULL ) {
            if ( subsession->sink != NULL ) {
                // 强制关闭子会话的SINK
                // Force to close the SINK of the child session
                Medium::close( subsession->sink );
                subsession->sink = NULL;
                if ( subsession->rtcpInstance() != NULL ) {
                    // 服务器可能在处理TEARDOWN时发来RTCP包BYE
                    // The server may send an RTCP packet BYE while processing TEARDOWN
                    subsession->rtcpInstance()->setByeHandler( NULL, NULL );
                }
                someSubsessionsWereActive = True;
            }
        }

        if ( someSubsessionsWereActive ) {
            // 向服务器发送TEARDOWN命令，让服务器关闭输入流
            // Send a TEARDOWN command to the server to make the server close the input stream
            sendTeardownCommand( *session, NULL );
        }
    }
    // 关闭客户端
    // Close client
    Medium::close( this );
}

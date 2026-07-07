#include <wups.h>
#include <nsysnet/_socket.h>
#include <coreinit/title.h>
#include <utils/logger.h>
#include "retain_vars.hpp"
#include "EncodingHelper.h"
#include "MJPEGStreamServerUDP.hpp"
#include "HeartBeatServer.hpp"

// Mandatory plugin information.
WUPS_PLUGIN_NAME("Wii U Screen Streaming");
WUPS_PLUGIN_DESCRIPTION("Streams the Wii U screen to a PC on the same network (use StreamingPluginClient).");
WUPS_PLUGIN_VERSION("v0.1");
WUPS_PLUGIN_AUTHOR("Maschell, ItsOmey");
WUPS_PLUGIN_LICENSE("GPL");

WUPS_USE_WUT_DEVOPTAB();

static void startStreaming() {
    DEBUG_FUNCTION_LINE("startStreaming: begin\n");
    EncodingHelper::destroyInstance();
    EncodingHelper::getInstance()->StartAsyncThread();
    EncodingHelper::getInstance()->setMJPEGStreamServer(HeartBeatServer::getInstance()->getMJPEGServer());
    DEBUG_FUNCTION_LINE("startStreaming: done, heartbeat server should be listening\n");
}

// Returns true for the Wii U Menu and other system applications
// (Settings, Mii Maker, etc.) which all live under the 0x00050010 range.
// Running our socket server + capture threads during these hangs the console
// at boot, and they can't be meaningfully streamed anyway.
static bool isSystemMenuTitle() {
    return (uint32_t)(OSGetTitleID() >> 32) == 0x00050010;
}

// Gets called once the loader exists. Keep this minimal - all real setup is
// deferred to ON_APPLICATION_START so nothing heavy runs at plugin load time.
INITIALIZE_PLUGIN() {
    DEBUG_FUNCTION_LINE("INITIALIZE_PLUGIN fired\n");
}

// Called whenever an application was started.
ON_APPLICATION_START() {
    DEBUG_FUNCTION_LINE("ON_APPLICATION_START fired\n");

    // Stay completely passive in the Wii U Menu / system apps: no sockets, no
    // threads, and gAppStatus stays BACKGROUND so the GX2 hook does nothing.
    if (isSystemMenuTitle()) {
        gAppStatus = APP_STATUS_BACKGROUND;
        return;
    }

    socket_lib_init();
    gAppStatus = APP_STATUS_FOREGROUND;
    startStreaming();
}

ON_ACQUIRED_FOREGROUND() {
    gAppStatus = APP_STATUS_FOREGROUND;
}

ON_RELEASE_FOREGROUND() {
    gAppStatus = APP_STATUS_BACKGROUND;
}

ON_APPLICATION_ENDS() {
    gAppStatus = APP_STATUS_BACKGROUND;
    EncodingHelper::destroyInstance();
    HeartBeatServer::destroyInstance();
}

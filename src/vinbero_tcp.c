#include <arpa/inet.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vinbero_com/vinbero_com_Call.h>
#include <vinbero_com/vinbero_com_Config.h>
#include <vinbero_com/vinbero_com_Log.h>
#include <vinbero_com/vinbero_com_Module.h>
#include <vinbero_iface_MODULE/vinbero_iface_MODULE.h>
#include <vinbero_iface_BASIC/vinbero_iface_BASIC.h>
#include <libgenc/genc_Tree.h>
#include "config.h"

VINBERO_COM_MODULE_META_NAME("vinbero_tcp")
VINBERO_COM_MODULE_META_LICENSE("MPL-2.0")
VINBERO_COM_MODULE_META_VERSION(
    VINBERO_TCP_VERSION_MAJOR,
    VINBERO_TCP_VERSION_MINOR,
    VINBERO_TCP_VERSION_PATCH
)

VINBERO_COM_MODULE_META_IN_IFACES("BASIC")
VINBERO_COM_MODULE_META_OUT_IFACES("BASIC")
VINBERO_COM_MODULE_META_CHILD_COUNT(-1, -1)

struct vinbero_tcp_LocalModule {
    int socket;
    pthread_mutex_t* socketMutex;
    const char* address;
    int port;
    int backlog;
    bool reuseAddress;
    bool reusePort;
    bool keepAlive;
};

VINBERO_IFACE_MODULE_FUNCS;
VINBERO_IFACE_BASIC_FUNCS;

int vinbero_iface_MODULE_init(struct vinbero_com_Module* module) {
    VINBERO_COM_LOG_TRACE2();
    module->localModule.pointer = malloc(1 * sizeof(struct vinbero_tcp_LocalModule));
    struct vinbero_tcp_LocalModule* localModule = module->localModule.pointer;
    vinbero_com_Config_getConstring(module->config, module, "vinbero_tcp.address", &localModule->address, "0.0.0.0");
    vinbero_com_Config_getInt(module->config, module, "vinbero_tcp.port", &localModule->port, 80);
    vinbero_com_Config_getInt(module->config, module, "vinbero_tcp.backlog", &localModule->backlog, 1024);
    vinbero_com_Config_getBool(module->config, module, "vinbero_tcp.reuseAddress", &localModule->reuseAddress, false);
    vinbero_com_Config_getBool(module->config, module, "vinbero_tcp.reusePort", &localModule->reusePort, false);
    vinbero_com_Config_getBool(module->config, module, "vinbero_tcp.keepAlive", &localModule->keepAlive, false);
    struct sockaddr_in serverAddressSockAddrIn;
    memset(serverAddressSockAddrIn.sin_zero, 0, 1 * sizeof(serverAddressSockAddrIn.sin_zero));
    serverAddressSockAddrIn.sin_family = AF_INET; 
    inet_aton(localModule->address, &serverAddressSockAddrIn.sin_addr);
    serverAddressSockAddrIn.sin_port = htons(localModule->port);
    if((localModule->socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        VINBERO_COM_LOG_ERROR("socket(...) failed");
        return -errno;
    }
    if(setsockopt(localModule->socket, SOL_SOCKET, SO_REUSEADDR, localModule->reuseAddress ? &(const int){1} : &(const int){0}, sizeof(int)) == -1) {
        VINBERO_COM_LOG_ERROR("setsockopt(...) failed");
        return -errno;
    }
    if(setsockopt(localModule->socket, SOL_SOCKET, SO_REUSEPORT, localModule->reusePort ? &(const int){1} : &(const int){0}, sizeof(int)) == -1) {
        VINBERO_COM_LOG_ERROR("setsockopt(...) failed");
        return -errno;
    }
    if(setsockopt(localModule->socket, SOL_SOCKET, SO_KEEPALIVE, localModule->keepAlive ? &(const int){1} : &(const int){0}, sizeof(int)) == -1) {
        VINBERO_COM_LOG_ERROR("setsockopt(...) failed");
        return -errno;
    }
    if(bind(localModule->socket, (struct sockaddr*)&serverAddressSockAddrIn, sizeof(struct sockaddr)) == -1) {
        VINBERO_COM_LOG_ERROR("bind(...) failed");
        return -errno;
    }
    if(listen(localModule->socket, localModule->backlog) == -1) {
        VINBERO_COM_LOG_ERROR("listen(...) failed");
        return -errno;
    }
    localModule->socketMutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(localModule->socketMutex, NULL);
    return 0;
}

int vinbero_iface_MODULE_rInit(struct vinbero_com_Module* module) {
    VINBERO_COM_LOG_TRACE2();
    return 0;
}

int vinbero_iface_BASIC_service(struct vinbero_com_Module* module) {
    int ret;
    struct vinbero_tcp_LocalModule* localModule = module->localModule.pointer;
    GENC_TREE_NODE_FOREACH(module, index) {
        struct vinbero_com_Module* childModule = GENC_TREE_NODE_RAW_GET(module, index);
        childModule->arg = &localModule->socket;
        VINBERO_COM_CALL(BASIC, service, childModule, &ret, childModule);
        if(ret < 0)
            return ret;
    }
    return 0;
}

int vinbero_iface_MODULE_destroy(struct vinbero_com_Module* module) {
    VINBERO_COM_LOG_TRACE2();
    return 0;
}

int vinbero_iface_MODULE_rDestroy(struct vinbero_com_Module* module) {
    VINBERO_COM_LOG_TRACE2();
    struct vinbero_tcp_LocalModule* localModule = module->localModule.pointer;
    pthread_mutex_destroy(localModule->socketMutex);
    free(localModule->socketMutex);
    free(module->localModule.pointer);
    return 0;
}

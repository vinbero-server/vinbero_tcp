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
#include <vinbero_common/vinbero_common_Config.h>
#include <vinbero_common/vinbero_common_Log.h>
#include <vinbero_common/vinbero_common_Module.h>
#include <vinbero/vinbero_IModule.h>
#include <vinbero/vinbero_IBasic.h>
#include <libgenc/genc_Tree.h>

struct vinbero_tcp_LocalModule {
    int socket;
    pthread_mutex_t* socketMutex;
    const char* address;
    short port;
    int backlog;
    bool reuseAddress;
    bool reusePort;
    bool keepAlive;
};

VINBERO_IMODULE_FUNCTIONS;
VINBERO_IBASIC_FUNCTIONS;

int vinbero_IModule_init(struct vinbero_common_Module* module, struct vinbero_common_Config* config, void* args[]) {
    VINBERO_COMMON_LOG_TRACE("in %s(...)", __FUNCTION__);
    int ret;
    module->name = "vinbero_tcp";
    module->version = "0.0.1";
    module->localModule.pointer = malloc(1 * sizeof(struct vinbero_tcp_LocalModule));
    struct vinbero_tcp_LocalModule* localModule = module->localModule.pointer;
    vinbero_common_Config_getString(config, module, "vinbero_tcp.address", &localModule->address, "0.0.0.0");
    vinbero_common_Config_getInt(config, module, "vinbero_tcp.port", &localModule->port, 80);
    vinbero_common_Config_getInt(config, module, "vinbero_tcp.backlog", &localModule->backlog, 1024);
    vinbero_common_Config_getBool(config, module, "vinbero_tcp.reuseAddress", &localModule->reuseAddress, false);
    vinbero_common_Config_getBool(config, module, "vinbero_tcp.reusePort", &localModule->reusePort, false);
    vinbero_common_Config_getBool(config, module, "vinbero_tcp.keepAlive", &localModule->keepAlive, false);
    struct sockaddr_in serverAddressSockAddrIn;
    memset(serverAddressSockAddrIn.sin_zero, 0, 1 * sizeof(serverAddressSockAddrIn.sin_zero));
    serverAddressSockAddrIn.sin_family = AF_INET; 
    inet_aton(localModule->address, &serverAddressSockAddrIn.sin_addr);
    serverAddressSockAddrIn.sin_port = htons(localModule->port);
    if((localModule->socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        VINBERO_COMMON_LOG_ERROR("socket(...) failed");
        return -errno;
    }
    if(setsockopt(localModule->socket, SOL_SOCKET, SO_REUSEADDR, localModule->reuseAddress ? &(const int){1} : &(const int){0}, sizeof(int)) == -1) {
        VINBERO_COMMON_LOG_ERROR("setsockopt(...) failed");
        return -errno;
    }
    if(setsockopt(localModule->socket, SOL_SOCKET, SO_REUSEPORT, localModule->reusePort ? &(const int){1} : &(const int){0}, sizeof(int)) == -1) {
        VINBERO_COMMON_LOG_ERROR("setsockopt(...) failed");
        return -errno;
    }
    if(setsockopt(localModule->socket, SOL_SOCKET, SO_KEEPALIVE, localModule->keepAlive ? &(const int){1} : &(const int){0}, sizeof(int)) == -1) {
        VINBERO_COMMON_LOG_ERROR("setsockopt(...) failed");
        return -errno;
    }
    if(bind(localModule->socket, (struct sockaddr*)&serverAddressSockAddrIn, sizeof(struct sockaddr)) == -1) {
        VINBERO_COMMON_LOG_ERROR("bind(...) failed");
        return -errno;
    }
    if(listen(localModule->socket, localModule->backlog) == -1) {
        VINBERO_COMMON_LOG_ERROR("listen(...) failed");
        return -errno;
    }
    localModule->socketMutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(localModule->socketMutex, NULL);
/*
    struct vinbero_common_Module_Ids childModuleIds;
    GENC_ARRAY_LIST_INIT(&childModuleIds);
    if((ret = vinbero_common_Config_getChildModuleIds(config, module->id, &childModuleIds)) < 0) {
        VINBERO_COMMON_LOG_ERROR("child modules are invalid at module %s", module->id);
        GENC_ARRAY_LIST_FREE(&childModuleIds);
        return ret;
    }
    GENC_TREE_NODE_FOR_EACH_CHILD(module, index) {
        struct vinbero_common_Module* childModule = &GENC_TREE_NODE_GET_CHILD(module, index);
        struct vinbero_IBasic_Interface childInterface;
        VINBERO_IBASIC_DLSYM(&childInterface, &childModule->dlHandle, &ret);
        if(ret < 0) {
            VINBERO_COMMON_LOG_ERROR("module %s doesn't satisfy IBAISC interface", childModule->id);
            GENC_ARRAY_LIST_FREE(&childModuleIds);
            return ret;
        }
    }

//    module->childRequired = true
*/
    return 0;
}

int vinbero_IModule_rInit(struct vinbero_common_Module* module, struct vinbero_common_Config* config, void* args[]) {
    VINBERO_COMMON_LOG_TRACE("in %s(...)", __FUNCTION__);
    return 0;
}

int vinbero_IBasic_service(struct vinbero_common_Module* module, void* args[]) {
    VINBERO_COMMON_LOG_TRACE("in %s(...)", __FUNCTION__);
    int ret;
    struct vinbero_tcp_LocalModule* localModule = module->localModule.pointer;
    struct vinbero_common_Module* parentModule = GENC_TREE_NODE_GET_PARENT(module);
    GENC_TREE_NODE_FOR_EACH_CHILD(module, index) {
        struct vinbero_common_Module* childModule = &GENC_TREE_NODE_GET_CHILD(module, index);
        struct vinbero_IBasic_Interface childInterface;
        VINBERO_IBASIC_DLSYM(&childInterface, &childModule->dlHandle, &ret);
        if(ret < 0) {
            VINBERO_COMMON_LOG_FATAL("VINBERO_IBASIC_DLSYM() failed");
            return ret;
        }
        if(childInterface.vinbero_IBasic_service(childModule, (void*[]){&localModule->socket, NULL}) == -1)
            return -1;
    }
    return 0;
}

int vinbero_IModule_destroy(struct vinbero_common_Module* module) {
    VINBERO_COMMON_LOG_TRACE("in %s(...)", __FUNCTION__);
    return 0;
}

int vinbero_IModule_rDestroy(struct vinbero_common_Module* module) {
    VINBERO_COMMON_LOG_TRACE("in %s(...)", __FUNCTION__);
    struct vinbero_tcp_LocalModule* localModule = module->localModule.pointer;
    pthread_mutex_destroy(localModule->socketMutex);
    free(localModule->socketMutex);
    free(module->localModule.pointer);
//    dlclose(module->dlHandle);
    return 0;
}

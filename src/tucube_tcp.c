#include <arpa/inet.h>
#include <dlfcn.h>
#include <err.h>
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
#include <tucube/tucube_Module.h>
#include <tucube/tucube_IModule.h>
#include <tucube/tucube_IBasic.h>
#include <libgenc/genc_Tree.h>

struct tucube_tcp_Interface {
    TUCUBE_IBASIC_FUNCTION_POINTERS;
};

struct tucube_tcp_LocalModule {
    int socket;
    pthread_mutex_t* socketMutex;
    const char* address;
    short port;
    int backlog;
    bool reuseAddress;
    bool reusePort;
    bool keepAlive;
};

TUCUBE_IMODULE_FUNCTIONS;
TUCUBE_IBASIC_FUNCTIONS;

int tucube_IModule_init(struct tucube_Module* module, struct tucube_Config* config, void* args[]) {
warnx("%s: %u: %s", __FILE__, __LINE__, __FUNCTION__);
    module->localModule.pointer = malloc(1 * sizeof(struct tucube_tcp_LocalModule));

    struct tucube_tcp_LocalModule* localModule = module->localModule.pointer;
    TUCUBE_CONFIG_GET(config, module->id, "tucube_tcp.address", string, &(localModule->address), "0.0.0.0");
    TUCUBE_CONFIG_GET(config, module->id, "tucube_tcp.port", integer, &(localModule->port), 80);
    TUCUBE_CONFIG_GET(config, module->id, "tucube_tcp.backlog", integer, &(localModule->backlog), 1024);
    TUCUBE_CONFIG_GET(config, module->id, "tucube_tcp.reuseAddress", boolean, &(localModule->reuseAddress), false);
    TUCUBE_CONFIG_GET(config, module->id, "tucube_tcp.reusePort", boolean, &(localModule->reusePort), false);
    TUCUBE_CONFIG_GET(config, module->id, "tucube_tcp.keepAlive", boolean, &(localModule->keepAlive), false);

    struct sockaddr_in serverAddressSockAddrIn;
    memset(serverAddressSockAddrIn.sin_zero, 0, 1 * sizeof(serverAddressSockAddrIn.sin_zero));
    serverAddressSockAddrIn.sin_family = AF_INET; 
    inet_aton(localModule->address, &serverAddressSockAddrIn.sin_addr);
    serverAddressSockAddrIn.sin_port = htons(localModule->port);
    if((localModule->socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        warn("%s: %u", __FILE__, __LINE__);
        return -1;
    }
    if(setsockopt(localModule->socket, SOL_SOCKET, SO_REUSEADDR, localModule->reuseAddress ? &(const int){1} : &(const int){0}, sizeof(int)) == -1) {
        warn("%s: %u", __FILE__, __LINE__);
        return -1;
    }
    if(setsockopt(localModule->socket, SOL_SOCKET, SO_REUSEPORT, localModule->reusePort ? &(const int){1} : &(const int){0}, sizeof(int)) == -1) {
        warn("%s: %u", __FILE__, __LINE__);
        return -1;
    }
    if(setsockopt(localModule->socket, SOL_SOCKET, SO_KEEPALIVE, localModule->keepAlive ? &(const int){1} : &(const int){0}, sizeof(int)) == -1) {
        warn("%s: %u", __FILE__, __LINE__);
        return -1;
    }
    if(bind(localModule->socket, (struct sockaddr*)&serverAddressSockAddrIn, sizeof(struct sockaddr)) == -1) {
        warn("%s: %u", __FILE__, __LINE__);
        return -1;
    }
    if(listen(localModule->socket, localModule->backlog) == -1) {
        warn("%s: %u", __FILE__, __LINE__);
        return -1;
    }
    localModule->socketMutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(localModule->socketMutex, NULL);
    struct tucube_Module_Ids childModuleIds;
    GENC_ARRAY_LIST_INIT(&childModuleIds);
    TUCUBE_CONFIG_GET_CHILD_MODULE_IDS(config, module->id, &childModuleIds);
    GENC_TREE_NODE_FOR_EACH_CHILD(module, index) {
        struct tucube_Module* childModule = &GENC_TREE_NODE_GET_CHILD(module, index);
        childModule->interface = malloc(1 * sizeof(struct tucube_tcp_Interface));
        struct tucube_tcp_Interface* moduleInterface = childModule->interface;
        if((moduleInterface->tucube_IBasic_service = dlsym(childModule->dlHandle, "tucube_IBasic_service")) == NULL) {
            warnx("%s: %u: Unable to find tucube_IBasic_service()", __FILE__, __LINE__);
            return -1;
        }
    }
    return 0;
}

int tucube_IBasic_service(struct tucube_Module* module, void* args[]) {
warnx("%s: %u: %s", __FILE__, __LINE__, __FUNCTION__);
    struct tucube_tcp_LocalModule* localModule = module->localModule.pointer;
    struct tucube_Module* parentModule = GENC_TREE_NODE_GET_PARENT(module);
    GENC_TREE_NODE_FOR_EACH_CHILD(module, index) {
        struct tucube_Module* childModule = &GENC_TREE_NODE_GET_CHILD(module, index);
        struct tucube_tcp_Interface* moduleInterface = childModule->interface;
        if(moduleInterface->tucube_IBasic_service(childModule, (void*[]){&localModule->socket, NULL}) == -1)
            return -1;
    }
    return 0;
}

int tucube_IModule_destroy(struct tucube_Module* module) {
warnx("%s: %u: %s", __FILE__, __LINE__, __FUNCTION__);
    struct tucube_tcp_LocalModule* localModule = module->localModule.pointer;
    pthread_mutex_destroy(localModule->socketMutex);
    free(localModule->socketMutex);
    GENC_TREE_NODE_FOR_EACH_CHILD(module, index) {
        struct tucube_Module* childModule = &GENC_TREE_NODE_GET_CHILD(module, index);
        free(childModule->interface);
    }
    free(module->localModule.pointer);
//    dlclose(module->dlHandle);
    return 0;
}

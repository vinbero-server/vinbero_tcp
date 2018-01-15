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
#include <vinbero/vinbero_Module.h>
#include <vinbero/vinbero_IModule.h>
#include <vinbero/vinbero_IBasic.h>
#include <libgenc/genc_Tree.h>

struct vinbero_tcp_Interface {
    VINBERO_IBASIC_FUNCTION_POINTERS;
};

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

int vinbero_IModule_init(struct vinbero_Module* module, struct vinbero_Config* config, void* args[]) {
warnx("%s: %u: %s", __FILE__, __LINE__, __FUNCTION__);
    module->name = "vinbero_tcp";
    module->version = "0.0.1";
    module->localModule.pointer = malloc(1 * sizeof(struct vinbero_tcp_LocalModule));
    struct vinbero_tcp_LocalModule* localModule = module->localModule.pointer;
    VINBERO_CONFIG_GET(config, module, "vinbero_tcp.address", string, &localModule->address, "0.0.0.0");
    VINBERO_CONFIG_GET(config, module, "vinbero_tcp.port", integer, &localModule->port, 80);
    VINBERO_CONFIG_GET(config, module, "vinbero_tcp.backlog", integer, &localModule->backlog, 1024);
    VINBERO_CONFIG_GET(config, module, "vinbero_tcp.reuseAddress", boolean, &localModule->reuseAddress, false);
    VINBERO_CONFIG_GET(config, module, "vinbero_tcp.reusePort", boolean, &localModule->reusePort, false);
    VINBERO_CONFIG_GET(config, module, "vinbero_tcp.keepAlive", boolean, &localModule->keepAlive, false);
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
    struct vinbero_Module_Ids childModuleIds;
    GENC_ARRAY_LIST_INIT(&childModuleIds);
    VINBERO_CONFIG_GET_CHILD_MODULE_IDS(config, module->id, &childModuleIds);
    GENC_TREE_NODE_FOR_EACH_CHILD(module, index) {
        struct vinbero_Module* childModule = &GENC_TREE_NODE_GET_CHILD(module, index);
        childModule->interface = malloc(1 * sizeof(struct vinbero_tcp_Interface));
        struct vinbero_tcp_Interface* childInterface = childModule->interface;
        int errorVariable;
        VINBERO_IBASIC_DLSYM(childInterface, childModule->dlHandle, &errorVariable);
        if(errorVariable == 1) {
            warnx("module %s doesn't satisfy IBAISC interface", childModule->id);
            return -1;
        }
    }
    return 0;
}

int vinbero_IModule_rInit(struct vinbero_Module* module, struct vinbero_Config* config, void* args[]) {
warnx("%s: %u: %s", __FILE__, __LINE__, __FUNCTION__);
    return 0;
}

int vinbero_IBasic_service(struct vinbero_Module* module, void* args[]) {
warnx("%s: %u: %s", __FILE__, __LINE__, __FUNCTION__);
    struct vinbero_tcp_LocalModule* localModule = module->localModule.pointer;
    struct vinbero_Module* parentModule = GENC_TREE_NODE_GET_PARENT(module);
    GENC_TREE_NODE_FOR_EACH_CHILD(module, index) {
        struct vinbero_Module* childModule = &GENC_TREE_NODE_GET_CHILD(module, index);
        struct vinbero_tcp_Interface* childInterface = childModule->interface;
        if(childInterface->vinbero_IBasic_service(childModule, (void*[]){&localModule->socket, NULL}) == -1)
            return -1;
    }
    return 0;
}

int vinbero_IModule_destroy(struct vinbero_Module* module) {
warnx("%s: %u: %s", __FILE__, __LINE__, __FUNCTION__);
    return 0;
}

int vinbero_IModule_rDestroy(struct vinbero_Module* module) {
warnx("%s: %u: %s", __FILE__, __LINE__, __FUNCTION__);
    struct vinbero_tcp_LocalModule* localModule = module->localModule.pointer;
    pthread_mutex_destroy(localModule->socketMutex);
    free(localModule->socketMutex);
    GENC_TREE_NODE_FOR_EACH_CHILD(module, index) {
        struct vinbero_Module* childModule = &GENC_TREE_NODE_GET_CHILD(module, index);
        free(childModule->interface);
    }
    free(module->localModule.pointer);
//    dlclose(module->dlHandle);
    return 0;
}

#include "yk/application.h"
#include "yk/module.h"
#include "chat/myModule.h"
#include <signal.h>


int main(int argc, char** argv) {
    signal(SIGPIPE, SIG_IGN);
    yk::ModuleMgr::GetInstance()->add(yk::Module::ptr(new chat::MyModule));
    yk::Application app;
    if(app.init(argc, argv)) {
        return app.run();
    }
    
    return 0;
}

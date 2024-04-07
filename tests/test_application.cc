#include "yk/application.h"
int main(int argc, char** argv) {
    yk::Application app;
    if(app.init(argc, argv)) {
        return app.run();
    }
    return 0;
}

#include <string>
#include "dump_config.hpp"

class dump{
    public:
        dump(){
            // lowkey not sure what needs to go here
        }

        void cut_delay() {
            // to cut the delay, we need to stop the stream (force stop, don't let it stop gracefully), and then start it again
            // we also need to remove the frame from memory to prevent leaks
        }

        void fill_delay() {
            // to fill the delay, we need to just change the scene/source/color of the frames in the delay to the one specified by the user
            // we keep it on that for the duration of the delay, and then we change it back to the normal stream
        }

    private:
        dump_config config;

};
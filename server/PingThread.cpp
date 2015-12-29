#include "Global.h"
#include "Protocol.h"

#include "Packet.h"
#include "Device.h"
#include "PingThread.h"
#include "packet/PingDevicePacket.h"

void PingThread::run(void* obj)
{
    vector<Device>* devs = (vector<Device> *) obj;
    while (1) {
        sleep(5);
        cout << "Ping Thread!! : " << devs->size() << endl;
        for (vector<Device>::iterator iter = devs->begin(); iter != devs->end(); iter++) {
            PingDevicePacket pp;
            int retval = iter->send(&pp);
            if (retval <= 0) {
                // client disconnected
                vector<Device>::iterator delIter = iter;
                devs->erase(delIter);
            }
        }
    }
}


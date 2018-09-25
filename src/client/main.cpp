#include <iostream>
#include <fstream>

#include <stdint.h>
#include <stdio.h>
#include "tcp.h"
#include "client.h"
#include "schedule.h"
#include "json/json.h"

using namespace std;

int main(int argc, char * argv[])
{
    int32_t fdsock;
    string rsp;
    uint32_t dst_port;
    Json::Value root, config;
    Json::CharReaderBuilder builder;
    Json::CharReader * creader = builder.newCharReader();

    // load argument
    if(argc != 2) {
        cout << "Usage: guard-client configfile" << endl;
        return -1;
    }

    fstream inf;
    string line;
    inf.open(argv[1]);
    if(!inf.is_open()) {
        cout << "can't open file" << endl;
        return 1;
    }

    rsp.clear();
    while(getline(inf, line))
        rsp += line;
    inf.close();

    creader->parse(rsp.c_str(), rsp.c_str() + rsp.size(), &config, NULL);
    cout << config.toStyledString() << endl;

    // connect to server
    fdsock = tcp::Connect(config["ip"].asCString(), (uint16_t)(config["port"].asUInt()));
    if(-1 == fdsock) {
        cout << "Can't not connect to server." << endl;
        return -1;
    }

    root.clear();
    root["cmd"] = "ping";
    tcp::Write(fdsock, root.toStyledString());
    rsp = tcp::Read(fdsock, 1024);
    creader->parse(rsp.c_str(), rsp.c_str() + rsp.size(), &root, NULL);
    if(string("pong") == root["cmd"].asString()) {
        cout << "Connect to " << config["ip"].asString() << ":" << config["port"].asUInt() << endl;

        // create a schedule, to monitor all client
        schedule sch(fdsock);

        // find all rtsp camera, get video frame, and push to server
        Json::Value rtsp_list = config["rtsp"];
        for(int i = 0; i < rtsp_list.size(); i++) {
            cout << rtsp_list[i]["url"].asString() << ":" << rtsp_list[i]["sn"].asString() << endl;

            // require a port from server
            root.clear();
            root["cmd"] = "push";
            root["sn"] = rtsp_list[i]["sn"].asString();
            tcp::Write(fdsock, root.toStyledString());
            rsp = tcp::Read(fdsock, 1024);
            creader->parse(rsp.c_str(), rsp.c_str() + rsp.size(), &root, NULL);
            dst_port = root["port"].asUInt();
            if(dst_port) {
                rsp = config["ip"].asString();
                session * sess = session::create(rsp.c_str(), (uint16_t)dst_port, (uint16_t)(dst_port + 1000));

                rsp = rtsp_list[i]["url"].asString();
                videocap * vc = videocap::create(rsp.c_str(), videocap::url_type_rtsp);
                if(vc)
                    sch.reg(new client(root["sn"].asString(), sess, vc));
            } else {
                cout << "error: " << root["msg"].asString()<< endl;
            }
        }

        sch.run();
        tcp::Close(fdsock);
        return 0;
    } else {
        cout << "connect server fail." << endl;
    }

    return 0;
}


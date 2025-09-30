#include <iostream>
#include <vector>
#include <algorithm>
#include <cstring>
#include <cstdint>
#include <chrono>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

#pragma pack(push,1)
struct MsgHeader {
    uint16_t type;   // 1=HELLO, 2=CHAT, 3=ACK
    uint32_t seq;    // for CHAT and ACK
    uint16_t len;    // payload length (bytes)
};
#pragma pack(pop)

static const uint16_t MSG_HELLO = 1;
static const uint16_t MSG_CHAT  = 2;
static const uint16_t MSG_ACK   = 3;

struct Endpoint {
    sockaddr_in addr;
};
static vector<Endpoint> clients;

static bool same_ep(const sockaddr_in& a, const sockaddr_in& b) {
    return a.sin_addr.s_addr == b.sin_addr.s_addr && a.sin_port == b.sin_port;
}

static void add_client(const sockaddr_in& ep) {
    for (auto &c : clients) if (same_ep(c.addr, ep)) return;
    Endpoint e; e.addr = ep; clients.push_back(e);
    cerr << "Registered client " << inet_ntoa(ep.sin_addr)
         << ":" << ntohs(ep.sin_port) << "\n";
}

int main() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) { perror("socket"); return 1; }

    char portbuf[16] = {0};
    cout << "Please enter UDP listening port (default 5001): ";
    cin.getline(portbuf, sizeof(portbuf));
    int port = (strlen(portbuf) ? atoi(portbuf) : 5001);

    sockaddr_in srv{};
    srv.sin_family = AF_INET;
    srv.sin_addr.s_addr = INADDR_ANY;
    srv.sin_port = htons(port);

    if (bind(sockfd, (sockaddr*)&srv, sizeof(srv)) < 0) {
        perror("bind"); return 1;
    }
    cout << "UDP server listening on " << port << "...\n";

    char buf[2048];
    for (;;) {
        sockaddr_in src{}; socklen_t slen = sizeof(src);
        ssize_t n = recvfrom(sockfd, buf, sizeof(buf), 0, (sockaddr*)&src, &slen);
        if (n <= 0) continue;
        if ((size_t)n < sizeof(MsgHeader)) continue;

        MsgHeader hdr;
        memcpy(&hdr, buf, sizeof(hdr));
        hdr.type = ntohs(hdr.type);
        hdr.seq  = ntohl(hdr.seq);
        hdr.len  = ntohs(hdr.len);

        if (hdr.type == MSG_HELLO) {
            add_client(src);
            // optional: send ACK(seq=0) as a welcome
            MsgHeader ack{htons(MSG_ACK), htonl(0u), htons(0)};
            sendto(sockfd, &ack, sizeof(ack), 0, (sockaddr*)&src, slen);
        } else if (hdr.type == MSG_CHAT) {
            // ACK back to sender (Stop-and-Wait)
            MsgHeader ack{htons(MSG_ACK), htonl(hdr.seq), htons(0)};
            sendto(sockfd, &ack, sizeof(ack), 0, (sockaddr*)&src, slen);

            // Broadcast payload to all known clients except sender
            size_t plen = 0;
            if ((size_t)n > sizeof(MsgHeader))
                plen = min<size_t>(hdr.len, (size_t)n - sizeof(MsgHeader));

            vector<char> out(sizeof(MsgHeader) + plen);
            MsgHeader oh{htons(MSG_CHAT), htonl(hdr.seq), htons((uint16_t)plen)};
            memcpy(out.data(), &oh, sizeof(oh));
            if (plen) memcpy(out.data() + sizeof(MsgHeader), buf + sizeof(MsgHeader), plen);

            for (auto &c : clients) {
                if (same_ep(c.addr, src)) continue;
                sendto(sockfd, out.data(), out.size(), 0,
                       (sockaddr*)&c.addr, sizeof(c.addr));
            }
        }
    }

    close(sockfd);
    return 0;
}

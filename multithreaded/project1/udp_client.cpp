#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#include <atomic>
#include <chrono>
#include <thread>
#include <iomanip>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

#pragma pack(push,1)
struct MsgHeader {
    uint16_t type;   // 1=HELLO, 2=CHAT, 3=ACK
    uint32_t seq;    // for CHAT and ACK
    uint16_t len;    // payload length
};
#pragma pack(pop)

static const uint16_t MSG_HELLO = 1;
static const uint16_t MSG_CHAT  = 2;
static const uint16_t MSG_ACK   = 3;

static int sockfd = -1;
static sockaddr_in server_addr{};
static atomic<uint32_t> last_ack_seq{0};

static void rx_loop() {
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

        if (hdr.type == MSG_ACK) {
            last_ack_seq.store(hdr.seq, memory_order_relaxed);
        } else if (hdr.type == MSG_CHAT) {
            size_t plen = 0;
            if ((size_t)n > sizeof(MsgHeader))
                plen = min<size_t>(hdr.len, (size_t)n - sizeof(MsgHeader));
            string text(buf + sizeof(MsgHeader), buf + sizeof(MsgHeader) + plen);
            auto t = chrono::system_clock::to_time_t(chrono::system_clock::now());
            cout << "[" << put_time(localtime(&t), "%H:%M:%S") << "] " << text << endl;
        }
    }
}

static void send_hello() {
    MsgHeader h{htons(MSG_HELLO), htonl(0u), htons(0)};
    sendto(sockfd, &h, sizeof(h), 0, (sockaddr*)&server_addr, sizeof(server_addr));
}

static bool send_chat_with_arq(uint32_t seq, const string& text,
                               int max_retx = 3, int timeout_ms = 600) {
    // Build packet
    vector<char> pkt(sizeof(MsgHeader) + text.size());
    MsgHeader h{htons(MSG_CHAT), htonl(seq), htons((uint16_t)text.size())};
    memcpy(pkt.data(), &h, sizeof(h));
    memcpy(pkt.data() + sizeof(MsgHeader), text.data(), text.size());

    for (int attempt = 0; attempt <= max_retx; ++attempt) {
        // send
        sendto(sockfd, pkt.data(), pkt.size(), 0, (sockaddr*)&server_addr, sizeof(server_addr));

        // wait for ACK(seq)
        auto start = chrono::steady_clock::now();
        while (chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start).count() < timeout_ms) {
            if (last_ack_seq.load(memory_order_relaxed) == seq) return true;
            this_thread::sleep_for(chrono::milliseconds(20));
        }
        cerr << "Timeout waiting ACK(" << seq << "), retransmitting... attempt " << (attempt+1) << "\n";
    }
    return false;
}

int main() {
    // Create socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) { perror("socket"); return 1; }

    // Ask for server IP/port
    char ip[64]={0}, portbuf[16]={0};
    cout << "Please enter UDP server IP Address: ";
    cin.getline(ip, sizeof(ip));
    cout << "Please enter the UDP server port: ";
    cin.getline(portbuf, sizeof(portbuf));
    int port = atoi(portbuf);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        cout << "Invalid IP\n"; return 1;
    }

    // Start receiver thread
    thread rx(rx_loop);
    rx.detach();

    // Hello registration
    send_hello();
    cout << "Registered with server. Use '/say <text>' to send chat. Type 'Quit' to exit.\n";

    // Main input loop
    uint32_t seq = 1;
    for (;;) {
        string line;
        cout << "> ";
        if (!getline(cin, line)) break;
        if (line == "Quit") break;
        if (line.rfind("/say ", 0) == 0) {
            string text = line.substr(5);
            bool ok = send_chat_with_arq(seq, text);
            if (!ok) cerr << "Failed after retransmissions for seq=" << seq << "\n";
            seq++;
        } else {
            cout << "(hint) use /say <text>\n";
        }
    }

    close(sockfd);
    return 0;
}

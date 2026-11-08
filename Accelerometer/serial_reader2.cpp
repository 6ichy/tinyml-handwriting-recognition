/*
 * serial_reader.cpp  —  Air Writing Edition
 * -------------------------------------------
 * Reads ax,ay,az,gx,gy,gz lines from Arduino over USB serial.
 * Forwards as UDP packets to localhost for the Python canvas.
 *
 * UDP packet format (28 bytes):
 *   float ax, ay, az  (accelerometer in g)
 *   float gx, gy, gz  (gyroscope in dps)
 *   uint32_t seq
 *
 * Compile (Mac/Linux):
 *   g++ -O2 -std=c++17 -o serial_reader serial_reader.cpp
 *
 * Run:
 *   ./serial_reader /dev/cu.usbmodem1101
 */

#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <cstdint>
#include <csignal>
#include <atomic>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef __APPLE__
  #include <sys/ioctl.h>
  #include <IOKit/serial/ioss.h>
#endif

static constexpr int  BAUD_RATE    = 115200;
static constexpr int  DEFAULT_PORT = 5005;
static constexpr char UDP_HOST[]   = "127.0.0.1";

static std::atomic<bool> g_running{true};
void handle_signal(int) { g_running = false; }

// ── UDP packet (28 bytes) ─────────────────────────────────────────────────────
#pragma pack(push, 1)
struct ImuPacket {
    float    ax, ay, az;   // accelerometer (g)
    float    gx, gy, gz;   // gyroscope (dps)
    uint32_t seq;
};
#pragma pack(pop)

// ── Serial ────────────────────────────────────────────────────────────────────
int open_serial(const std::string& port) {
    int fd = open(port.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) { std::cerr << "[ERROR] Cannot open " << port << "\n"; return -1; }

    struct termios tty{};
    tcgetattr(fd, &tty);
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_iflag &= ~IGNBRK;
    tty.c_lflag  = 0;
    tty.c_oflag  = 0;
    tty.c_cc[VMIN]  = 1;
    tty.c_cc[VTIME] = 0;
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(PARENB | PARODD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;
    tcsetattr(fd, TCSANOW, &tty);

    std::cerr << "[INFO] Opened " << port << " @ " << BAUD_RATE << " baud\n";
    return fd;
}

bool read_line(int fd, std::string& out) {
    out.clear();
    char c;
    while (g_running) {
        if (read(fd, &c, 1) <= 0) continue;
        if (c == '\n') return true;
        if (c != '\r') out += c;
    }
    return false;
}

// ── Main ──────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <serial_port> [udp_port]\n";
        return 1;
    }

    std::string port     = argv[1];
    int         udp_port = (argc >= 3) ? std::stoi(argv[2]) : DEFAULT_PORT;

    std::signal(SIGINT,  handle_signal);
    std::signal(SIGTERM, handle_signal);

    int serial = open_serial(port);
    if (serial < 0) return 1;

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in dest{};
    dest.sin_family      = AF_INET;
    dest.sin_port        = htons(udp_port);
    dest.sin_addr.s_addr = inet_addr(UDP_HOST);

    std::cerr << "[INFO] UDP → " << UDP_HOST << ":" << udp_port << "\n";
    std::cerr << "[INFO] Streaming — press Ctrl+C to stop\n";

    uint32_t seq = 0, sent = 0, skipped = 0;
    std::string line;

    while (g_running) {
        if (!read_line(serial, line)) break;
        if (line.empty())   continue;
        if (line[0] == '#') { std::cerr << "[META] " << line << "\n"; continue; }

        // Parse "ax,ay,az,gx,gy,gz"
        float ax, ay, az, gx, gy, gz;
        char  c1, c2, c3, c4, c5;
        std::istringstream ss(line);
        if (!(ss >> ax >> c1 >> ay >> c2 >> az >> c3 >> gx >> c4 >> gy >> c5 >> gz)) {
            skipped++;
            continue;
        }

        ImuPacket pkt{ax, ay, az, gx, gy, gz, seq++};
        sendto(sock, reinterpret_cast<const char*>(&pkt), sizeof(pkt), 0,
               reinterpret_cast<sockaddr*>(&dest), sizeof(dest));
        sent++;

        if (sent % 500 == 0)
            std::cerr << "[INFO] Sent " << sent << " packets  skipped " << skipped << "\n";
    }

    std::cerr << "[INFO] Done. Total sent: " << sent << "\n";
    close(sock);
    close(serial);
    return 0;
}

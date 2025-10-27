#include <thread>
#include <atomic>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <cstring>
#include <cstdio>
#include <chrono>

#include "../../ISerialCommunicator.hpp"
#include "../../Packet.hpp"

template<typename P>
class LinuxSerialCommunicator : public ISerialCommunicator<P>
{
  public:
    explicit LinuxSerialCommunicator(const std::string& devicePath)
        : ISerialCommunicator<P>(devicePath),
          fd_(-1), running_(false), baud_(115200), stopBits_(1),
          parity_('N'), flowCtrl_(false), timeoutMs_(0) {}

    ~LinuxSerialCommunicator() override
    {
        stop();
    }

    bool start() override
    {
        fd_ = ::open(this->devicePath_.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (fd_ < 0)
        {
            std::perror("open");
            return false;
        }
        if (!configurePort())
        {
            ::close(fd_);
            fd_ = -1;
            return false;
        }
        running_ = true;
        reader_ = std::thread([this] { readLoop(); });
        return true;
    }

    void stop() override
    {
        running_ = false;
        if (reader_.joinable()) reader_.join();
        if (fd_ >= 0)
        {
            ::close(fd_);
            fd_ = -1;
        }
    }

    bool isRunning() const override
    {
        return running_;
    }

    void onReceive(std::function<void(const P &)> cb) override
    {
        callback_ = std::move(cb);
    }

    bool operator<<(const P &pkt) override
    {
        return sendPacket(pkt);
    }

    void setBaudRate(uint32_t baud)
    {
        baud_ = baud;
        if (fd_ >= 0) configurePort();
    }
    void setStopBits(uint8_t bits)
    {
        stopBits_ = bits;
        if (fd_ >= 0) configurePort();
    }
    void setParityNone()
    {
        parity_ = 'N';
        if (fd_ >= 0) configurePort();
    }
    void setParityEven()
    {
        parity_ = 'E';
        if (fd_ >= 0) configurePort();
    }
    void setParityOdd()
    {
        parity_ = 'O';
        if (fd_ >= 0) configurePort();
    }
    void setFlowControlNone()
    {
        flowCtrl_ = false;
        if (fd_ >= 0) configurePort();
    }
    void setFlowControlHardware()
    {
        flowCtrl_ = true;
        if (fd_ >= 0) configurePort();
    }
    void setReadTimeout(uint32_t ms)
    {
        timeoutMs_ = ms;
        if (fd_ >= 0) configurePort();
    }

  private:
    int fd_;
    std::atomic<bool> running_;
    std::thread reader_;
    std::function<void(const P &)> callback_;

    uint32_t baud_;
    uint8_t stopBits_;
    char parity_;
    bool flowCtrl_;
    uint32_t timeoutMs_;

    bool sendPacket(const P &pkt)
    {
        std::vector<uint8_t> data = pkt.serialize();
        return writeAll(data.data(), data.size());
    }

    bool writeAll(const uint8_t* buf, size_t len)
    {
        size_t written = 0;
        while (written < len)
        {
            ssize_t r = ::write(fd_, buf + written, len - written);
            if (r < 0)
            {
                if (errno == EINTR) continue;
                std::perror("write"); return false;
            }
            written += r;
        }
        tcdrain(fd_);
        return true;
    }

    bool configurePort()
    {
        struct termios tio {};
        if (tcgetattr(fd_, &tio) != 0)
        {
            std::perror("tcgetattr");
            return false;
        }
        cfmakeraw(&tio);
        speed_t spd = B115200;
        switch (baud_)
        {
            case 9600: spd = B9600; break;
            case 19200: spd = B19200; break;
            case 38400: spd = B38400; break;
            case 57600: spd = B57600; break;
            case 115200: spd = B115200; break;
        }
        cfsetispeed(&tio, spd);
        cfsetospeed(&tio, spd);
        tio.c_cflag &= ~CSTOPB;
        if (stopBits_ == 2) tio.c_cflag |= CSTOPB;
        tio.c_cflag &= ~(PARENB | PARODD);
        if (parity_ == 'E') tio.c_cflag |= PARENB;
        else if (parity_ == 'O') tio.c_cflag |= PARENB | PARODD;
        if (flowCtrl_) tio.c_cflag |= CRTSCTS;
        else tio.c_cflag &= ~CRTSCTS;
        if (timeoutMs_ == 0)
        {
            tio.c_cc[VMIN] = 1;
            tio.c_cc[VTIME] = 0;
        }
        else
        {
            tio.c_cc[VMIN] = 0;
            tio.c_cc[VTIME] = timeoutMs_ / 100;
        }
        tio.c_cflag |= CLOCAL | CREAD;
        if (tcsetattr(fd_, TCSANOW, &tio) != 0)
        {
            std::perror("tcsetattr");
            return false;
        }
        tcflush(fd_, TCIOFLUSH);
        int status;
        if (ioctl(fd_, TIOCMGET, &status) == 0)
        {
            status |= TIOCM_DTR | TIOCM_RTS;
            ioctl(fd_, TIOCMSET, &status);
        }
        return true;
    }

    void readLoop()
    {
        constexpr size_t BUF = 1024;
        uint8_t tmp[BUF];
        std::vector<uint8_t> buffer;
        while (running_)
        {
            ssize_t r = ::read(fd_, tmp, BUF);
            if (r > 0)
            {
                buffer.insert(buffer.end(), tmp, tmp + r);
                size_t consumed = 0;
                P pkt;
                while (Packet::deserialize(buffer.data(), buffer.size(), pkt, consumed))
                {
                    if (callback_) callback_(pkt);
                    buffer.erase(buffer.begin(), buffer.begin() + consumed);
                }
            }
            else if (r == 0 || (r == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)))
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            else if (r == -1)
            {
                std::perror("read"); break;
            }
        }
    }
};

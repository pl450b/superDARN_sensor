#include <iostream>
#include <fstream>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <cstring>
#include <cstdlib>

// File descriptor for the serial port
int serial_fd = -1;
std::string error_file_path = "/var/log/superDARN.error"

// Function to close the serial port and clean up
void cleanup() {
    if (serial_fd != -1) {
        close(serial_fd);
        serial_fd = -1;
    }
}

// Signal handler for graceful shutdown
void signalHandler(int signum) {
    std::cerr << "Caught signal " << signum << ", exiting..." << std::endl;
    cleanup();
    exit(signum);
}

// Daemonize the process
void daemonize() {
    pid_t pid = fork();

    if (pid < 0) {
        perror("Failed to fork");
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        // Parent process exits
        std::cout << "Forked success! PID: " << pid << std::endl;
        exit(EXIT_SUCCESS);
    }

    // Child process continues
    if (setsid() < 0) {
        perror("Failed to create a new session");
        exit(EXIT_FAILURE);
    }

    // Redirect standard file descriptors to /dev/null
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    open("/dev/null", O_RDWR);
    dup(0);
    dup(0);
}

// Configure the serial port
int configureSerialPort(const char *device) {
    int fd = open(device, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        perror("Failed to open serial port");
        return -1;
    }

    struct termios tty;
    memset(&tty, 0, sizeof tty);
    if (tcgetattr(fd, &tty) != 0) {
        perror("Failed to get terminal attributes");
        close(fd);
        return -1;
    }

    // Configure the serial port
    cfsetospeed(&tty, B9600); // Output speed: 9600 baud
    cfsetispeed(&tty, B9600); // Input speed: 9600 baud

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8-bit chars
    tty.c_iflag &= ~IGNBRK;                     // Disable break processing
    tty.c_lflag = 0;                            // No signaling chars, no echo, no canonical processing
    tty.c_oflag = 0;                            // No remapping, no delays
    tty.c_cc[VMIN] = 1;                         // Read blocks until at least 1 char is available
    tty.c_cc[VTIME] = 1;                        // Timeout for read: 0.1 seconds

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Shut off xon/xoff control

    tty.c_cflag |= (CLOCAL | CREAD);    // Ignore modem controls, enable reading
    tty.c_cflag &= ~(PARENB | PARODD);  // No parity
    tty.c_cflag &= ~CSTOPB;             // 1 stop bit
    tty.c_cflag &= ~CRTSCTS;            // No hardware flow control

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        perror("Failed to set terminal attributes");
        close(fd);
        return -1;
    }

    return fd;
}

// Main program logic
void run(const char *device) {
    // Configure the serial port
    serial_fd = configureSerialPort(device);
    if (serial_fd < 0) {
        exit(EXIT_FAILURE);
    }

    char buffer[256];
    while (true) {
        // Read data from the serial port
        int n = read(serial_fd, buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            std::cout << "Received: " << buffer << std::endl;
        } else if (n < 0) {
            perror("Failed to read from serial port");
        }
        usleep(100000); // Small delay to prevent excessive CPU usage
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <serial-port> <log_file>" << std::endl;
        return EXIT_FAILURE;
    }

    std::ofstream logFile, errorFile;

    const char *serialPort = argv[1];

    // Register signal handler
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Daemonize the process
    daemonize();

    // Run the main logic
    run(serialPort);

    cleanup();
    return EXIT_SUCCESS;
}


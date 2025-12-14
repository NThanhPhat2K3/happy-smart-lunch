#include "scanner_device.h"
#include <QDebug>
#include <fcntl.h>
#include <unistd.h>

const std::array<char, 256> KEYMAP = []{
    std::array<char, 256> m = {};
    m[30] = '1'; m[31] = '2'; m[32] = '3';
    m[33] = '4'; m[34] = '5'; m[35] = '6';
    m[36] = '7'; m[37] = '8'; m[38] = '9';
    m[39] = '0'; m[40] = '\0';
    return m;
}();


scanner_device::scanner_device(const QString &path, scanner_device_type_t type, QObject *parent)
    : QObject(parent)
{
    this->scanner.scanner_fd    = DEFAULT_FD;
    this->scanner.scanner_path  = path;
    this->scanner.scanner_type  = type;
    scanner.scanner_notifier    = nullptr;
    this->scanner.scanner_buffer.scanner_code_index = 0;
    this->scanner.scanner_buffer.scanner_code_buffer_size   = MAX_CODE_LENGTH;
    memset(this->scanner.scanner_buffer.scanner_code_buffer, 0, sizeof(this->scanner.scanner_buffer.scanner_code_buffer));
}

bool scanner_device::scanner_open()
{
    this->scanner.scanner_fd = open(this->scanner.scanner_path.toStdString().c_str(), O_RDONLY | O_NONBLOCK);
    if (this->scanner.scanner_fd < 0)
    {
        qDebug() << "device open failed:" << this->scanner.scanner_path;
        return false;
    }

    qDebug() << (this->scanner.scanner_type == DEVICE_RFID ? "RFID" : "Barcode")
             << "device opened:" << this->scanner.scanner_path;

    this->scanner.scanner_notifier = new QSocketNotifier(this->scanner.scanner_fd, QSocketNotifier::Read, this);
    connect(this->scanner.scanner_notifier, &QSocketNotifier::activated,
            this, &scanner_device::scanner_handler_data);

    return true;
}

void scanner_device::scanner_close()
{
    if(this->scanner.scanner_fd >= 0)
    {
        close(this->scanner.scanner_fd);
        this->scanner.scanner_fd = -1;
    }
    else
    {
        /* do notthing */
    }
    if (this->scanner.scanner_notifier) {
        this->scanner.scanner_notifier->setEnabled(false);
        delete this->scanner.scanner_notifier;
        this->scanner.scanner_notifier = nullptr;
    }
    else
    {
        /* do notthing */
    }
}
void scanner_device::scanner_process_data(unsigned char *p_buffer, ssize_t bytes)
{
    for(int i = 0 ;i< bytes;i++)
    {
        if(HID_KEYCODE_NONE != p_buffer[i])
        {
            unsigned char keycode = p_buffer[i];
            if(HID_KEYCODE_ENTER == keycode)
            {
                scanner_process_keycode_enter();
                return;
            }
            else
            {
                scanner_process_keycode_mapping(keycode);
            }
        }
    }
}
void scanner_device::scanner_reset_code_buffer(char *p_scanner_reset_code_buffer, size_t buffer_size ,size_t *p_index)
{
    memset(p_scanner_reset_code_buffer, 0, buffer_size);
    *(p_index) = 0;
}
void scanner_device::scanner_handler_data()
{
    unsigned char report[HID_REPORT_SIZE];
    ssize_t bytes = read(this->scanner.scanner_fd,report,sizeof(report));
    if (bytes < 0)
    {
        if (EAGAIN == errno|| EWOULDBLOCK == errno) {
            return;
        }
        qDebug() << "read error on"
                 << (this->scanner.scanner_type == DEVICE_RFID ? "RFID" : "Barcode")
                 << "-" << strerror(errno);
        return;
    }
    else
    {
        scanner_process_data(report,bytes);
    }
}
void scanner_device::scanner_process_keycode_enter()
{
    if(this->scanner.scanner_buffer.scanner_code_index > 0)
    {
        qDebug() <<"scan complete ";
        qDebug() << "data: " << this->scanner.scanner_buffer.scanner_code_buffer;
        emit scanner_received_data(this->scanner.scanner_buffer.scanner_code_buffer);
        scanner_reset_code_buffer(this->scanner.scanner_buffer.scanner_code_buffer, sizeof(this->scanner.scanner_buffer.scanner_code_buffer),&(this->scanner.scanner_buffer.scanner_code_index));
    }
    else
    {
        /* do notthing */
    }
}
void scanner_device::scanner_process_keycode_mapping(unsigned char keycode)
{
    char mapped = KEYMAP[keycode];
    if (mapped) {
        if(this->scanner.scanner_buffer.scanner_code_index < MAX_CODE_LENGTH - 1)
        {
            this->scanner.scanner_buffer.scanner_code_buffer[this->scanner.scanner_buffer.scanner_code_index] = mapped;
            this->scanner.scanner_buffer.scanner_code_index = this->scanner.scanner_buffer.scanner_code_index + 1;
        }
        else
        {
            qDebug () << (this->scanner.scanner_type == DEVICE_RFID ? "RFID" : "Barcode ")<< "buffer overflow detected";
            scanner_reset_code_buffer(this->scanner.scanner_buffer.scanner_code_buffer, sizeof(this->scanner.scanner_buffer.scanner_code_buffer),&(this->scanner.scanner_buffer.scanner_code_index));
        }
    }
}

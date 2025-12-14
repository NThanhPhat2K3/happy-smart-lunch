    #ifndef SCANNER_DEVICE_H
#define SCANNER_DEVICE_H

#include <QObject>
#include <QSocketNotifier>
#include <QString>

#define MAX_CODE_LENGTH     (128U)
#define HID_REPORT_SIZE     (8U)       /* HID device report packet size */
#define NUM_DEVICES         (2U)
#define DEFAULT_FD          (-1)

#define HID_KEYCODE_NONE    (0x00)    /* No key pressed */
#define HID_KEYCODE_ENTER   (40U)      /* Enter key in HID */
#define MAX_VALID_INDEX     (MAX_CODE_LENGTH - 1)

typedef enum
{
    DEVICE_RFID,    /* RFID card reader device */
    DEVICE_BARCODE  /* Barcode scanner device */
} scanner_device_type_t;

struct scanner_buffer_t
{
    size_t  scanner_code_index;
    size_t  scanner_code_buffer_size;
    char    scanner_code_buffer[MAX_CODE_LENGTH];
};

struct scanner_t
{
    int     scanner_fd;
    QString scanner_path;
    QSocketNotifier *scanner_notifier;
    scanner_device_type_t scanner_type;
    struct  scanner_buffer_t scanner_buffer;
};

class scanner_device : public QObject
{
    Q_OBJECT
public:
    explicit scanner_device(const QString &path, scanner_device_type_t type, QObject *parent = nullptr);

    bool scanner_open();
    void scanner_close();
signals:
    void scanner_received_data(QString data);
private slots:
    void scanner_handler_data();

private:
    /* initialize properties */
    struct scanner_t scanner;

    /* initialize methods */
    void scanner_reset_code_buffer(char * p_scanner_reset_code_buffer, size_t buffer_size,size_t *p_index);
    void scanner_process_data(unsigned char *p_buffer, ssize_t bytes);
    void scanner_process_keycode_enter();
    void scanner_process_keycode_mapping(unsigned char keycode);

};

#endif // SCANNER_DEVICE_H

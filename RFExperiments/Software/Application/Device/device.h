#ifndef DEVICE_H
#define DEVICE_H

#include "../../../Software/VNA_embedded/Application/Communication/Protocol.hpp"
#include <functional>
#include <libusb-1.0/libusb.h>
#include <thread>
#include <QObject>
#include <condition_variable>

Q_DECLARE_METATYPE(Protocol::Datapoint);
Q_DECLARE_METATYPE(Protocol::ManualStatus);
Q_DECLARE_METATYPE(Protocol::DeviceInfo);

class USBInBuffer : public QObject {
    Q_OBJECT;
public:
    USBInBuffer(libusb_device_handle *handle, unsigned char endpoint, int buffer_size);
    ~USBInBuffer();

    void removeBytes(int handled_bytes);
    int getReceived() const;
    uint8_t *getBuffer() const;

signals:
    void DataReceived();
    void TransferError();

private:
    void Callback(libusb_transfer *transfer);
    static void LIBUSB_CALL CallbackTrampoline(libusb_transfer *transfer);
    libusb_transfer *transfer;
    unsigned char *buffer;
    int buffer_size;
    int received_size;
    bool inCallback;
    std::condition_variable cv;
};


class Device : public QObject
{
    Q_OBJECT
public:
    // connect to a VNA device. If serial is specified only connecting to this device, otherwise to the first one found
    Device(QString serial = QString());
    ~Device();
    bool Configure(Protocol::SweepSettings settings);
    bool SetManual(Protocol::ManualControl manual);
    // Returns serial numbers of all connected devices
    static std::vector<QString> GetDevices();
    QString serial() const;
    Protocol::DeviceInfo getLastInfo() const;
    QString getLastDeviceInfoString();

signals:
    void DatapointReceived(Protocol::Datapoint);
    void ManualStatusReceived(Protocol::ManualStatus);
    void DeviceInfoUpdated();
    void ConnectionLost();
    void LogLineReceived(QString line);
private slots:
    void ReceivedData();
    void ReceivedLog();

private:
    static constexpr int VID = 0x0483;
    static constexpr int PID = 0x564e;
    static constexpr int EP_Data_Out_Addr = 0x01;
    static constexpr int EP_Data_In_Addr = 0x81;
    static constexpr int EP_Log_In_Addr = 0x82;

    void USBHandleThread();
    // foundCallback is called for every device that is found. If it returns true the search continues, otherwise it is aborted.
    // When the search is aborted the last found device is still opened
    static void SearchDevices(std::function<bool(libusb_device_handle *handle, QString serial)> foundCallback, libusb_context *context);

    libusb_device_handle *m_handle;
    libusb_context *m_context;
    USBInBuffer *dataBuffer;
    USBInBuffer *logBuffer;


    QString m_serial;
    bool m_connected;
    std::thread *m_receiveThread;
    Protocol::DeviceInfo lastInfo;
    bool lastInfoValid;
};

#endif // DEVICE_H

#pragma once

#include <libusb.h>

#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

// 字符串描述符的最大长度，该值来自最新版的libusb代码中定义的LIBUSB_DEVICE_STRING_BYTES_MAX的值
constexpr uint16_t kMaxDescLength = 384;

struct UsbInfo {
    std::string version;
    std::string clazz;
    std::string sub_clazz;
    uint16_t pid;
    uint16_t vid;
    uint8_t bus_number;
    uint8_t address;
    uint8_t config_num;
    std::string serial_number;
    std::string manufacturer;
    std::string product;
};

class UsbTool {
   public:
    std::vector<UsbInfo> Search();

   private:
    std::string GetStringDesc(libusb_device_handle* dev_handle, const uint8_t desc_index);
    void GetConfigDesc(libusb_device* dev, const uint8_t config_num);
    void GetInterfaceInfo(const libusb_config_descriptor* config_desc);
    void GetEndpointInfo(const libusb_interface_descriptor* interface_desc);
    std::string ToVersion(const uint16_t bcdUsb);
    std::tuple<std::string, std::string> GetDeviceClass(const uint8_t device_class, const uint8_t device_sub_class);
};
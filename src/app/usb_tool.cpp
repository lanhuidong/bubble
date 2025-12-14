#include "usb_tool.h"

#include <libusb.h>
#include <spdlog/spdlog.h>

#include <array>
#include <memory>

using m_libusb_context_ptr = std::unique_ptr<libusb_context, decltype(&libusb_exit)>;
using m_libusb_device_handle_ptr = std::unique_ptr<libusb_device_handle, decltype(&libusb_close)>;
using m_libusb_config_desc_ptr = std::unique_ptr<libusb_config_descriptor, decltype(&libusb_free_config_descriptor)>;

std::vector<UsbInfo> UsbTool::Search() {
    std::vector<UsbInfo> infos;
    libusb_context* ctx{nullptr};
    int ret = libusb_init_context(&ctx, NULL, 0);
    if (ret != LIBUSB_SUCCESS) {
        SPDLOG_WARN("libusb_init error, code {} message {}", ret, libusb_strerror(ret));
        return infos;
    }

    m_libusb_context_ptr context{ctx, &libusb_exit};

    libusb_device** devices;
    auto count = libusb_get_device_list(context.get(), &devices);
    if (count < 0) {
        SPDLOG_WARN("libusb_get_device_list error, code {} message {}", count, libusb_strerror(count));
        libusb_exit(ctx);
        return infos;
    }
    infos.reserve(count);
    SPDLOG_INFO("{} usb device found", count);
    for (ssize_t i = 0; i < count; ++i) {
        libusb_device* device = devices[i];
        libusb_device_descriptor desc;
        ret = libusb_get_device_descriptor(device, &desc);
        if (ret < 0) {
            SPDLOG_WARN("libusb_get_device_descriptor, error, code {} message {}", ret, libusb_strerror(ret));
            continue;
        }
        UsbInfo info;
        info.vid = desc.idVendor;
        info.pid = desc.idProduct;
        info.config_num = desc.bNumConfigurations;
        info.version = ToVersion(desc.bcdUSB);
        auto clazz = GetDeviceClass(desc.bDeviceClass, desc.bDeviceSubClass);
        SPDLOG_INFO("class {}, {}", desc.bDeviceClass, desc.bDeviceSubClass);
        info.clazz = std::get<0>(clazz);
        info.sub_clazz = std::get<1>(clazz);

        GetConfigDesc(device, info.config_num);

        libusb_device_handle* device_handle;
        ret = libusb_open(device, &device_handle);
        if (ret < 0) {
            SPDLOG_WARN("libusb_open, error, code {} message {}", ret, libusb_strerror(ret));
            continue;
        }
        m_libusb_device_handle_ptr dev_handle{device_handle, &libusb_close};

        info.serial_number = GetStringDesc(device_handle, desc.iSerialNumber);
        info.manufacturer = GetStringDesc(device_handle, desc.iManufacturer);
        info.product = GetStringDesc(device_handle, desc.iProduct);

        info.bus_number = libusb_get_bus_number(device);
        info.address = libusb_get_device_address(device);

        SPDLOG_INFO("usb info {}, {}, {},{}, {}, {},{}, {}, {}, {}, {}", info.serial_number, info.manufacturer,
                    info.product, info.config_num, info.pid, info.vid, info.bus_number, info.address, info.version,
                    info.clazz, info.sub_clazz);
        infos.push_back(info);
    }
    return infos;
}

std::string UsbTool::GetStringDesc(libusb_device_handle* dev_handle, const uint8_t desc_index) {
    std::array<unsigned char, kMaxDescLength> buf;
    int ret = libusb_get_string_descriptor_ascii(dev_handle, desc_index, buf.data(), kMaxDescLength);
    if (ret < 0) {
        SPDLOG_WARN("libusb_get_string_descriptor_ascii, error, code {} message {}", ret, libusb_strerror(ret));
        return "";
    }
    return std::string{reinterpret_cast<char*>(buf.data()), static_cast<size_t>(ret)};
}

void UsbTool::GetConfigDesc(libusb_device* dev, const uint8_t config_num) {
    for (uint8_t i = 0; i < config_num; ++i) {
        libusb_config_descriptor* config_desc{nullptr};
        int ret = libusb_get_config_descriptor(dev, i, &config_desc);
        if (ret < 0) {
            SPDLOG_WARN("libusb_get_config_descriptor, error, code {} message {}", ret, libusb_strerror(ret));
            continue;
        }
        GetInterfaceInfo(config_desc);
    }
}

void UsbTool::GetInterfaceInfo(const libusb_config_descriptor* config_desc) {
    uint8_t interface_num = config_desc->bNumInterfaces;
    SPDLOG_INFO("interface number is {}", interface_num);
    for (uint8_t i = 0; i < interface_num; ++i) {
        auto iface = config_desc->interface[i];
        for (int j = 0; j < iface.num_altsetting; ++j) {
            GetEndpointInfo(&iface.altsetting[j]);
        }
    }
}

void UsbTool::GetEndpointInfo(const libusb_interface_descriptor* interface_desc) {
    uint8_t endpoint_num = interface_desc->bNumEndpoints;
    for (uint8_t i = 0; i < endpoint_num; ++i) {
        auto endpoint_desc = interface_desc->endpoint[i];
        SPDLOG_INFO("endpoint address {}", endpoint_desc.bEndpointAddress);
    }
}

std::string UsbTool::ToVersion(const uint16_t bcdUsb) {
    /**
     * bcdUsb的格式为0xAABC, AA: 主版本号，B: 次版本号，C: 子次版本号，
     * 例如: 0x0200, 则为USB 2.0
     * 平时好像很少看到有显示子子版本号的，因此这里先忽略子次版本号
     */
    return fmt::format("USB {:x}.{:x}", bcdUsb >> 8, (bcdUsb >> 1) & 0xF);
}

std::tuple<std::string, std::string> UsbTool::GetDeviceClass(const uint8_t device_class,
                                                             const uint8_t device_sub_class) {
    std::string clazz;
    std::string sub_clazz;
    if (device_class == 0x01) {
        clazz = "音频设备";
        if (device_sub_class == 0x01) {
            sub_clazz = "音频控制接口";
        } else if (device_sub_class == 0x02) {
            sub_clazz = "音频流接口";
        } else if (device_sub_class == 0x03) {
            sub_clazz = "MIDI流接口";
        }
    } else if (device_class == 0x02) {
        clazz = "通信设备";
        if (device_sub_class == 0x01) {
            sub_clazz = "直接线控模型";
        } else if (device_sub_class == 0x02) {
            sub_clazz = "抽象控制模型";
        } else if (device_sub_class == 0x03) {
            sub_clazz = "电话控制模型";
        }
    } else if (device_class == 0x03) {
        clazz = "人机接口";
        if (device_sub_class == 0x01) {
            sub_clazz = "引导接口";
        }
    } else if (device_class == 0x05) {
        clazz = "物理设备";
    } else if (device_class == 0x06) {
        clazz = "图像设备";
    } else if (device_class == 0x07) {
        clazz = "打印机";
    } else if (device_class == 0x08) {
        clazz = "大容量存储设备";
        if (device_sub_class == 0x01) {
            sub_clazz = "RBC";
        } else if (device_sub_class == 0x02) {
            sub_clazz = "ATAPI(CD/DVD等)";
        } else if (device_sub_class == 0x03) {
            sub_clazz = "QIC-157(磁带)";
        } else if (device_sub_class == 0x04) {
            sub_clazz = "UFI(软盘)";
        } else if (device_sub_class == 0x05) {
            sub_clazz = "8070i(可移动介质)";
        } else if (device_sub_class == 0x06) {
            sub_clazz = "SCSI(如U盘)";
        }
    } else if (device_class == 0x09) {
        clazz = "集线器";
        if (device_sub_class == 0x00) {
            sub_clazz = "普通集线器";
        } else if (device_sub_class == 0x01) {
            sub_clazz = "智能集线器";
        }
    } else if (device_class == 0x0A) {
        clazz = "CDC数据设备";
    } else if (device_class == 0x0B) {
        clazz = "智能卡";
    } else if (device_class == 0x0D) {
        clazz = "内容安全设备";
    } else if (device_class == 0x0E) {
        clazz = "视频设备";
    } else if (device_class == 0x0F) {
        clazz = "个人医疗设备";
    } else if (device_class == 0x10) {
        clazz = "音频/视频设备";
    } else if (device_class == 0xDC) {
        clazz = "诊断设备";
    } else if (device_class == 0xE0) {
        clazz = "无线控制器";
        if (device_sub_class == 0x01) {
            sub_clazz = "RF控制器(如蓝牙)";
        } else if (device_sub_class == 0x02) {
            sub_clazz = "Wi-Fi";
        } else if (device_sub_class == 0x03) {
            sub_clazz = "超宽带";
        } else if (device_sub_class == 0x04) {
            sub_clazz = "蓝牙AMP";
        }
    }
    return std::make_tuple<>(clazz, sub_clazz);
}
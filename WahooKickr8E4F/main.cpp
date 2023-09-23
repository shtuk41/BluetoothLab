#include "pch.h"

#include <iostream>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Devices.Enumeration.h>
#include <winrt/Windows.Devices.Bluetooth.h>
#include <winrt/Windows.Devices.Bluetooth.GenericAttributeProfile.h>
#include <winrt/Windows.Devices.Bluetooth.Advertisement.h>
#include <winrt/Windows.Storage.Streams.h>

#include <thread>


using namespace winrt;
using namespace Windows::Foundation;


using namespace winrt::Windows::Devices::Bluetooth::GenericAttributeProfile;
using namespace winrt::Windows::Devices::Enumeration;
using namespace winrt::Windows::Devices::Bluetooth;
using namespace winrt::Windows::Devices::Bluetooth::Advertisement;
using namespace winrt::Windows::Storage::Streams;

void printUuid(const winrt::guid&g)
{
    std::cout << "[" << std::hex << g.Data1 << '-';
    std::cout << g.Data2 << '-';
    std::cout << g.Data3 << '-';
    std::cout << g.Data4 << "]" << std::endl;
}


IAsyncAction BLOperationAsync(DeviceInformation device, GattDeviceService& service_ref, GattCharacteristic& myCharacteristic)
{
    auto bluetoothLeDevice = co_await BluetoothLEDevice::FromIdAsync(device.Id());

    std::cout << "Trying to pair" << std::endl;

    if (bluetoothLeDevice)
    {
        GattDeviceServicesResult result = co_await bluetoothLeDevice.GetGattServicesAsync();

        if (result.Status() == GattCommunicationStatus::Success)
        {
            std::cout << "Paired" << std::endl;

            auto services = result.Services();
            int count = 0;
            for (auto service : services)
            {
                std::cout << "Service" << count << std::endl;
               
                printUuid(service.Uuid());
                if (service.Uuid().Data1 == 0x1826)
                {
                    service_ref = service;
                    GattCharacteristicsResult charactiristicResult = co_await service.GetCharacteristicsAsync();

                    if (charactiristicResult.Status() == GattCommunicationStatus::Success)
                    {
                        auto characteristics = charactiristicResult.Characteristics();

                        for (auto characteristic : characteristics)
                        {
                            std::cout << "___________________" << std::endl;
                            GattCharacteristicProperties properties = characteristic.CharacteristicProperties();

                            if (characteristic.Uuid().Data1 == 0x2ad9)
                            {

                                printUuid(characteristic.Uuid());

                                if ((properties & GattCharacteristicProperties::Notify) == GattCharacteristicProperties::Notify)
                                {
                                    std::cout << "Notify property found" << std::endl;
                                }
                                else if ((properties & GattCharacteristicProperties::Write) == GattCharacteristicProperties::Write)
                                {
                                    std::cout << "Write property found" << std::endl;
                                }
                                else if ((properties & GattCharacteristicProperties::Read) == GattCharacteristicProperties::Read)
                                {
                                    std::cout << "Read property found" << std::endl;
                                }
                                else if ((properties & GattCharacteristicProperties::WriteWithoutResponse) == GattCharacteristicProperties::WriteWithoutResponse)
                                {
                                    std::cout << "WriteWithoutResponse property found" << std::endl;
                                }

                                auto writer = DataWriter();
                                writer.WriteByte(0x00);

                                GattCommunicationStatus resultWrite = co_await characteristic.WriteValueAsync(writer.DetachBuffer());

                                if (resultWrite == GattCommunicationStatus::Success)
                                {
                                    std::cout << "wrote 0x00\n";

                                    writer.WriteByte(0x01);
                                    resultWrite = co_await characteristic.WriteValueAsync(writer.DetachBuffer());

                                    if (resultWrite == GattCommunicationStatus::Success)
                                    {
                                        std::cout << "wrote 0x01\n";

                                        writer.WriteByte(0x01);
                                        resultWrite = co_await characteristic.WriteValueAsync(writer.DetachBuffer());

                                        if (resultWrite == GattCommunicationStatus::Success)
                                        {
                                            std::cout << "wrote 0x01\n";

                                            writer.WriteByte(0x04);
                                            writer.WriteByte(0);
                                            resultWrite = co_await characteristic.WriteValueAsync(writer.DetachBuffer());

                                            if (resultWrite == GattCommunicationStatus::Success)
                                            {
                                                std::cout << "wrote resistance 0 \n";

                                            }
                                        }
                                    }


                                }




                            }
                        }
                    }

                    std::cout << "0x1826 characteristics discovery done" << std::endl;
                }
                count++;
            }
        }
    }
}

void function(GattCharacteristic sender, GattValueChangedEventArgs args)
{
    IBuffer buffer = args.CharacteristicValue();

    std::vector<uint8_t> data(buffer.Length());
    Windows::Storage::Streams::DataReader::FromBuffer(buffer).ReadBytes(data);

    wprintf(L"Value changed: ");
    for (uint8_t byte : data)
    {
        wprintf(L"%02d ", byte);
    }
    wprintf(L"\n");


}

int main()
{
    init_apartment();
    Uri uri(L"http://aka.ms/cppwinrt");
    printf("Hello, %ls!\n", uri.AbsoluteUri().c_str());

    DeviceInformation device = nullptr;


    auto requestedProperties = winrt::single_threaded_vector<winrt::hstring>();
    requestedProperties.Append(L"System.Devices.DeviceInstanceId");

    DeviceWatcher deviceWatcher = DeviceInformation::CreateWatcher(
        BluetoothLEDevice::GetDeviceSelectorFromPairingState(false),
        requestedProperties,
        DeviceInformationKind::AssociationEndpoint);

    // Add event handlers for when devices are added, updated, or removed
    deviceWatcher.Added([&](DeviceWatcher sender, DeviceInformation args)
        {
            printf("Hello, %ls!\n", args.Name().c_str());
            if (args.Name() == L"Wahoo KICKR 8E4F")
            {
                device = args;
            }
        });

    deviceWatcher.Updated([](DeviceWatcher sender, DeviceInformationUpdate args)
        {
            // Handle device updated
        });

    deviceWatcher.Removed([](DeviceWatcher sender, DeviceInformationUpdate args)
        {
            // Handle device removed
            std::cout << "REMOVED" << std::endl;
        });

    GattCharacteristic chr = GattCharacteristic(nullptr);
    GattDeviceService service = GattDeviceService(nullptr);

    deviceWatcher.Start();

    while (true)
    {
        if (device == nullptr)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        else
        {
            std::cout << "Press any key to pair" << std::endl;
            wchar_t key = _getwch();



            BLOperationAsync(device, service, chr).get();

            //chr.ValueChanged(function);

            std::cout << "Press any key to disconnect" << std::endl;
            key = _getwch();

            //chr.ValueChanged(function);

            service.Close();

            break;
        }
    }
}







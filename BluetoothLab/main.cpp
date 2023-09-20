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


IAsyncAction BLOperationAsync(DeviceInformation device, GattCharacteristic& myCharacteristic)
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
                if (service.Uuid().Data1 == 0x180d)
                {
                    GattCharacteristicsResult charactiristicResult = co_await service.GetCharacteristicsAsync();

                    if (charactiristicResult.Status() == GattCommunicationStatus::Success)
                    {
                        auto characteristics = charactiristicResult.Characteristics();

                        for (auto characteristic : characteristics)
                        {
                            std::cout << "___________________" << std::endl;
                            GattCharacteristicProperties properties = characteristic.CharacteristicProperties();

                            if ((properties & GattCharacteristicProperties::Notify) == GattCharacteristicProperties::Notify)
                            {
                                std::cout << "Notify property found" << std::endl;

                                GattCommunicationStatus status = co_await characteristic.WriteClientCharacteristicConfigurationDescriptorAsync(
                                    GattClientCharacteristicConfigurationDescriptorValue::Notify);
                                if (status == GattCommunicationStatus::Success)
                                {
                                    myCharacteristic = characteristic;

                                    co_return;
                                }

                            }

                        }
                    }
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
            //if (args.Name == "Wahoo KICKR 8E4F")
            if (args.Name() == L"Forerunner 245")
                device = args;
        });

    deviceWatcher.Updated([](DeviceWatcher sender, DeviceInformationUpdate args)
        {
            // Handle device updated
        });

    deviceWatcher.Removed([](DeviceWatcher sender, DeviceInformationUpdate args)
        {
            // Handle device removed
        });

    GattCharacteristic chr = GattCharacteristic(nullptr);

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



            BLOperationAsync(device, chr).get();

            chr.ValueChanged(function);
        }
    }
}







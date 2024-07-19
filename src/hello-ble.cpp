#include <iostream>
#include <iomanip>
#include <simpleble/SimpleBLE.h>


// from bluez5
#define HEART_RATE_SERVICE_UUID		"0000180d-0000-1000-8000-00805f9b34fb"
#define HEART_RATE_MEASUREMENT_UUID	"00002a37-0000-1000-8000-00805f9b34fb"
#define BODY_SENSOR_LOCATION_UUID	"00002a38-0000-1000-8000-00805f9b34fb"
#define HEART_RATE_CONTROL_POINT_UUID	"00002a39-0000-1000-8000-00805f9b34fb"

#define DEVICE_INFORMATION_SERVICE_UUID	"0000180a-0000-1000-8000-00805f9b34fb"


#define SEARCH_ID "COROS PACE 2 177FB1"

void print_byte_array(const SimpleBLE::ByteArray &bytes)
{
	for (auto b : bytes)
	{
		std::cout << std::hex << std::setfill('0') << std::setw(2) << (uint32_t)((uint8_t)b) << " ";
	}
	std::cout << std::endl;
}

void print_peripheral(SimpleBLE::Peripheral peripheral)
{
	std::cout << "Peripheral:" << std::endl;
	std::cout << "\tIdentifier: " << peripheral.identifier() << std::endl;
	std::cout << "\tAddress: " << peripheral.address() << std::endl;
	std::cout << std::dec << "\tRSSI: " << peripheral.rssi() << std::endl;
	std::cout << std::dec << "\tIs Connectable: " << peripheral.is_connectable() << std::endl;

	std::map<uint16_t, SimpleBLE::ByteArray> manufacturer_data = peripheral.manufacturer_data();
	for (auto &[manufacturer_id, data] : manufacturer_data)
	{
		std::cout << "\tManufacturer ID: " << manufacturer_id << std::endl;
		std::cout << "\tManufacturer Data: ";
		print_byte_array(data);
		std::cout << std::endl;
	}
}

void print_peripherals(std::vector<SimpleBLE::Peripheral> peripherals)
{
	for (auto p : peripherals)
	{
		print_peripheral(p);
	}
}


std::vector<SimpleBLE::Peripheral> scan_adapter(SimpleBLE::Adapter adapter, int timeout_ms)
{
	std::cout << "Adapter identifier: " << adapter.identifier() << std::endl;
	std::cout << "Adapter address: " << adapter.address() << std::endl;

	// Establish the list of peripherals to populate
	std::vector<SimpleBLE::Peripheral> peripherals;

	adapter.set_callback_on_scan_found([&](SimpleBLE::Peripheral peripheral)
									   { peripherals.push_back(peripheral); });

	// TODO: Actual callbacks on the scan - not just print...
	adapter.set_callback_on_scan_start([]()
									   { std::cout << "Scan started." << std::endl; });
	adapter.set_callback_on_scan_stop([]()
									  { std::cout << "Scan stopped." << std::endl; });
	adapter.scan_for(timeout_ms);

	return peripherals;
}

void print_services_and_chars(SimpleBLE::Peripheral* peripheral)
{
	for (auto &service : peripheral->services())
	{
		// Now find the characteristics of each service.
		std::cout << "Service: " << service.uuid() << std::endl;

		if (service.uuid().compare(HEART_RATE_SERVICE_UUID) == 0)
		{
			std::cout << "HEART RATE MONITOR FOUND" << std::endl;
		}
		else if (service.uuid().compare(DEVICE_INFORMATION_SERVICE_UUID) == 0)
		{
			std::cout << "CAN READ DEVICE INFO" << std::endl;
		}

		for (auto &characteristic : service.characteristics())
		{
			std::cout << "\tCharacteristic: " << characteristic.uuid() << std::endl;
			continue; // temp

			// And then capabilities...
			std::cout << "\t\tCapabilities: ";
			for (auto &capability : characteristic.capabilities())
			{
				std::cout << capability << " ";
				if (characteristic.can_read()) {
					SimpleBLE::ByteArray rx_data = peripheral->read(service.uuid(), characteristic.uuid());
					print_byte_array(rx_data);
				}
			}
			std::cout << std::endl;
		}
	}


}

int main(int argc, char **argv)
{
	if (!SimpleBLE::Adapter::bluetooth_enabled())
	{
		std::cout << "Bluetooth is not enabled" << std::endl;
		return 1;
	}

	auto adapters = SimpleBLE::Adapter::get_adapters();
	if (adapters.empty())
	{
		std::cout << "No Bluetooth adapters found" << std::endl;
		return 1;
	}

	// Use the first adapter - might be others in the future.
	auto adapter = adapters[0];
	auto peripherals = scan_adapter(adapter, 5000);
//	print_peripherals(peripherals);

	for (auto &peripheral : peripherals) {
		if (peripheral.is_connectable()) {
			std::cout << "Connecting to " << peripheral.identifier() << std::endl;
			// TODO: catch the exception
			peripheral.connect();

			// get UUID, look for HRM
			print_services_and_chars(&peripheral);
			
			peripheral.disconnect();
		}
	}

	auto search_id = SEARCH_ID;
	auto search_result = std::find_if(peripherals.begin(), peripherals.end(), [&](SimpleBLE::Peripheral peripheral)
									  { return peripheral.identifier() == search_id; });
	if (search_result == peripherals.end())
	{
		std::cout << "Failed to find " << search_id << std::endl;
		return -1;
	}

	std::cout << "Found: " << std::endl;
	print_peripheral(*search_result);

	// Connect, to find the services of the searched peripheral
	// TODO: error handling of connect...
	std::cout << "Connecting to " << search_result->identifier() << std::endl;
	search_result->connect();

	std::cout << "Connected." << std::endl;
//	print_services_and_chars(search_result);
	// Find out the services
	search_result->disconnect();
	return 0;

}

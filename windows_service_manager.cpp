#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include "json-develop/single_include/nlohmann/json.hpp"


using json = nlohmann::json;

class WindowsService {
public:
    std::vector<std::string> serviceName;
    std::string help;
    std::vector<std::string> displayName;
    
    void PrintDisplayNames() const{
    	for (auto& displayName : this->displayName){
    		std::cout << displayName << std::endl;
		}
	}
};

enum ServiceAction {
    Disable,
    Enable
};

int WindowsServiceAction(const WindowsService& service, ServiceAction action) {
    // Use the system function to enable or disable the service
    
    int result;
    
	for(auto &serviceName : service.serviceName){
	
		
		std::string command = "sc config " + serviceName + " start=";
	    if (action == Disable) {
	        command += "disabled";
	    } else {
	        command += "auto"; // You can change this to "demand" or "auto" as needed
	    }
	    int result = system(command.c_str());
	    
	}
	
    return result;
}

int UserConfirmWindowsServiceAction(const WindowsService& service, ServiceAction action) {
    // Ask the user for confirmation
    
    std::cout <<"Usage Of Service : "<< service.help << "\n\n";
    std::cout << "Service Display Names: " << std::endl << "-----------" << std::endl;
    service.PrintDisplayNames();
    std::cout <<"-----------";
    std::cout << "\n\n";
    std::cout << "Are you sure you want to " << (action == Disable ? "disable" : "enable") << " this service? (y/n): ";
    
    char response;
    std::cin >> response;
    
    if (response == 'y' || response == 'Y') {
        return WindowsServiceAction(service, action);
    } else {
        return 1; // Action canceled
    }
}

int LoadFromJson(std::vector<WindowsService> &WindowsServices, std::string fileName = "services.json"){
	// Load data from JSON file
    std::ifstream jsonFile(fileName);
    if (!jsonFile.is_open()) {
        std::cerr << "Error opening JSON file." << std::endl;
        return 1;
    }

    try {
        json jsonData;
        jsonFile >> jsonData;

        for (const auto& item : jsonData["Services"]) {
			WindowsService service;
			
            for (const auto& serviceName : item["serviceName"]) {
		    service.serviceName.push_back(serviceName);
			}
			service.help = item["help"];
			
			// Extract the array of display names from the JSON and store it in the displayName member.
			for (const auto& displayName : item["displayName"]) {
			    service.displayName.push_back(displayName);
			}
			WindowsServices.push_back(service);
        }
    } catch (json::exception& e) {
        std::cerr << "Error parsing JSON: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}

int RollBack(const std::vector<WindowsService> &services){
	for(auto &service : services){
		WindowsServiceAction(service, ServiceAction::Enable);
	}
	return 1;
}

void PrintHelp(){
	system("cls");
	std::cout << "this program will disable the services you agree to.\n" <<
					"DO NOT disable the services that are NOT ADVISED TO DISABLE unless you abolustly know what they are.\n" <<
					"You can enable all the listed services in the app by calling this app with \"rollbacl\" argument from the terminal.\n" <<
					"You can find the services by their display name in the services app.\n" <<
					"Be carefull out there!" << std::endl;
}

void ArgsHandler(std::string arg, const std::vector<WindowsService> &WindowsServices){
	
	if (arg == "rollback"){
		RollBack(WindowsServices);
		system("pause");
		exit(0);
	}
	
	if (arg == "help"){
		PrintHelp();
		system("pause");
		exit(0);
	}
}

int main(int argc, char ** args) {
	
	std::vector<WindowsService> WindowsServices;

    if (LoadFromJson(WindowsServices) == 1)
    	return 1;
	
	if (argc > 1){
		ArgsHandler(args[1], WindowsServices);
	}
	
	
	PrintHelp();
	system("pause");
	system("cls");

	std::cout << "rollback? y/n";
	char input;
	std::cin >> input;
	if(input == 'Y' || input == 'y'){
		RollBack(WindowsServices);
		system("pause");
		exit(0);
	}
	
	system("cls");	

    // Loop through WindowsServices and ask for confirmation to disable
	for (const auto& service : WindowsServices) {
        int result = UserConfirmWindowsServiceAction(service, ServiceAction::Disable);
        if (result == 0) {
            std::cout << "Service disabled successfully." << std::endl;
            system("pause");
            system("cls");
        } else if (result == 1) {
            std::cout << "Action canceled." << std::endl;
            system("pause");
            system("cls");
        } else {
            std::cout << "Error disabling service." << std::endl;
            system("pause");
            system("cls");
        }
    }

    return 0;
}

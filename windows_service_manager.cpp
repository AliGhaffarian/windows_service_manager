#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include "json-develop/single_include/nlohmann/json.hpp"
#include <regex>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <filesystem>

using json = nlohmann::json;

//intersect
//log
//rollback based on log

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
	bool operator ==(const WindowsService &second){
		return this->serviceName == second.serviceName;
	}
};

enum ServiceAction {
    Disable,
    Enable
};

int WindowsServiceAction(const WindowsService& service, ServiceAction action, std::ofstream &log) {
    // Use the system function to enable or disable the service
    
    int result;
    
	for(auto &serviceName : service.serviceName){
	
		
		std::string command = "sc config " + serviceName + " start=";
	    if (action == Disable) {
	        command += "disabled";
	    } else {
	        command += "auto"; // You can change this to "demand" or "auto" as needed
	    }
	    result = system(command.c_str());
	    if (result == 0){
			std::string logStr = serviceName + " " + (action == Disable ? "disabled" : "auto");
			log.write(logStr.c_str(), logStr.size());
			log.write("\n", 1);
		}
		
	}
	
    return result;
}

int UserConfirmWindowsServiceAction(const WindowsService& service, ServiceAction action, std::ofstream &log) {
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
        return WindowsServiceAction(service, action, log);
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

std::string LoadFromTextFile(std::string lileName){
	std::ifstream file(lileName);

	if (file.is_open() == false){
		std::cout << "problem reading result of sc query";
		return std::string();
	}
		

	std::string loadedLine;
	std::string wholeString;

	while(std::getline(file, loadedLine))
		wholeString += loadedLine + "\n";

	file.close();

	return wholeString;

}

std::vector<std::string> ActiveServiceNames(std::string fileName = "QueryResult.txt"){
	if (system("servicequery") != 0){
		std::cout << "\n-------------------\nproblem running servicequery.bat\nchecking if services are running or not is not possible\ncontinue? (y/n)";
		char response;
		std::cin >> response;
		if (response == 'n' || response == 'N')
			exit(0);
	}
	
	std::string loadedText;
	std::vector<std::string> result;
	if ((loadedText = LoadFromTextFile(fileName)) == std::string())
		return std::vector<std::string>();

	std::regex pattern("SERVICE_NAME: ([^\\r\\n]+)");

    // Iterator to search for matches
    std::sregex_iterator iterator(loadedText.begin(), loadedText.end(), pattern);
    std::sregex_iterator end;

    // Loop through the matches and extract SERVICE_NAME values
    while (iterator != end) {
        std::smatch match = *iterator;
        result.push_back(match[1].str());
        

        // Move to the next match
        ++iterator;
    }
	return result;
}



int RollBack(const std::vector<WindowsService> &services, std::ofstream &log){
	for(auto &service : services){
		WindowsServiceAction(service, ServiceAction::Enable, log);
	}
	return 0;
}

void PrintHelp(){
	system("cls");
	std::cout << "this program will disable the services you agree to.\n" <<
					"DO NOT disable the services that are NOT ADVISED TO DISABLE unless you abolustly know what they are.\n" <<
					"You can enable all the listed services in the app by calling this app with \"rollbacl\" argument from the terminal.\n" <<
					"You can find the services by their display name in the services app.\n" <<
					"Be carefull out there!" << std::endl;
}



void ArgsHandler(std::string arg, const std::vector<WindowsService> &WindowsServices, std::ofstream &log){
	
	if (arg == "rollback"){
		RollBack(WindowsServices, log);
		system("pause");
		log.close();
		exit(0);
	}
	
	if (arg == "help"){
		PrintHelp();
		system("pause");
		log.close();
		exit(0);
	}
}

int ServicePurge(std::vector<WindowsService> &windowsServices){
	std::vector<std::string> activeWindowsServiceNames = ActiveServiceNames();
	int removedTimes = 0;
	
	
	
	for (size_t i = 0 ; i < windowsServices.size() ; i++)
	{
		for(size_t j = 0 ; j < windowsServices[i].serviceName.size() ; j++){
			
			if (
			std::find(activeWindowsServiceNames.begin(), 
							activeWindowsServiceNames.end(), 
							windowsServices[i].serviceName[j]) 
				== activeWindowsServiceNames.end()){
								
				windowsServices.erase(windowsServices.begin() + i);
				
				i = 0;
				removedTimes += 1;
			}
		}
	}
	
	return removedTimes;
	
}

std::string GetLogFileName() {
    std::time_t tt = std::time(nullptr);
    std::tm* ti = std::localtime(&tt);
    char buffer[80];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d", ti);

    std::filesystem::path logDir("log");
    if (!std::filesystem::exists(logDir))
        std::filesystem::create_directory(logDir);

    std::ostringstream fileNameStream;
    fileNameStream << logDir.string() << "\\" << buffer;

    std::string baseFileName = fileNameStream.str();
    std::string fileName = baseFileName;
    int counter = 0;

    while (std::filesystem::exists(fileName + ".txt")) {
        counter++;
        std::ostringstream newFileNameStream;
        newFileNameStream << baseFileName << "_" << counter;
        fileName = newFileNameStream.str();
    }

    fileName += ".txt"; // Append the file extension

    return fileName;
}

std::ofstream CreateLogFile(){
	std::string fileName = GetLogFileName();
	
    std::ofstream log(fileName);

    if (!log.is_open()) {
        std::cerr << "Problem opening log file: " << fileName << std::endl;
		std::cerr << "Error code: " << errno << std::endl;
        // Handle the error here, e.g., throw an exception or return an empty stream
		system("dir");
		system("pause");
    }

    return log;
}

int main(int argc, char ** args) {
	
	std::vector<WindowsService> windowsServices;
	std::ofstream log = CreateLogFile();

	if (LoadFromJson(windowsServices) == 1)
    	return 1;
	
	if (argc > 1){
		ArgsHandler(args[1], windowsServices, log);
	}
	
	system("cls");
	
	
	
	PrintHelp();
	system("pause");
	system("cls");

	std::cout << ServicePurge(windowsServices) << " services were not running and skipped" << std::endl;
	system("pause");
	system("cls");
	
	std::cout << "rollback previous changes? y/n";
	char input;
	std::cin >> input;
	if(input == 'Y' || input == 'y'){
		RollBack(windowsServices, log);
		system("pause");
		log.close();
		exit(0);
	}
	
	system("cls");	

    // Loop through WindowsServices and ask for confirmation to disable
	for (const auto& service : windowsServices) {
        int result = UserConfirmWindowsServiceAction(service, ServiceAction::Disable, log);
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
	log.close();
    return 0;
}

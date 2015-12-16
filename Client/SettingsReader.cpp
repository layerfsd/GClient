#include "StdAfx.h"
#include "SettingsReader.h"

std::string SettingsReader::getSetting(std::string settingName){
	// Grazina nustatymo reiksme stringe
	// Grazinamos reiksmes:
	// NULL - jei nerado tokio nustatymo
	// REIKSME - jie rado
	try{
		return this->settings[settingName];
	}catch(System::Exception^){
		return nullptr;
	}
}

int SettingsReader::ReadRegistry(){
	// Grazinomos reiksmes:
	// 0 - jei pavyko nuskaityti registrus
	// 1 - jei nepavyko nuskaityt registrus

	//Registru kintamieji
	RegistryKey^ RKey;

	//Prasom po viena registo kintamojo
	try{
		RKey = Registry::CurrentUser;
		RKey = RKey->OpenSubKey("Software");
		RKey = RKey->OpenSubKey("gNet");
		RKey = RKey->OpenSubKey("Client");
		array<String^>^ subKeys = RKey->GetValueNames();
		string tempKey;
		string tempValue;
		for(int i = 0; i < subKeys->Length; i++){
			// System::String verciam i STD ir talinam i hash_map
			this->SystemStringToStdString(subKeys[i],tempKey);
			this->SystemStringToStdString((RKey->GetValue(subKeys[i]))->ToString(), tempValue);
			//this->settings.insert(std::make_pair(tempKey, tempValue));
			this->settings[tempKey] = tempValue;
		}
		//std::string testas = ; 
		//std::cout << testas << endl;
	} catch (System::Exception^){
		//std::cerr << "Nepavyko nuskaityti duomenu is registru" << std::endl;
		return 10;
	}
	__finally {
		if(RKey) RKey->Close();
	}
	return 0;
}

void SettingsReader::SystemStringToStdString(System::String^ s, std::string& string){
	string = marshal_as<std::string>(s);
}

SettingsReader::SettingsReader(void)
{
	this->ReadRegistry();
}

SettingsReader::~SettingsReader(void)
{
}
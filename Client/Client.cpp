// Client.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "GClientLib.h"

using namespace GClientLib;

int _tmain(int argc, _TCHAR* argv[])
{
	// Nuskaito nustatymus is registru
	SettingsReader^ settings = gcnew SettingsReader();
	// Kuriamas visu sujungimu saraso saugykla
	SocketToObjectContainer^ STOContainer = gcnew SocketToObjectContainer();

	timeval time;
	time.tv_sec = 0;
	time.tv_usec = 5000;

	// --- Select funkcijos kintamieji ---
	// Inicijuoju
	fd_set skaitomiSocket, rasomiSocket, klaidingiSocket;
	// Nunulinu strukturas
	FD_ZERO(&skaitomiSocket);
	FD_ZERO(&rasomiSocket);
	// Pridedu konsole prie rasomu
	//FD_SET(0, &rasomiSocket);
	FD_ZERO(&klaidingiSocket);
	
	//Sukuria socketa jungtis prie pagrindinio serverio
	ToServerSocket^ ToServer = gcnew ToServerSocket(settings->getSetting("serverAddress"),
		settings->getSetting("serverPort"), &skaitomiSocket, &rasomiSocket, &klaidingiSocket);
	// Tikrinam ar pavyko uzmegsti rysi iki centrinio serverio
	if(ToServer->GetSocket() == INVALID_SOCKET){
		printf("Nepavyko sukurti objekto darbui su centriniu serveriu\n");
		return 1;
	}
	// Priskiriam pradinius rezius
	Globals::maxD = ToServer->GetSocket();
	// Sukurta sujungima dedame i sarasus
	STOContainer->Add(ToServer);
	
	// Pradedu CLI gija valdymui
	CLI ^console = gcnew CLI(ToServer, STOContainer, settings);
	Thread ^consoleThread = gcnew Thread(gcnew ThreadStart(console, &CLI::Start));
	consoleThread->Start();

	// JSON API
	JSONapi^ JSON_API = gcnew JSONapi(settings, ToServer);
	// WEB klientu socketas
	JSONapiServer^ JSON;
	try{
		// Sukuriu JSON API socket'a
		JSON = gcnew JSONapiServer(settings->getSetting("JSONapi_address"), settings->getSetting("JSONapi_port"), &skaitomiSocket, &rasomiSocket, &klaidingiSocket, JSON_API);
		// Tikrinam ar klausytis nurodytu adresu ir portu
		if (JSON->GetSocket() == INVALID_SOCKET){
			printf("Nepavyko klausytis %s:%s\n", settings->getSetting("JSONapi_address"), settings->getSetting("JSONapi_port"));
		}
		else {
			// Nustatom maksimalu deskriptoriu
			// Tikrinu ar JSON socketas nera didensi nei ToServer :)
			if (Globals::maxD < (int)JSON->GetSocket())
				Globals::maxD = (int)JSON->GetSocket();
			// Sukurta sujungima dedame i sarasus
			STOContainer->Add(JSON);
		}
	} catch (Exception^ e)
	{
		Console::WriteLine("Nepavyko sukurti prievado grafinei sasajai. Klaida:");
		Console::WriteLine(e);
	}

	fd_set tempRead, tempWrite, tempError;	// Laikinas dekriptoriu kintamasis
	while(!Globals::quit){
		// Siam ciklui skaitomi dekriptoriai
		tempRead = skaitomiSocket;
		tempWrite = rasomiSocket;
		tempError = klaidingiSocket;
		// Pasiemam dekriptorius kurie turi kazka nuskaitimui
		if(select(Globals::maxD+1, &tempRead, &tempWrite, &tempError, &time) == SOCKET_ERROR){
			// Select nepasiseke grazinti dekriptoriu
			switch(WSAGetLastError()){
			case WSAENOTSOCK:{
				printf("Praradau rysi su serveriu\n");
				ToServer->Reconnect();
							 }
			default:{
				printf("Select klaida : %d\n", WSAGetLastError());
				continue;
					}
			}
		}
		// Begam per esamus sujungimus ir ieskom ar kas ka atsiunte
		for (int i = 0; i <= Globals::maxD; i++){
			// Tikrinam ar i-asis yra dekriptorius kuriame ivyko klaida
			if (FD_ISSET(i, &tempError)) {
				printf("Ivyko kalida %d dekriptoriuje", i);
			}
			
			/*
			// Tikrinam ar i-asis yra dekriptorius kuriame ivyko klaida
			if (FD_ISSET(i, &tempWrite)) {
				printf("Ivyko rasimas %d dekriptoriuje", i);
			}
			*/
			

			// Tikrinam ar i-asis yra deskriptorius is kurio reikia ka nors nuskaityti
			if (FD_ISSET(i, &tempRead)) {
				try{
					STOContainer->FindBySocket(i)->Recive(STOContainer);
				}catch(System::Exception^ e){
					//printf("Nepavyko rasti %d\n", i);
					// Salinu socket is skaitomu saraso
					FD_CLR(i, &skaitomiSocket);
					FD_CLR(i, &rasomiSocket);
					FD_CLR(i, &klaidingiSocket);
				}
			} // if (FD_ISSET(i, &read_fds)) { pabaiga
		} // for(int i = minD; i <= maxD; i++){ pabaiga
	}
	return 0;
}

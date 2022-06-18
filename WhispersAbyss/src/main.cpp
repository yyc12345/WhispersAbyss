#include "../include/abyss_client.hpp"
#include "../include/celestia_server.hpp"
#include "../include/output_helper.hpp"
#include <atomic>
#include <conio.h>

void MainWorker(
	std::atomic_bool& signalStop, 
	WhispersAbyss::OutputHelper& output, 
	WhispersAbyss::CelestiaServer& server, 
	WhispersAbyss::AbyssClient& client) {

	std::deque<WhispersAbyss::Bmmo::Messages::IMessage*> abyss_incoming_message, abyss_outbound_message;
	std::deque<WhispersAbyss::Cmmo::Messages::IMessage*> celestia_incoming_message, celestia_outbound_message;

	// start server and client and waiting for initialize
	// start abyss client first, because abyss client need more time to init
	output.Printf("Starting server & client...");
	server.Start();
	client.Start();
	while (!server.mIsRunning.load());
	output.Printf("Celestia Server started.");
	while (!client.mIsRunning.load());
	output.Printf("Abyss Client started.");

	// msg deliver
	while (true) {
		// check exit
		if (signalStop.load()) break;
		if (client.mIsDead.load()) {
			output.Printf("Abyss Client is dead. Stop worker.");
			break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));

		// todo: placeholder for proc msg
		client.Recv(&abyss_incoming_message);
		while (abyss_incoming_message.begin() != abyss_incoming_message.end()) {
			auto msg = *abyss_incoming_message.begin();
			delete msg;
			abyss_incoming_message.pop_front();
		}
		server.Recv(&celestia_incoming_message);
		while (celestia_incoming_message.begin() != celestia_incoming_message.end()) {
			auto msg = *celestia_incoming_message.begin();
			delete msg;
			celestia_incoming_message.pop_front();
		}

	}

	output.Printf("Stoping server & client...");
	server.Stop();
	output.Printf("Celestia Server stoped.");
	client.Stop();
	output.Printf("Abyss Client stoped.");
	//while (server.mIsRunning.load());
	//while (client.mIsRunning.load());
}

int main(int argc, char* argv[]) {

	puts("Whispers Abyss - A sh*t protocol converter.");
	puts("");
	puts("From the ashes to this world anew.");
	puts("Ah... How brightly burned the oath. Hear the inferno's call.");
	puts("Now... Is the time for my second coming.");
	puts("");
	puts("======");

	// allocate signal for worker
	std::atomic_bool signalStop;
	signalStop.store(false);

	WhispersAbyss::OutputHelper outputHelper;
	WhispersAbyss::CelestiaServer celestiaServer(&outputHelper, (uint16_t)6172u);
	WhispersAbyss::AbyssClient abyssClient(&outputHelper, "p.okbc.st:26676");

	// start worker
	std::thread tdMainWorker(
		&MainWorker, 
		std::ref(signalStop),
		std::ref(outputHelper), 
		std::ref(celestiaServer), 
		std::ref(abyssClient)
	);

	// accept input
	char inc;
	while (true) {
		inc = _getch();

		switch (inc) {
			case 'q':
			{
				signalStop.store(true);
				goto exit_program;	// exit switch and while
			}
			default:
			{
				puts("Invalid command!");
				break;
			}
				
		}
	}

	// stop server and client
exit_program:
	if (tdMainWorker.joinable())
		tdMainWorker.join();

	outputHelper.Printf("See ya~");

	return 0;
}


#include "bridge_factory.hpp"
#include <atomic>
#include <conio.h>

void MainWorker(
	uint16_t tcp_port,
	std::atomic_bool& signalStop,
	std::atomic_bool& signalProfile,
	WhispersAbyss::OutputHelper& output) {

	// init factory
	WhispersAbyss::BridgeFactory factory(&output, tcp_port);

	// core processor
	std::deque<WhispersAbyss::TcpInstance*> conns;
	std::deque<WhispersAbyss::BridgeInstance*> conn_pairs, cached_pairs;
	bool order_profile = false;
	while (true) {
		// check exit
		if (signalStop.load()) break;
		std::this_thread::sleep_for(std::chrono::milliseconds(WhispersAbyss::DISPOSAL_INTERVAL));

		// get whether need profile
		order_profile = false;
		order_profile = signalProfile.exchange(order_profile);
		if (order_profile) {
			output.Printf("Start collecting profile...");
			factory.ReportStatus();
		}

	}

	// stop factory
	factory.Stop();
	factory.mStatusReporter.SpinUntil(WhispersAbyss::StateMachine::Stopped);
}

int main(int argc, char* argv[]) {

	WhispersAbyss::OutputHelper output;

	output.RawPrintf("Whispers Abyss - A sh*t protocol converter.");
	output.RawPrintf("");
	output.RawPrintf("From the ashes to this world anew.");
	output.RawPrintf("Ah... How brightly burned the oath. Hear the inferno's call.");
	output.RawPrintf("Now... Is the time for my second coming.");
	output.RawPrintf("");
	output.RawPrintf("======");
	output.RawPrintf("");

	// ========== Check Parameter ==========
	if (argc != 2) {
		puts("Wrong arguments.");
		puts("Syntax: WhispersAbyss [accept_port]");
		puts("Program will exit. See README.md for more detail about commandline arguments.");
		return 0;
	}
	long int argsAcceptPort = strtoul(argv[1], NULL, 10);
	if (argsAcceptPort == LONG_MAX || argsAcceptPort == LONG_MIN || argsAcceptPort > 65535u) {
		puts("Wrong arguments. Port value is illegal.");
		puts("Syntax: WhispersAbyss [accept_port] [server_address]");
		puts("Program will exit. Please specific a correct port number.");
		return 0;
	}

	// ==========Real Work ==========
	// allocate signal for worker
	std::atomic_bool signalStop, signalProfile;
	signalStop.store(false);


	// start worker
	std::thread tdMainWorker(
		&MainWorker,
		static_cast<uint16_t>(argsAcceptPort),
		std::ref(signalStop),
		std::ref(signalProfile),
		std::ref(output)
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
			case 'p':
			{
				signalProfile.store(true);
				break;
			}
			default:
			{
				output.RawPrintf("Invalid command!");
				break;
			}

		}
	}

	// stop server and client
exit_program:
	if (tdMainWorker.joinable())
		tdMainWorker.join();

	output.RawPrintf("======");
	output.RawPrintf("See ya~");

	return 0;
}


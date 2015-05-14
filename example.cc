
#include "channel.h"

#include <iostream>
#include <future>

using namespace channel;

void SendFiveNumbers(Channel<int> *chan) {
	auto s = chan->CreateSender();
	for (int i = 0; i < 5; i++) {
		s->Send(i);
	}
	// only sender is destroyed, so channel is killed.
}

void RecieveUntilEmpty(Channel<int> *chan) {
	auto r = chan->CreateReciever();
	int i;
	while (r->Recieve(i)) {
		std::cout << "recieved: " << i << std::endl;
	}
	// reciever is destroyed.
}

int main() {
	Channel<int> chan;
	auto st = std::async(SendFiveNumbers, &chan);
	auto rt = std::async(RecieveUntilEmpty, &chan);
	rt.wait();
}


// Copyright (c) 2015 Connor Taffe. All rights reserved.

#ifndef CHANNEL_H_
#define CHANNEL_H_

#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <iostream>
#include <memory>

namespace channel {

template <typename t>
class Channel {
public:
	Channel(int max = 10) : senders(0), chan(max) {}

	class Sender {
	public:
		Sender(Channel<t>& m) : mesg(m) {}
		Sender(const Sender&) = delete;
		Sender(Sender&&) = delete;
		~Sender() {
			mesg.DeadSender();
		}

		void Send(const t& item) {
			mesg.chan.Put(item);
		}
	private:
		Channel<t>& mesg;
	};

	class Reciever {
	public:
		Reciever(Channel<t>& m) : mesg(m) {}
		Reciever(const Reciever&) = delete;
		Reciever(Reciever&&) = delete;
		~Reciever() {}

		bool Recieve(t& item) {
			return mesg.chan.Get(&item);
		}
	private:
		Channel<t>& mesg;
	};

	std::shared_ptr<Reciever> CreateReciever() {
		std::shared_ptr<Reciever> r(new Reciever(*this));
		return r;
	}

	std::shared_ptr<Sender> CreateSender() {
		senders++;
		std::shared_ptr<Sender> s(new Sender(*this));
		return s;
	}

	void Clear() {
		chan.Reset();
	}
private:

	class BaseChannel {
	public:
		BaseChannel(int max) : alive_(true), max_count(max) {}

		bool Get(t *item) {
			// aquire mutex & wait for empty.
			std::unique_lock<std::mutex> lock(mut);
			while (queue.empty()) {
				if (!alive_) {
					return false;
				} else {
					std::cout << "WAITING" << std::endl;
					nempty.wait(lock);
				}
			}

			*item = queue.front();
			queue.pop();

			lock.unlock();
			nfull.notify_one();
			return true;
		}

		void Put(const t& item) {
			// aquire mutex & wait for empty.
			std::unique_lock<std::mutex> lock(mut);
			while (queue.size() == max_count) {
				if (!alive_) {
					return;
				} else {
					nfull.wait(lock);
				}
			}

			queue.push(item);

			lock.unlock();
			nempty.notify_one();
		}

		void Kill() {
			// wake up all waiting because death.
			alive_ = false;
			nfull.notify_all();
			nempty.notify_all();
		}

		void Reset() {
			std::unique_lock<std::mutex> lock(mut);
			while (!queue.empty()) {
				queue.pop();
			}
			alive_ = true;
			lock.unlock();
		}

		bool alive() const {
			return alive_;
		}
	private:
		std::mutex mut;
		std::queue<t> queue;

		// dead queue.
		std::atomic<bool> alive_;

		// atomic access full/empty
		const int max_count;

		std::condition_variable nfull;
		std::condition_variable nempty;
	};

	void DeadSender() {
		senders--;
		if (senders == 0 && chan.alive()) {
			chan.Kill(); // no more senders.
		}
	}

	BaseChannel chan;
	std::atomic<int> senders = {0};
};

} // namespace channel

#endif // CHANNEL_H_

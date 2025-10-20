#pragma once

#include <cstdint>
#include <mutex>
#include <unordered_map>

/**
 * Ticketer
 * This single-header library provides a reference implementation of what I call the Service Ticket Model,
 * in which requests to a separate service can be tracked in a decoupled and thread-safe manner.
 *
 * In the most basic use-case, suppose a main thread and a service thread.
 * Let's make a service class to contain a work queue and facilitate
 *
 * class ServiceClass {
 * public:
 *     ticket_t request(Job &job);
 *     TicketStatus claim(ticket_t ticket);
 *
 *     // ...
 * private:
 *     std::thread _worker_thread;
 *     std::queue<Job> _work_queue;
 *     Ticketer<Job> _ticketer;
 *
 * }
 *
 * ticket_t ServiceClass::request(Job &job) {
 *     ticket_t ticket = _ticketer.generate(job);
 *     job.ticket = ticket;
 *     _work_queue.emplace(job);
 *     return ticket;
 * }
 *
 * ticket_t ServiceClass::claim(ticket_t ticket) {
 *     return _ticketer.claim(ticket);
 * }
 *
 * void ServiceClass::thread_loop() {
 *     while (!_work_queue.empty()) {
 *         Job job = std::move(_work_queue.front());
 *         _work_queue.pop();
 *
 *         if (_process(job) == true) {
 *             _ticketer.update(job.ticket, SUCCEEDED, job);
 *         }
 *         else {
 *             _ticketer.update(job.ticket, FAILED, job);
 *         }
 *     }
 * }
 *
 * And suppose a main window with a button and a timer event for checking on long-running tasks..
 *
 * void MainWindow::onButtonPushed() {
 *     _job_ticket = _service.request(Job("example task")); // for some class member variable _job_ticket
 * }
 *
 * void MainWindow::timerEvent() { // suppose an event every 100ms or so
 *     if (_job_ticket == NO_TICKET) {
 *         // no ticket to track
 *         return;
 *     }
 *     TicketStatus status = _service.claim(_job_ticket);
 *     switch(status) {
 *     case PENDING:
 *         std::cout << "..." << std::endl;
 *         break;
 *     case SUCCEEDED:
 *         std::cout << ":)" << std::endl;
 *         _job_ticket = NO_TICKET;
 *         break;
 *     case FAILED:
 *         std::cout << ":(" << std::endl;
 *         _job_ticket = NO_TICKET;
 *         break;
 *     case CANCELED:
 *     case ERROR:
 *     case NOT_FOUND:
 *         std::cout << "!!!" << std::endl;
 *         _job_ticket = NO_TICKET;
 *         break;
 *     }
 * }
 */

enum TicketStatus {
	PENDING,
	SUCCEEDED,
	FAILED,
	CANCELED,
	ERROR,
    NOT_FOUND
};

using ticket_t = uint64_t;

// The ticketer will never return a ticket value of 0.
// So, 0 can always be counted on to represent a non-existent ticket.
// This way, you can check if (!ticket) {} or if (ticket) {}
const ticket_t NO_TICKET = 0;

class BasicTicketer
{
public:
	BasicTicketer() : _next_ticket(1) , _mtx() , _tickets()
	{
		// Empty
	}

	ticket_t generate()
	{
		const std::lock_guard lock(_mtx);
		ticket_t ticket = _next_ticket;
		_tickets[ticket] = PENDING;
		_next_ticket += 1;
		if (_next_ticket == 0) {
			_next_ticket = 1;
		}
		return ticket;
	}

	TicketStatus claim(ticket_t ticket)
	{
		const std::lock_guard lock(_mtx);
		auto found = _tickets.find(ticket);
		if (found == _tickets.end()) {
			return NONE;
		}

		TicketStatus status = found->second;
		if (status != PENDING) {
			_tickets.erase(found);
		}
		return status;
	}

	bool discard(ticket_t ticket)
	{
		const std::lock_guard lock(_mtx);
		auto found = _tickets.find(ticket);
		if (found == _tickets.end()) {
			return false;
		}
		_tickets.erase(found);
		return true;
	}

	bool update(ticket_t ticket, TicketStatus status)
	{
		const std::lock_guard lock(_mtx);
		auto found = _tickets.find(ticket);
		if (found == _tickets.end()) {
			return false;
		}

		found->second = status;
		return true;
	}

	void clear()
	{
		const std::lock_guard lock(_mtx);
		_tickets.clear();
	}

private:
	ticket_t _next_ticket;
	std::mutex _mtx;
	std::unordered_map<ticket_t, TicketStatus> _tickets;
};

template <typename T>
class Ticketer
{
public:
	Ticketer() : _next_ticket(1), _mtx(), _tickets()
	{
		// Empty
	}

    ticket_t generate(T &data)
    {
        const std::lock_guard lock(_mtx);

        ticket_t ticket = _next_ticket;
        _tickets[ticket] = { PENDING, data };

        _next_ticket += 1;
        if (_next_ticket == 0) {
            _next_ticket = 1;
        }
        return ticket;
    }

    TicketStatus claim(ticket_t ticket, T &result)
	{
		const std::lock_guard lock(_mtx);

		auto found = _tickets.find(ticket);
		if (found == _tickets.end()) {
            return NOT_FOUND;
		}

		Entry &entry = found->second;
		if (entry.status != PENDING) {
			TicketStatus status = entry.status;
			result = entry.data;
			_tickets.erase(found);
			return status;
		}
        return entry.status;
	}

	bool discard(ticket_t ticket)
	{
		const std::lock_guard lock(_mtx);
		auto found = _tickets.find(ticket);
		if (found == _tickets.end()) {
			return false;
		}
		_tickets.erase(found);
		return true;
	}

	bool update(ticket_t ticket, TicketStatus status, const T &data)
	{
		const std::lock_guard lock(_mtx);
		auto found = _tickets.find(ticket);
		if (found == _tickets.end()) {
			return false;
		}
		Entry &entry = found->second;
		entry.status = status;
		entry.data = data;
		return true;
	}

	void clear()
	{
		const std::lock_guard lock(_mtx);
		_tickets.clear();
	}

private:
	struct Entry {
		TicketStatus status;
		T data;
	};
	ticket_t _next_ticket;
	std::mutex _mtx;
	std::unordered_map<ticket_t, Entry> _tickets;
};

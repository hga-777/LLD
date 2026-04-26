#include <iostream>
#include <string>
#include <memory>
#include <stdexcept>
#include <vector>
// ─────────────────────────────────────────────
// Ticket — the request object passed along chain
// ─────────────────────────────────────────────
enum class Severity {
    MINOR    = 1,  // billing questions, password resets
    MODERATE = 2,  // feature not working, UI bugs
    CRITICAL = 3,  // data loss, service down
    DISASTER = 4   // full outage, security breach
};

std::string severityToStr(Severity s) {
    switch(s) {
        case Severity::MINOR:    return "MINOR";
        case Severity::MODERATE: return "MODERATE";
        case Severity::CRITICAL: return "CRITICAL";
        case Severity::DISASTER: return "DISASTER";
    }
    return "UNKNOWN";
}

struct Ticket {
    int         id;
    std::string description;
    Severity    severity;
};

// ─────────────────────────────────────────────
// IHandler — abstract base for every handler
// KEY: each handler knows its successor (next in chain)
// ─────────────────────────────────────────────
class IHandler {
protected:
    std::shared_ptr<IHandler> next_; // next handler in chain

public:
    // Returns *this so we can chain: h1.setNext(h2).setNext(h3)
    IHandler& setNext(std::shared_ptr<IHandler> next) {
        next_ = std::move(next);
        return *next_;
    }

    virtual void handle(const Ticket& ticket) = 0;
    virtual ~IHandler() = default;

protected:
    // Helper — escalate to next or log unhandled
    void escalate(const Ticket& ticket) {
        if (next_) {
            std::cout << "  ↑ Escalating ticket #" << ticket.id << " to next level...\n";
            next_->handle(ticket);
        } else {
            std::cout << "  ⚠️  Ticket #" << ticket.id
                      << " could not be handled by anyone in the chain!\n";
        }
    }
};

// ─────────────────────────────────────────────
// Concrete Handlers — one per support level
// Each knows EXACTLY what severity it can handle
// ─────────────────────────────────────────────

// Level 1 — handles MINOR only
class FrontlineAgent : public IHandler {
    std::string name_;
public:
    explicit FrontlineAgent(std::string name) : name_(std::move(name)) {}

    void handle(const Ticket& ticket) override {
        std::cout << "[L1 Agent: " << name_ << "] Received ticket #"
                  << ticket.id << " [" << severityToStr(ticket.severity) << "]\n";

        if (ticket.severity == Severity::MINOR) {
            std::cout << "[L1 Agent: " << name_ << "] ✅ Resolved: "
                      << ticket.description << "\n";
        } else {
            std::cout << "[L1 Agent: " << name_ << "] ❌ Beyond my authority.\n";
            escalate(ticket);
        }
    }
};

// Level 2 — handles MINOR + MODERATE
class TechSpecialist : public IHandler {
    std::string name_;
public:
    explicit TechSpecialist(std::string name) : name_(std::move(name)) {}

    void handle(const Ticket& ticket) override {
        std::cout << "[L2 Specialist: " << name_ << "] Received ticket #"
                  << ticket.id << " [" << severityToStr(ticket.severity) << "]\n";

        if (ticket.severity <= Severity::MODERATE) {
            std::cout << "[L2 Specialist: " << name_ << "] ✅ Resolved: "
                      << ticket.description << "\n";
        } else {
            std::cout << "[L2 Specialist: " << name_ << "] ❌ Need management approval.\n";
            escalate(ticket);
        }
    }
};

// Level 3 — handles up to CRITICAL
class SupportManager : public IHandler {
    std::string name_;
public:
    explicit SupportManager(std::string name) : name_(std::move(name)) {}

    void handle(const Ticket& ticket) override {
        std::cout << "[L3 Manager: " << name_ << "] Received ticket #"
                  << ticket.id << " [" << severityToStr(ticket.severity) << "]\n";

        if (ticket.severity <= Severity::CRITICAL) {
            std::cout << "[L3 Manager: " << name_ << "] ✅ Resolved with priority fix: "
                      << ticket.description << "\n";
        } else {
            std::cout << "[L3 Manager: " << name_ << "] ❌ Escalating to director.\n";
            escalate(ticket);
        }
    }
};

// Level 4 — Director, handles EVERYTHING including DISASTER
class Director : public IHandler {
    std::string name_;
public:
    explicit Director(std::string name) : name_(std::move(name)) {}

    void handle(const Ticket& ticket) override {
        std::cout << "[L4 Director: " << name_ << "] Received ticket #"
                  << ticket.id << " [" << severityToStr(ticket.severity) << "]\n";
        // Director is last — handles all remaining, no further escalation
        std::cout << "[L4 Director: " << name_ << "] ✅ Emergency protocol activated: "
                  << ticket.description << "\n";
    }
};

// ─────────────────────────────────────────────
// Chain Builder — assembles the chain cleanly
// Keeps main() free of wiring logic
// ─────────────────────────────────────────────
class SupportChainBuilder {
public:
    static std::shared_ptr<IHandler> build() {
        auto agent    = std::make_shared<FrontlineAgent>("Ravi");
        auto spec     = std::make_shared<TechSpecialist>("Priya");
        auto manager  = std::make_shared<SupportManager>("Arjun");
        auto director = std::make_shared<Director>("Sunita");

        // Wire the chain: agent → spec → manager → director
        agent->setNext(spec)
              .setNext(manager)
              .setNext(director);

        return agent; // entry point of chain
    }
};

// ─────────────────────────────────────────────
// Driver
// ─────────────────────────────────────────────
int main() {
    auto chain = SupportChainBuilder::build();

    std::vector<Ticket> tickets = {
        {101, "Password reset request",        Severity::MINOR},
        {102, "Login button not responding",   Severity::MODERATE},
        {103, "Database corruption detected",  Severity::CRITICAL},
        {104, "Full system outage — all down", Severity::DISASTER},
    };

    for (auto& ticket : tickets) {
        std::cout << "\n── Processing Ticket #" << ticket.id << " ──\n";
        chain->handle(ticket);
    }
}
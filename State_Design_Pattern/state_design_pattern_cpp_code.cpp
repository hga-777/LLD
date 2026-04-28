#include <iostream>
#include <memory>
#include <string>
#include <stdexcept>

// ─────────────────────────────────────────────
// Forward Declaration
// ─────────────────────────────────────────────
class DocumentContext;

// ─────────────────────────────────────────────
// IDocumentState — State Interface
// Defines behavior that changes with state
// ─────────────────────────────────────────────
class IDocumentState {
public:
    virtual void edit(DocumentContext& ctx) = 0;
    virtual void submit(DocumentContext& ctx) = 0;
    virtual void publish(DocumentContext& ctx) = 0;
    virtual std::string name() const = 0;
    virtual ~IDocumentState() = default;
};

// ─────────────────────────────────────────────
// DocumentContext — holds current state
// Client interacts ONLY with this
// ─────────────────────────────────────────────
class DocumentContext {
    std::unique_ptr<IDocumentState> state_;

    void log(const std::string& action) const {
        std::cout << "[Context] State=" << state_->name()
                  << " Action=" << action << "\n";
    }

public:
    DocumentContext(std::unique_ptr<IDocumentState> initialState)
        : state_(std::move(initialState)) {}

    void setState(std::unique_ptr<IDocumentState> newState) {
        state_ = std::move(newState);
    }

    void edit() {
        log("EDIT");
        state_->edit(*this);
    }

    void submit() {
        log("SUBMIT");
        state_->submit(*this);
    }

    void publish() {
        log("PUBLISH");
        state_->publish(*this);
    }

    std::string getState() const {
        return state_->name();
    }
};

// ─────────────────────────────────────────────
// Concrete States
// ─────────────────────────────────────────────

// Draft State
class DraftState : public IDocumentState {
public:
    void edit(DocumentContext& ctx) override {
        std::cout << "[Draft] Editing document\n";
    }

    void submit(DocumentContext& ctx) override;

    void publish(DocumentContext& ctx) override {
        throw std::runtime_error("Cannot publish directly from Draft");
    }

    std::string name() const override { return "DRAFT"; }
};

// Review State
class ReviewState : public IDocumentState {
public:
    void edit(DocumentContext& ctx) override {
        std::cout << "[Review] Editing after review\n";
    }

    void submit(DocumentContext& ctx) override {
        std::cout << "[Review] Already in review\n";
    }

    void publish(DocumentContext& ctx) override;

    std::string name() const override { return "REVIEW"; }
};

// Published State
class PublishedState : public IDocumentState {
public:
    void edit(DocumentContext& ctx) override {
        throw std::runtime_error("Cannot edit published document");
    }

    void submit(DocumentContext& ctx) override {
        throw std::runtime_error("Already published");
    }

    void publish(DocumentContext& ctx) override {
        std::cout << "[Published] Already live\n";
    }

    std::string name() const override { return "PUBLISHED"; }
};

// ─────────────────────────────────────────────
// State Transitions (defined after classes)
// ─────────────────────────────────────────────

void DraftState::submit(DocumentContext& ctx) {
    std::cout << "[Draft] Moving to Review\n";
    ctx.setState(std::make_unique<ReviewState>());
}

void ReviewState::publish(DocumentContext& ctx) {
    std::cout << "[Review] Publishing document\n";
    ctx.setState(std::make_unique<PublishedState>());
}

// ─────────────────────────────────────────────
// Driver — Client interacts with Context
// ─────────────────────────────────────────────
int main() {
    DocumentContext doc(std::make_unique<DraftState>());

    std::cout << "=== Initial State ===\n";
    doc.edit();

    std::cout << "\n=== Submit for Review ===\n";
    doc.submit();

    std::cout << "\n=== Publish ===\n";
    doc.publish();

    std::cout << "\n=== Try Editing Published ===\n";
    try {
        doc.edit();
    } catch (const std::exception& e) {
        std::cout << "Caught: " << e.what() << "\n";
    }
}
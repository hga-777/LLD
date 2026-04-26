#include <iostream>
#include <string>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <stdexcept>

// ─────────────────────────────────────────────
// Roles and Permissions
// ─────────────────────────────────────────────
enum class Role { VIEWER, EDITOR, ADMIN };

std::string roleToStr(Role r) {
    switch(r) {
        case Role::VIEWER: return "VIEWER";
        case Role::EDITOR: return "EDITOR";
        case Role::ADMIN:  return "ADMIN";
    }
    return "UNKNOWN";
}

struct User {
    std::string name;
    Role        role;
};

// ─────────────────────────────────────────────
// IDocumentService — shared interface
// Both RealService and Proxy implement this
// Client talks ONLY to this interface
// ─────────────────────────────────────────────
class IDocumentService {
public:
    virtual std::string readDocument  (const User& user, const std::string& docId) = 0;
    virtual void        writeDocument (const User& user, const std::string& docId,
                                       const std::string& content)                  = 0;
    virtual void        deleteDocument(const User& user, const std::string& docId) = 0;
    virtual ~IDocumentService() = default;
};

// ─────────────────────────────────────────────
// RealDocumentService — actual business logic
// Knows NOTHING about auth — pure SRP
// ─────────────────────────────────────────────
class RealDocumentService : public IDocumentService {
    std::unordered_map<std::string, std::string> storage_;

public:
    std::string readDocument(const User& user, const std::string& docId) override {
        auto it = storage_.find(docId);
        if (it == storage_.end())
            throw std::runtime_error("Document not found: " + docId);
        std::cout << "[RealService] '" << user.name
                  << "' read document '" << docId << "'\n";
        return it->second;
    }

    void writeDocument(const User& user, const std::string& docId,
                       const std::string& content) override {
        storage_[docId] = content;
        std::cout << "[RealService] '" << user.name
                  << "' wrote to document '" << docId << "'\n";
    }

    void deleteDocument(const User& user, const std::string& docId) override {
        if (storage_.erase(docId) == 0)
            throw std::runtime_error("Document not found: " + docId);
        std::cout << "[RealService] '" << user.name
                  << "' deleted document '" << docId << "'\n";
    }
};

// ─────────────────────────────────────────────
// AccessPolicy — pure permission logic
// Separated from proxy so it's independently testable
// ─────────────────────────────────────────────
class AccessPolicy {
public:
    bool canRead  (Role r) const { return true; } // everyone can read
    bool canWrite (Role r) const { return r == Role::EDITOR || r == Role::ADMIN; }
    bool canDelete(Role r) const { return r == Role::ADMIN; }
};

// ─────────────────────────────────────────────
// ProtectionProxy — sits in front of RealService
// Checks permissions BEFORE forwarding
// Logs EVERY attempt — allowed or denied
// ─────────────────────────────────────────────
class DocumentServiceProxy : public IDocumentService {
    std::unique_ptr<IDocumentService> real_;   // the real thing
    AccessPolicy                      policy_;

    // Centralised logger — all attempts go through here
    void log(const User& user, const std::string& op,
             const std::string& docId, bool allowed) const {
        std::cout << "[Proxy][" << (allowed ? "✅ ALLOW" : "❌ DENY ")
                  << "] User='" << user.name
                  << "' Role=" << roleToStr(user.role)
                  << " Op=" << op
                  << " Doc='" << docId << "'\n";
    }

public:
    // Proxy creates and OWNS the real service — lazy init possible here
    DocumentServiceProxy()
        : real_(std::make_unique<RealDocumentService>()) {}

    std::string readDocument(const User& user, const std::string& docId) override {
        bool allowed = policy_.canRead(user.role);
        log(user, "READ", docId, allowed);
        if (!allowed)
            throw std::runtime_error("Access denied: READ requires VIEWER+ role");
        return real_->readDocument(user, docId);
    }

    void writeDocument(const User& user, const std::string& docId,
                       const std::string& content) override {
        bool allowed = policy_.canWrite(user.role);
        log(user, "WRITE", docId, allowed);
        if (!allowed)
            throw std::runtime_error("Access denied: WRITE requires EDITOR+ role");
        real_->writeDocument(user, docId, content);
    }

    void deleteDocument(const User& user, const std::string& docId) override {
        bool allowed = policy_.canDelete(user.role);
        log(user, "DELETE", docId, allowed);
        if (!allowed)
            throw std::runtime_error("Access denied: DELETE requires ADMIN role");
        real_->deleteDocument(user, docId);
    }
};

// ─────────────────────────────────────────────
// Driver — client talks ONLY to IDocumentService
// Never sees RealDocumentService directly
// ─────────────────────────────────────────────
int main() {
    // Client gets proxy — thinks it's talking to real service
    std::unique_ptr<IDocumentService> service =
        std::make_unique<DocumentServiceProxy>();

    User viewer = {"Alice", Role::VIEWER};
    User editor = {"Bob",   Role::EDITOR};
    User admin  = {"Carol", Role::ADMIN};

    // Setup: admin writes initial documents
    std::cout << "=== Setup ===\n";
    service->writeDocument(admin, "doc1", "Q3 Financial Report");
    service->writeDocument(admin, "doc2", "Product Roadmap 2026");

    // Viewer — can read, cannot write or delete
    std::cout << "\n=== Viewer (Alice) ===\n";
    auto content = service->readDocument(viewer, "doc1");
    std::cout << "Content: " << content << "\n";

    try {
        service->writeDocument(viewer, "doc1", "Hacked!");
    } catch (const std::exception& e) {
        std::cout << "Caught: " << e.what() << "\n";
    }

    try {
        service->deleteDocument(viewer, "doc1");
    } catch (const std::exception& e) {
        std::cout << "Caught: " << e.what() << "\n";
    }

    // Editor — can read and write, cannot delete
    std::cout << "\n=== Editor (Bob) ===\n";
    service->writeDocument(editor, "doc2", "Updated Roadmap v2");
    auto updated = service->readDocument(editor, "doc2");
    std::cout << "Content: " << updated << "\n";

    try {
        service->deleteDocument(editor, "doc2");
    } catch (const std::exception& e) {
        std::cout << "Caught: " << e.what() << "\n";
    }

    // Admin — can do everything
    std::cout << "\n=== Admin (Carol) ===\n";
    service->writeDocument(admin, "doc3", "Confidential Strategy");
    service->readDocument(admin, "doc3");
    service->deleteDocument(admin, "doc3");
}
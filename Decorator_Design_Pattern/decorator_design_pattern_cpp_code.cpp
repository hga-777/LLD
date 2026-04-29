#include <iostream>
#include <memory>
#include <string>

// ─────────────────────────────────────────────
// STEP 1: Base Interface
// Every burger — plain or loaded — must have these
// ─────────────────────────────────────────────
class IBurger {
public:
    virtual std::string getDescription() = 0;
    virtual double      getCost()        = 0;
    virtual ~IBurger() = default;
};

// ─────────────────────────────────────────────
// STEP 2: Base Burger — the plain patty
// This is what you START with before any toppings
// ─────────────────────────────────────────────
class PlainBurger : public IBurger {
public:
    std::string getDescription() override { return "Plain Burger"; }
    double      getCost()        override { return 50.0; }
};

// ─────────────────────────────────────────────
// STEP 3: Abstract Decorator
// Think of this as the "tray" that holds whatever
// burger you give it, and lets you add stuff on top
//
// KEY: It IS-A IBurger (so it can be used anywhere)
//      AND HAS-A IBurger (so it wraps another burger)
// ─────────────────────────────────────────────
class ToppingDecorator : public IBurger {
protected:
    std::shared_ptr<IBurger> burger_; // the burger being wrapped
public:
    explicit ToppingDecorator(std::shared_ptr<IBurger> burger)
        : burger_(std::move(burger)) {}
};

// ─────────────────────────────────────────────
// STEP 4: Concrete Toppings — each adds ONE thing
// ─────────────────────────────────────────────

class CheeseTopping : public ToppingDecorator {
public:
    explicit CheeseTopping(std::shared_ptr<IBurger> burger)
        : ToppingDecorator(std::move(burger)) {}

    std::string getDescription() override {
        return burger_->getDescription() + " + Cheese";
    }
    double getCost() override {
        return burger_->getCost() + 15.0;
    }
};

class BaconTopping : public ToppingDecorator {
public:
    explicit BaconTopping(std::shared_ptr<IBurger> burger)
        : ToppingDecorator(std::move(burger)) {}

    std::string getDescription() override {
        return burger_->getDescription() + " + Bacon";
    }
    double getCost() override {
        return burger_->getCost() + 30.0;
    }
};

class SauceTopping : public ToppingDecorator {
public:
    explicit SauceTopping(std::shared_ptr<IBurger> burger)
        : ToppingDecorator(std::move(burger)) {}

    std::string getDescription() override {
        return burger_->getDescription() + " + Sauce";
    }
    double getCost() override {
        return burger_->getCost() + 10.0;
    }
};

// ─────────────────────────────────────────────
// STEP 5: Driver — build burgers dynamically
// ─────────────────────────────────────────────
int main() {
    // Just a plain burger
    std::shared_ptr<IBurger> myBurger = std::make_shared<PlainBurger>();
    std::cout << myBurger->getDescription() << " | Rs." << myBurger->getCost() << "\n";

    // Add cheese on top
    myBurger = std::make_shared<CheeseTopping>(myBurger);
    std::cout << myBurger->getDescription() << " | Rs." << myBurger->getCost() << "\n";

    // Add bacon on top of that
    myBurger = std::make_shared<BaconTopping>(myBurger);
    std::cout << myBurger->getDescription() << " | Rs." << myBurger->getCost() << "\n";

    // Add sauce on top of everything
    myBurger = std::make_shared<SauceTopping>(myBurger);
    std::cout << myBurger->getDescription() << " | Rs." << myBurger->getCost() << "\n";
}
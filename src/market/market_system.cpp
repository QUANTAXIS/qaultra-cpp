#include "qaultra/market/market_system.hpp"
#include <algorithm>
#include <stdexcept>
#include <ctime>

namespace qaultra::market {

// ============ Helper Functions ============

static std::string get_username_impl() {
    const char* user = std::getenv("USER");
    return user ? std::string(user) : "quantaxis";
}

static std::string get_default_datetime() {
    return "1970-01-01 09:30:00";
}

static std::string get_default_date() {
    return "1970-01-01";
}

// ============ Constructors ============

QAMarketSystem::QAMarketSystem()
    : username_(get_username_impl())
    , portfolio_name_("")
    , reg_accounts_()
    , market_data_(std::make_shared<data::QAMarketCenter>())
    , today_(get_default_date())
    , curtime_(get_default_datetime())
    , schedule_queue_()
    , schedule_order_queue_()
    , schedule_target_queue_()
    , cache_()
{
}

QAMarketSystem::QAMarketSystem(const std::string& data_path, const std::string& portfolio_name)
    : username_(get_username_impl())
    , portfolio_name_(portfolio_name)
    , reg_accounts_()
    , market_data_(std::make_shared<data::QAMarketCenter>(data_path))
    , today_(get_default_date())
    , curtime_(get_default_datetime())
    , schedule_queue_()
    , schedule_order_queue_()
    , schedule_target_queue_()
    , cache_()
{
}

QAMarketSystem::QAMarketSystem(std::shared_ptr<data::QAMarketCenter> market_center)
    : username_(get_username_impl())
    , portfolio_name_("")
    , reg_accounts_()
    , market_data_(market_center)
    , today_(get_default_date())
    , curtime_(get_default_datetime())
    , schedule_queue_()
    , schedule_order_queue_()
    , schedule_target_queue_()
    , cache_()
{
}

// ============ Basic Management ============

void QAMarketSystem::reinit() {
    reg_accounts_.clear();
    portfolio_name_ = "";
    today_ = get_default_date();
    curtime_ = get_default_datetime();

    // Clear queues
    std::queue<std::tuple<std::string, std::string, std::unordered_map<std::string, double>, std::string>>().swap(schedule_queue_);
    std::queue<std::tuple<std::string, MarketOrder, std::string>>().swap(schedule_order_queue_);
    std::queue<std::tuple<std::string, std::string, std::unordered_map<std::string, double>, std::string>>().swap(schedule_target_queue_);

    cache_.clear();
}

void QAMarketSystem::save() {
    // TODO: Implement MongoDB save logic
    // For now, this is a placeholder matching Rust save() interface
    // In Rust: saves all QIFI slices to MongoDB
}

// ============ Time Management ============

void QAMarketSystem::set_date(const std::string& date) {
    today_ = date;
    // Update all registered accounts
    for (auto& [name, account] : reg_accounts_) {
        // account->set_datetime(today_);  // Assuming QA_Account has set_datetime
    }
}

void QAMarketSystem::set_datetime(const std::string& datetime) {
    curtime_ = datetime;
    // Update all registered accounts
    for (auto& [name, account] : reg_accounts_) {
        // account->set_datetime(curtime_);
    }
}

// ============ Account Management ============

void QAMarketSystem::register_account(const std::string& account_name, double init_cash) {
    if (reg_accounts_.find(account_name) != reg_accounts_.end()) {
        throw std::runtime_error("Account already registered: " + account_name);
    }

    auto account = std::make_shared<account::QA_Account>(
        account_name,           // account_cookie
        portfolio_name_,        // portfolio_cookie
        username_,              // user_cookie
        init_cash,              // init_cash
        false                   // auto_reload
    );

    reg_accounts_[account_name] = account;
}

void QAMarketSystem::register_account_from_qifi(const protocol::qifi::QIFI& qifi) {
    const std::string& account_name = qifi.account_cookie;

    if (reg_accounts_.find(account_name) != reg_accounts_.end()) {
        throw std::runtime_error("Account already registered: " + account_name);
    }

    // Create account from QIFI data
    auto account = std::make_shared<account::QA_Account>(
        qifi.account_cookie,
        qifi.portfolio,
        qifi.investor_name,
        qifi.money,
        false
    );

    reg_accounts_[account_name] = account;
}

std::shared_ptr<account::QA_Account> QAMarketSystem::get_account(const std::string& account_name) {
    auto it = reg_accounts_.find(account_name);
    if (it == reg_accounts_.end()) {
        throw std::runtime_error("Account not found: " + account_name);
    }
    return it->second;
}

std::shared_ptr<const account::QA_Account> QAMarketSystem::get_account(const std::string& account_name) const {
    auto it = reg_accounts_.find(account_name);
    if (it == reg_accounts_.end()) {
        throw std::runtime_error("Account not found: " + account_name);
    }
    return it->second;
}

std::vector<std::string> QAMarketSystem::get_account_names() const {
    std::vector<std::string> names;
    names.reserve(reg_accounts_.size());
    for (const auto& [name, _] : reg_accounts_) {
        names.push_back(name);
    }
    return names;
}

// ============ Market Data ============

std::vector<data::StockCnDay> QAMarketSystem::get_stock_day(
    const std::string& code,
    const std::string& start_date,
    const std::string& end_date) const
{
    if (!market_data_) {
        throw std::runtime_error("Market data center not initialized");
    }
    return market_data_->get_stock_day(code, start_date, end_date);
}

std::vector<data::StockCn1Min> QAMarketSystem::get_stock_min(
    const std::string& code,
    const std::string& start_datetime,
    const std::string& end_datetime) const
{
    if (!market_data_) {
        throw std::runtime_error("Market data center not initialized");
    }
    return market_data_->get_stock_min(code, start_datetime, end_datetime);
}

// ============ Trading Scheduling ============

void QAMarketSystem::schedule_order(const std::string& account_name,
                                    const MarketOrder& order,
                                    const std::string& label)
{
    schedule_order_queue_.push(std::make_tuple(account_name, order, label));
}

void QAMarketSystem::schedule_position(const std::string& account_name,
                                       const std::string& code,
                                       const std::unordered_map<std::string, double>& positions,
                                       const std::string& label)
{
    schedule_queue_.push(std::make_tuple(account_name, code, positions, label));
}

void QAMarketSystem::schedule_target(const std::string& account_name,
                                     const std::string& code,
                                     const std::unordered_map<std::string, double>& targets,
                                     const std::string& label)
{
    schedule_target_queue_.push(std::make_tuple(account_name, code, targets, label));
}

void QAMarketSystem::process_order_queue() {
    while (!schedule_order_queue_.empty()) {
        auto [account_name, order, label] = schedule_order_queue_.front();
        schedule_order_queue_.pop();

        try {
            auto account = get_account(account_name);
            // Execute order through account
            // account->send_order(order.code, order.amount, order.price, order.direction, order.offset);
        } catch (const std::exception& e) {
            // Log error but continue processing
        }
    }
}

void QAMarketSystem::process_target_queue() {
    while (!schedule_target_queue_.empty()) {
        auto [account_name, code, targets, label] = schedule_target_queue_.front();
        schedule_target_queue_.pop();

        try {
            auto account = get_account(account_name);
            // Process target positions
            // This would calculate difference and generate orders
        } catch (const std::exception& e) {
            // Log error but continue processing
        }
    }
}

void QAMarketSystem::clear_queues() {
    std::queue<std::tuple<std::string, std::string, std::unordered_map<std::string, double>, std::string>>().swap(schedule_queue_);
    std::queue<std::tuple<std::string, MarketOrder, std::string>>().swap(schedule_order_queue_);
    std::queue<std::tuple<std::string, std::string, std::unordered_map<std::string, double>, std::string>>().swap(schedule_target_queue_);
}

// ============ QIFI Snapshot Management ============

void QAMarketSystem::snapshot_all_accounts() {
    for (const auto& [name, account] : reg_accounts_) {
        // Get QIFI snapshot from account
        // protocol::qifi::QIFI qifi = account->get_qifi();
        // cache_[name].push_back(qifi);
    }
}

const std::vector<protocol::qifi::QIFI>& QAMarketSystem::get_account_snapshots(const std::string& account_name) const {
    auto it = cache_.find(account_name);
    if (it == cache_.end()) {
        static std::vector<protocol::qifi::QIFI> empty;
        return empty;
    }
    return it->second;
}

// ============ Backtest Execution ============

void QAMarketSystem::run_backtest(const std::string& start_date,
                                  const std::string& end_date,
                                  std::function<void(QAMarketSystem&)> strategy_func)
{
    // Get trading date list
    // For now, placeholder implementation
    // In production, this would:
    // 1. Get trading calendar from market_data_
    // 2. Iterate through each trading day
    // 3. For each day:
    //    - set_date(date)
    //    - For each minute/bar:
    //      - set_datetime(datetime)
    //      - Call strategy_func(*this)
    //      - process_order_queue()
    //      - snapshot_all_accounts()
}

void QAMarketSystem::step(const std::string& datetime) {
    set_datetime(datetime);
    // Process any pending orders
    process_order_queue();
    process_target_queue();
}

void QAMarketSystem::update_all_prices(const std::unordered_map<std::string, double>& price_map) {
    for (auto& [name, account] : reg_accounts_) {
        for (const auto& [code, price] : price_map) {
            // account->on_price_change(code, price, curtime_);
        }
    }
}

} // namespace qaultra::market

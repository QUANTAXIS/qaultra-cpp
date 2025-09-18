#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <pybind11/chrono.h>

#include "qaultra/engine/backtest_engine.hpp"
#include "qaultra/engine/strategy.hpp"

namespace py = pybind11;
using namespace qaultra;

void bind_engine_types(py::module& m) {
    auto engine = m.def_submodule("engine", "Backtesting and strategy execution engine");

    // Backtest Configuration
    py::class_<engine::BacktestConfig>(engine, "BacktestConfig")
        .def(py::init<>())
        .def_readwrite("start_date", &engine::BacktestConfig::start_date)
        .def_readwrite("end_date", &engine::BacktestConfig::end_date)
        .def_readwrite("initial_cash", &engine::BacktestConfig::initial_cash)
        .def_readwrite("commission_rate", &engine::BacktestConfig::commission_rate)
        .def_readwrite("slippage", &engine::BacktestConfig::slippage)
        .def_readwrite("benchmark", &engine::BacktestConfig::benchmark)
        .def_readwrite("frequency", &engine::BacktestConfig::frequency)
        .def_readwrite("market_data_source", &engine::BacktestConfig::market_data_source)
        .def_readwrite("output_file", &engine::BacktestConfig::output_file)
        .def("__repr__", [](const engine::BacktestConfig& config) {
            return "BacktestConfig(start=" + config.start_date +
                   ", end=" + config.end_date +
                   ", cash=" + std::to_string(config.initial_cash) + ")";
        });

    // Backtest Results
    py::class_<engine::BacktestResults>(engine, "BacktestResults")
        .def(py::init<>())
        .def_readwrite("total_return", &engine::BacktestResults::total_return)
        .def_readwrite("annual_return", &engine::BacktestResults::annual_return)
        .def_readwrite("sharpe_ratio", &engine::BacktestResults::sharpe_ratio)
        .def_readwrite("max_drawdown", &engine::BacktestResults::max_drawdown)
        .def_readwrite("volatility", &engine::BacktestResults::volatility)
        .def_readwrite("win_rate", &engine::BacktestResults::win_rate)
        .def_readwrite("profit_factor", &engine::BacktestResults::profit_factor)
        .def_readwrite("total_trades", &engine::BacktestResults::total_trades)
        .def_readwrite("final_value", &engine::BacktestResults::final_value)
        .def_readwrite("equity_curve", &engine::BacktestResults::equity_curve)
        .def_readwrite("trade_list", &engine::BacktestResults::trade_list)
        .def_readwrite("daily_returns", &engine::BacktestResults::daily_returns)
        .def("__repr__", [](const engine::BacktestResults& results) {
            return "BacktestResults(return=" + std::to_string(results.total_return) +
                   ", sharpe=" + std::to_string(results.sharpe_ratio) +
                   ", trades=" + std::to_string(results.total_trades) + ")";
        });

    // Strategy Context
    py::class_<engine::StrategyContext>(engine, "StrategyContext")
        .def(py::init<>())
        .def_readwrite("current_date", &engine::StrategyContext::current_date)
        .def_readwrite("current_price", &engine::StrategyContext::current_price)
        .def_readwrite("account", &engine::StrategyContext::account)
        .def_readwrite("universe", &engine::StrategyContext::universe)
        .def("get_price", &engine::StrategyContext::get_price,
            "Get current price for symbol",
            py::arg("symbol"))
        .def("get_history", &engine::StrategyContext::get_history,
            "Get historical data for symbol",
            py::arg("symbol"), py::arg("window"), py::arg("field") = "close")
        .def("get_position", &engine::StrategyContext::get_position,
            "Get current position for symbol",
            py::arg("symbol"))
        .def("get_cash", &engine::StrategyContext::get_cash,
            "Get available cash")
        .def("get_portfolio_value", &engine::StrategyContext::get_portfolio_value,
            "Get total portfolio value")
        .def("log", &engine::StrategyContext::log,
            "Log message with timestamp",
            py::arg("message"));

    // Base Strategy (abstract)
    py::class_<engine::Strategy>(engine, "Strategy")
        .def(py::init<>())
        .def("initialize", &engine::Strategy::initialize,
            "Initialize strategy",
            py::arg("context"))
        .def("handle_data", &engine::Strategy::handle_data,
            "Handle data event",
            py::arg("context"))
        .def("before_market_open", &engine::Strategy::before_market_open,
            "Called before market opens",
            py::arg("context"))
        .def("after_market_close", &engine::Strategy::after_market_close,
            "Called after market closes",
            py::arg("context"))
        .def("get_name", &engine::Strategy::get_name,
            "Get strategy name")
        .def("get_parameters", &engine::Strategy::get_parameters,
            "Get strategy parameters")
        .def("set_parameter", &engine::Strategy::set_parameter,
            "Set strategy parameter",
            py::arg("name"), py::arg("value"));

    // Simple Moving Average Strategy
    py::class_<engine::SMAStrategy, engine::Strategy>(engine, "SMAStrategy")
        .def(py::init<int, int>(),
            "Create SMA crossover strategy",
            py::arg("fast_window"), py::arg("slow_window"))
        .def_readwrite("fast_window", &engine::SMAStrategy::fast_window)
        .def_readwrite("slow_window", &engine::SMAStrategy::slow_window);

    // Momentum Strategy
    py::class_<engine::MomentumStrategy, engine::Strategy>(engine, "MomentumStrategy")
        .def(py::init<int, double>(),
            "Create momentum strategy",
            py::arg("lookback_window"), py::arg("threshold"))
        .def_readwrite("lookback_window", &engine::MomentumStrategy::lookback_window)
        .def_readwrite("threshold", &engine::MomentumStrategy::threshold);

    // Mean Reversion Strategy
    py::class_<engine::MeanReversionStrategy, engine::Strategy>(engine, "MeanReversionStrategy")
        .def(py::init<int, double>(),
            "Create mean reversion strategy",
            py::arg("window"), py::arg("z_score_threshold"))
        .def_readwrite("window", &engine::MeanReversionStrategy::window)
        .def_readwrite("z_score_threshold", &engine::MeanReversionStrategy::z_score_threshold);

    // Backtest Engine
    py::class_<engine::BacktestEngine>(engine, "BacktestEngine")
        .def(py::init<const engine::BacktestConfig&>(),
            "Create backtest engine",
            py::arg("config"))
        .def("add_strategy", &engine::BacktestEngine::add_strategy,
            "Add strategy to backtest",
            py::arg("strategy"))
        .def("set_universe", &engine::BacktestEngine::set_universe,
            "Set trading universe",
            py::arg("symbols"))
        .def("load_data", &engine::BacktestEngine::load_data,
            "Load market data",
            py::arg("data_source"))
        .def("run", &engine::BacktestEngine::run,
            "Run backtest")
        .def("get_results", &engine::BacktestEngine::get_results,
            "Get backtest results")
        .def("save_results", &engine::BacktestEngine::save_results,
            "Save results to file",
            py::arg("filename"))
        .def("get_performance_summary", &engine::BacktestEngine::get_performance_summary,
            "Get performance summary")
        .def("plot_equity_curve", &engine::BacktestEngine::plot_equity_curve,
            "Plot equity curve (returns matplotlib figure handle)")
        .def("get_trade_analysis", &engine::BacktestEngine::get_trade_analysis,
            "Get detailed trade analysis");

    // Strategy Factory
    auto factory = engine.def_submodule("factory", "Strategy factory functions");

    factory.def("create_sma_strategy", [](int fast, int slow) {
        return std::make_shared<engine::SMAStrategy>(fast, slow);
    }, "Create SMA crossover strategy",
       py::arg("fast_window"), py::arg("slow_window"));

    factory.def("create_momentum_strategy", [](int window, double threshold) {
        return std::make_shared<engine::MomentumStrategy>(window, threshold);
    }, "Create momentum strategy",
       py::arg("lookback_window"), py::arg("threshold"));

    factory.def("create_mean_reversion_strategy", [](int window, double threshold) {
        return std::make_shared<engine::MeanReversionStrategy>(window, threshold);
    }, "Create mean reversion strategy",
       py::arg("window"), py::arg("z_score_threshold"));

    // Strategy utilities
    engine.def("calculate_sharpe_ratio", [](const std::vector<double>& returns, double risk_free_rate = 0.0) {
        if (returns.empty()) return 0.0;

        double mean_return = 0.0;
        for (double ret : returns) {
            mean_return += ret;
        }
        mean_return /= returns.size();

        double variance = 0.0;
        for (double ret : returns) {
            variance += (ret - mean_return) * (ret - mean_return);
        }
        variance /= returns.size();

        double std_dev = std::sqrt(variance);
        return std_dev > 0 ? (mean_return - risk_free_rate) / std_dev : 0.0;
    }, "Calculate Sharpe ratio from returns",
       py::arg("returns"), py::arg("risk_free_rate") = 0.0);

    engine.def("calculate_max_drawdown", [](const std::vector<double>& equity_curve) {
        if (equity_curve.empty()) return 0.0;

        double max_value = equity_curve[0];
        double max_drawdown = 0.0;

        for (double value : equity_curve) {
            if (value > max_value) {
                max_value = value;
            }

            double drawdown = (max_value - value) / max_value;
            if (drawdown > max_drawdown) {
                max_drawdown = drawdown;
            }
        }

        return max_drawdown;
    }, "Calculate maximum drawdown from equity curve",
       py::arg("equity_curve"));

    engine.def("calculate_volatility", [](const std::vector<double>& returns, bool annualized = true) {
        if (returns.empty()) return 0.0;

        double mean_return = 0.0;
        for (double ret : returns) {
            mean_return += ret;
        }
        mean_return /= returns.size();

        double variance = 0.0;
        for (double ret : returns) {
            variance += (ret - mean_return) * (ret - mean_return);
        }
        variance /= returns.size();

        double volatility = std::sqrt(variance);
        return annualized ? volatility * std::sqrt(252) : volatility; // 252 trading days
    }, "Calculate volatility from returns",
       py::arg("returns"), py::arg("annualized") = true);

    engine.def("calculate_win_rate", [](const std::vector<double>& trade_returns) {
        if (trade_returns.empty()) return 0.0;

        int winning_trades = 0;
        for (double ret : trade_returns) {
            if (ret > 0) {
                winning_trades++;
            }
        }

        return static_cast<double>(winning_trades) / trade_returns.size();
    }, "Calculate win rate from trade returns",
       py::arg("trade_returns"));

    engine.def("calculate_profit_factor", [](const std::vector<double>& trade_returns) {
        if (trade_returns.empty()) return 0.0;

        double total_profit = 0.0;
        double total_loss = 0.0;

        for (double ret : trade_returns) {
            if (ret > 0) {
                total_profit += ret;
            } else {
                total_loss += std::abs(ret);
            }
        }

        return total_loss > 0 ? total_profit / total_loss : 0.0;
    }, "Calculate profit factor from trade returns",
       py::arg("trade_returns"));

    // Performance attribution analysis
    engine.def("calculate_rolling_sharpe", [](const std::vector<double>& returns, int window) {
        std::vector<double> rolling_sharpe;

        if (returns.size() < static_cast<size_t>(window)) {
            return rolling_sharpe;
        }

        for (size_t i = window - 1; i < returns.size(); ++i) {
            std::vector<double> window_returns(returns.begin() + i - window + 1, returns.begin() + i + 1);

            double mean_return = 0.0;
            for (double ret : window_returns) {
                mean_return += ret;
            }
            mean_return /= window_returns.size();

            double variance = 0.0;
            for (double ret : window_returns) {
                variance += (ret - mean_return) * (ret - mean_return);
            }
            variance /= window_returns.size();

            double std_dev = std::sqrt(variance);
            double sharpe = std_dev > 0 ? mean_return / std_dev : 0.0;
            rolling_sharpe.push_back(sharpe);
        }

        return rolling_sharpe;
    }, "Calculate rolling Sharpe ratio",
       py::arg("returns"), py::arg("window"));

    // Benchmark comparison utilities
    engine.def("calculate_beta", [](const std::vector<double>& strategy_returns,
                                   const std::vector<double>& benchmark_returns) {
        if (strategy_returns.size() != benchmark_returns.size() || strategy_returns.empty()) {
            return 0.0;
        }

        double strategy_mean = 0.0, benchmark_mean = 0.0;
        for (size_t i = 0; i < strategy_returns.size(); ++i) {
            strategy_mean += strategy_returns[i];
            benchmark_mean += benchmark_returns[i];
        }
        strategy_mean /= strategy_returns.size();
        benchmark_mean /= benchmark_returns.size();

        double covariance = 0.0, benchmark_variance = 0.0;
        for (size_t i = 0; i < strategy_returns.size(); ++i) {
            double strategy_diff = strategy_returns[i] - strategy_mean;
            double benchmark_diff = benchmark_returns[i] - benchmark_mean;
            covariance += strategy_diff * benchmark_diff;
            benchmark_variance += benchmark_diff * benchmark_diff;
        }

        return benchmark_variance > 0 ? covariance / benchmark_variance : 0.0;
    }, "Calculate beta relative to benchmark",
       py::arg("strategy_returns"), py::arg("benchmark_returns"));

    engine.def("calculate_alpha", [](const std::vector<double>& strategy_returns,
                                    const std::vector<double>& benchmark_returns,
                                    double risk_free_rate = 0.0) {
        if (strategy_returns.size() != benchmark_returns.size() || strategy_returns.empty()) {
            return 0.0;
        }

        // Calculate beta first
        double strategy_mean = 0.0, benchmark_mean = 0.0;
        for (size_t i = 0; i < strategy_returns.size(); ++i) {
            strategy_mean += strategy_returns[i];
            benchmark_mean += benchmark_returns[i];
        }
        strategy_mean /= strategy_returns.size();
        benchmark_mean /= benchmark_returns.size();

        double covariance = 0.0, benchmark_variance = 0.0;
        for (size_t i = 0; i < strategy_returns.size(); ++i) {
            double strategy_diff = strategy_returns[i] - strategy_mean;
            double benchmark_diff = benchmark_returns[i] - benchmark_mean;
            covariance += strategy_diff * benchmark_diff;
            benchmark_variance += benchmark_diff * benchmark_diff;
        }

        double beta = benchmark_variance > 0 ? covariance / benchmark_variance : 0.0;

        // Alpha = Strategy Return - (Risk Free Rate + Beta * (Benchmark Return - Risk Free Rate))
        return strategy_mean - (risk_free_rate + beta * (benchmark_mean - risk_free_rate));
    }, "Calculate alpha relative to benchmark",
       py::arg("strategy_returns"), py::arg("benchmark_returns"), py::arg("risk_free_rate") = 0.0);
}
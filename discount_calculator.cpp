#include <iostream>
#include <memory>
#include <vector>
#include <numeric>

// --- 1. 抽象クラス ---
// 割引戦略を定義する抽象クラス
class DiscountStrategy {
public:
    // 純粋仮想関数。このクラスを継承するクラスは必ずこのメソッドを実装する必要がある
    virtual double apply_discount(double total_price) const = 0;
    // 仮想デストラクタ。派生クラスのデストラクタが正しく呼び出されるようにする
    virtual ~DiscountStrategy() = default;
};

// --- 2. 派生クラス（戦略の実装） ---
// 固定割引率を適用する戦略
class PercentageDiscount : public DiscountStrategy {
private:
    double percentage;

public:
    PercentageDiscount(double p) : percentage(p) {}
    
    double apply_discount(double total_price) const override {
        return total_price * (1.0 - percentage);
    }
};

// 特定の金額以上で固定額を割り引く戦略
class ThresholdDiscount : public DiscountStrategy {
private:
    double threshold;
    double deduction;

public:
    ThresholdDiscount(double t, double d) : threshold(t), deduction(d) {}
    
    double apply_discount(double total_price) const override {
        if (total_price >= threshold) {
            return total_price - deduction;
        }
        return total_price;
    }
};

// --- 3. コンテキストクラス ---
// 割引戦略を適用するクラス
class DiscountCalculator {
private:
    // スマートポインタで戦略オブジェクトを所有
    std::unique_ptr<DiscountStrategy> strategy;

public:
    DiscountCalculator(std::unique_ptr<DiscountStrategy> s) : strategy(std::move(s)) {}
    
    // 戦略を動的に変更するメソッド
    void set_strategy(std::unique_ptr<DiscountStrategy> s) {
        strategy = std::move(s);
    }
    
    double calculate_final_price(const std::vector<double>& prices) const {
        // 価格リストの合計を計算
        double total_price = std::accumulate(prices.begin(), prices.end(), 0.0);
        
        // ポリモーフィズムにより、strategyが指すオブジェクトのapply_discountが呼び出される
        return strategy->apply_discount(total_price);
    }
};

// --- 4. メイン関数 ---
int main() {
    std::vector<double> product_prices = {100.0, 250.0, 50.0};
    
    // --- 1. 10%割引戦略を適用 ---
    // スマートポインタでPercentageDiscountオブジェクトを生成
    auto percentage_strategy = std::make_unique<PercentageDiscount>(0.10);
    DiscountCalculator calculator(std::move(percentage_strategy));
    
    double final_price_1 = calculator.calculate_final_price(product_prices);
    std::cout << "合計価格: " << std::accumulate(product_prices.begin(), product_prices.end(), 0.0) << "円" << std::endl;
    std::cout << "10%割引適用後の価格: " << final_price_1 << "円" << std::endl;
    std::cout << "--------------------" << std::endl;
    
    // --- 2. 300円以上で50円引き戦略に切り替え ---
    auto threshold_strategy = std::make_unique<ThresholdDiscount>(300.0, 50.0);
    calculator.set_strategy(std::move(threshold_strategy));
    
    double final_price_2 = calculator.calculate_final_price(product_prices);
    std::cout << "合計価格: " << std::accumulate(product_prices.begin(), product_prices.end(), 0.0) << "円" << std::endl;
    std::cout << "300円以上で50円引き適用後の価格: " << final_price_2 << "円" << std::endl;
    
    // スコープを抜けると、unique_ptrが自動的にメモリを解放する
    return 0;
}

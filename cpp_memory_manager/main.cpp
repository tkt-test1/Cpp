// main.cpp
//
// 【処理概要】
// メモリプールアロケータとLRUキャッシュのデモプログラム。
// マルチスレッド環境での動作を検証する。
//
// 【主な機能】
// - メモリプールからの高速なメモリ確保/解放
// - LRUキャッシュによるデータの自動追い出し
// - 複数スレッドでの同時アクセステスト
// - パフォーマンス計測
//
// 【実装内容】
// 1. メモリプールを初期化
// 2. LRUキャッシュを初期化
// 3. シングルスレッドでの動作確認
// 4. マルチスレッドでの負荷テスト
// 5. 統計情報の表示

#include "memory_pool.hpp"
#include "lru_cache.hpp"
#include "thread_safe_allocator.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <random>
#include <iomanip>

// テスト用データ構造
struct UserData {
    int id;
    char name[32];
    double score;
    
    UserData() : id(0), score(0.0) {
        std::memset(name, 0, sizeof(name));
    }
    
    UserData(int i, const std::string& n, double s) : id(i), score(s) {
        std::strncpy(name, n.c_str(), sizeof(name) - 1);
    }
};

// ===== シングルスレッドテスト =====

void test_memory_pool() {
    std::cout << "=== Memory Pool Test ===\n\n";
    
    // 64バイトのブロックを100個持つメモリプール
    MemoryPool pool(64, 100);
    
    std::cout << "📊 Initial pool stats:\n";
    pool.print_stats();
    
    // メモリ確保テスト
    std::vector<void*> allocations;
    
    std::cout << "\n🔧 Allocating 10 blocks...\n";
    for (int i = 0; i < 10; ++i) {
        void* ptr = pool.allocate();
        if (ptr) {
            allocations.push_back(ptr);
            // データを書き込んでみる
            UserData* user = new(ptr) UserData(i, "User" + std::to_string(i), i * 10.5);
        }
    }
    
    pool.print_stats();
    
    // 解放テスト
    std::cout << "\n🗑️  Deallocating 5 blocks...\n";
    for (int i = 0; i < 5; ++i) {
        pool.deallocate(allocations[i]);
    }
    
    pool.print_stats();
    
    // 残りを解放
    std::cout << "\n🗑️  Deallocating remaining blocks...\n";
    for (size_t i = 5; i < allocations.size(); ++i) {
        pool.deallocate(allocations[i]);
    }
    
    pool.print_stats();
}

void test_lru_cache() {
    std::cout << "\n\n=== LRU Cache Test ===\n\n";
    
    // 容量3のLRUキャッシュ
    LRUCache<int, std::string> cache(3);
    
    std::cout << "📦 Cache capacity: 3\n\n";
    
    // データ追加
    std::cout << "➕ Adding items...\n";
    cache.put(1, "Alice");
    cache.put(2, "Bob");
    cache.put(3, "Charlie");
    cache.print_stats();
    
    // 取得テスト
    std::cout << "\n🔍 Getting key 2...\n";
    if (auto val = cache.get(2)) {
        std::cout << "   Found: " << *val << "\n";
    }
    
    // 容量オーバー（LRU追い出しが発生）
    std::cout << "\n➕ Adding key 4 (eviction should occur)...\n";
    cache.put(4, "Diana");
    cache.print_stats();
    
    // 追い出されたキーを確認
    std::cout << "\n🔍 Checking evicted key 1...\n";
    if (auto val = cache.get(1)) {
        std::cout << "   Found: " << *val << "\n";
    } else {
        std::cout << "   ❌ Key 1 was evicted (as expected)\n";
    }
}

// ===== マルチスレッドテスト =====

void worker_thread(ThreadSafeAllocator<UserData>& allocator, int thread_id, int iterations) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 100);
    
    std::vector<UserData*> local_allocations;
    
    for (int i = 0; i < iterations; ++i) {
        // ランダムに確保または解放
        if (dis(gen) < 50 && !local_allocations.empty()) {
            // 解放
            auto* ptr = local_allocations.back();
            local_allocations.pop_back();
            allocator.deallocate(ptr);
        } else {
            // 確保
            auto* ptr = allocator.allocate();
            if (ptr) {
                new(ptr) UserData(thread_id * 1000 + i, "Thread" + std::to_string(thread_id), i * 1.5);
                local_allocations.push_back(ptr);
            }
        }
    }
    
    // 残りを全て解放
    for (auto* ptr : local_allocations) {
        allocator.deallocate(ptr);
    }
}

void test_multithread() {
    std::cout << "\n\n=== Multi-threaded Stress Test ===\n\n";
    
    const int NUM_THREADS = 4;
    const int ITERATIONS_PER_THREAD = 1000;
    
    // スレッドセーフアロケータ（1000個のUserDataを管理）
    ThreadSafeAllocator<UserData> allocator(1000);
    
    std::cout << "🧵 Starting " << NUM_THREADS << " threads...\n";
    std::cout << "🔄 Each thread: " << ITERATIONS_PER_THREAD << " operations\n\n";
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // スレッド起動
    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(worker_thread, std::ref(allocator), i, ITERATIONS_PER_THREAD);
    }
    
    // 全スレッド終了待機
    for (auto& t : threads) {
        t.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "✅ All threads completed\n";
    std::cout << "⏱️  Total time: " << duration.count() << " ms\n\n";
    
    allocator.print_stats();
}

// ===== パフォーマンス比較 =====

void benchmark_comparison() {
    std::cout << "\n\n=== Performance Benchmark ===\n\n";
    
    const int ALLOCATIONS = 10000;
    
    // 標準new/deleteでのベンチマーク
    {
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<UserData*> ptrs;
        for (int i = 0; i < ALLOCATIONS; ++i) {
            ptrs.push_back(new UserData(i, "Test", i * 1.0));
        }
        for (auto* ptr : ptrs) {
            delete ptr;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "🐢 Standard new/delete: " << duration.count() << " μs\n";
    }
    
    // カスタムアロケータでのベンチマーク
    {
        ThreadSafeAllocator<UserData> allocator(ALLOCATIONS);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<UserData*> ptrs;
        for (int i = 0; i < ALLOCATIONS; ++i) {
            auto* ptr = allocator.allocate();
            new(ptr) UserData(i, "Test", i * 1.0);
            ptrs.push_back(ptr);
        }
        for (auto* ptr : ptrs) {
            allocator.deallocate(ptr);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "🚀 Memory pool: " << duration.count() << " μs\n";
    }
}

// ===== メイン =====

int main() {
    std::cout << "╔════════════════════════════════════════════╗\n";
    std::cout << "║  C++ Advanced Memory Manager Demo         ║\n";
    std::cout << "║  - Memory Pool Allocator                   ║\n";
    std::cout << "║  - LRU Cache                               ║\n";
    std::cout << "║  - Thread-safe Operations                  ║\n";
    std::cout << "╚════════════════════════════════════════════╝\n\n";
    
    try {
        // 各テストを実行
        test_memory_pool();
        test_lru_cache();
        test_multithread();
        benchmark_comparison();
        
        std::cout << "\n\n✨ All tests completed successfully!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}

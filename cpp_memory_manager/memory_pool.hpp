// memory_pool.hpp
//
// 【処理概要】
// 固定サイズブロックのメモリプールアロケータ。
// 事前に大きなメモリ領域を確保し、小さなブロックに分割して管理する。
//
// 【主な機能】
// - 高速なメモリ確保/解放（O(1)）
// - メモリ断片化の防止
// - フリーリストによる空きブロック管理
// - 統計情報の収集
//
// 【実装内容】
// 1. コンストラクタで大きなメモリブロックを確保
// 2. ブロックをフリーリストで連結
// 3. allocate()で先頭から取り出し
// 4. deallocate()でフリーリストに戻す
// 5. デストラクタで全メモリを解放

#ifndef MEMORY_POOL_HPP
#define MEMORY_POOL_HPP

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <iostream>

/// 固定サイズメモリプールアロケータ
class MemoryPool {
private:
    // フリーリストのノード（各ブロックの先頭に配置）
    struct FreeBlock {
        FreeBlock* next;
    };
    
    void* memory_start_;        // メモリプール全体の開始アドレス
    size_t block_size_;         // 1ブロックのサイズ（バイト）
    size_t block_count_;        // 総ブロック数
    FreeBlock* free_list_;      // 空きブロックのリスト
    
    // 統計情報
    size_t allocations_;        // 確保回数
    size_t deallocations_;      // 解放回数
    size_t current_usage_;      // 現在使用中のブロック数

public:
    /// コンストラクタ
    /// block_size: 各ブロックのサイズ（バイト）
    /// block_count: 総ブロック数
    MemoryPool(size_t block_size, size_t block_count)
        : block_size_(block_size)
        , block_count_(block_count)
        , allocations_(0)
        , deallocations_(0)
        , current_usage_(0)
    {
        // ブロックサイズはFreeBlock以上必要
        if (block_size_ < sizeof(FreeBlock)) {
            block_size_ = sizeof(FreeBlock);
        }
        
        // メモリプール全体を確保
        size_t total_size = block_size_ * block_count_;
        memory_start_ = ::operator new(total_size);
        
        if (!memory_start_) {
            throw std::bad_alloc();
        }
        
        // フリーリストを初期化（全ブロックを連結）
        init_free_list();
        
        std::cout << "💾 Memory pool created: "
                  << block_count_ << " blocks × "
                  << block_size_ << " bytes = "
                  << total_size << " bytes\n";
    }
    
    /// デストラクタ
    ~MemoryPool() {
        // メモリプール全体を解放
        ::operator delete(memory_start_);
        
        std::cout << "🗑️  Memory pool destroyed\n";
    }
    
    // コピー禁止
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;
    
    /// メモリブロックを確保
    /// 戻り値: 確保したメモリのポインタ（失敗時はnullptr）
    void* allocate() {
        if (!free_list_) {
            // 空きブロックなし
            std::cerr << "⚠️  Memory pool exhausted!\n";
            return nullptr;
        }
        
        // フリーリストの先頭を取り出す
        FreeBlock* block = free_list_;
        free_list_ = block->next;
        
        // 統計更新
        ++allocations_;
        ++current_usage_;
        
        return block;
    }
    
    /// メモリブロックを解放
    /// ptr: 解放するメモリのポインタ
    void deallocate(void* ptr) {
        if (!ptr) {
            return;
        }
        
        // プール範囲外のポインタチェック
        if (!is_from_pool(ptr)) {
            std::cerr << "⚠️  Attempt to deallocate pointer not from this pool!\n";
            return;
        }
        
        // フリーリストの先頭に戻す
        FreeBlock* block = static_cast<FreeBlock*>(ptr);
        block->next = free_list_;
        free_list_ = block;
        
        // 統計更新
        ++deallocations_;
        --current_usage_;
    }
    
    /// 統計情報を表示
    void print_stats() const {
        std::cout << "📊 Pool Stats:\n";
        std::cout << "   Total blocks: " << block_count_ << "\n";
        std::cout << "   Block size: " << block_size_ << " bytes\n";
        std::cout << "   Used blocks: " << current_usage_ << "\n";
        std::cout << "   Free blocks: " << (block_count_ - current_usage_) << "\n";
        std::cout << "   Total allocations: " << allocations_ << "\n";
        std::cout << "   Total deallocations: " << deallocations_ << "\n";
        std::cout << "   Usage: " 
                  << (current_usage_ * 100.0 / block_count_) << "%\n";
    }
    
    /// 現在の使用ブロック数を取得
    size_t current_usage() const {
        return current_usage_;
    }
    
    /// 総ブロック数を取得
    size_t capacity() const {
        return block_count_;
    }

private:
    /// フリーリストを初期化
    /// 全ブロックを連結リストで繋ぐ
    void init_free_list() {
        free_list_ = nullptr;
        
        // 後ろから前に向かって連結（先頭が最初に使われる）
        char* current = static_cast<char*>(memory_start_) + (block_count_ - 1) * block_size_;
        
        for (size_t i = 0; i < block_count_; ++i) {
            FreeBlock* block = reinterpret_cast<FreeBlock*>(current);
            block->next = free_list_;
            free_list_ = block;
            current -= block_size_;
        }
    }
    
    /// ポインタがこのプールのものか確認
    bool is_from_pool(void* ptr) const {
        char* p = static_cast<char*>(ptr);
        char* start = static_cast<char*>(memory_start_);
        char* end = start + (block_size_ * block_count_);
        
        return (p >= start && p < end);
    }
};

#endif // MEMORY_POOL_HPP

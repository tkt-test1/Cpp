// lru_cache.hpp
//
// 【処理概要】
// LRU (Least Recently Used) キャッシュの実装。
// 最近使われていないデータを自動的に追い出す。
//
// 【主な機能】
// - O(1)でのデータ取得/追加
// - 容量超過時の自動追い出し（LRU方式）
// - ハッシュマップ + 双方向リストによる実装
// - キャッシュヒット率の統計
//
// 【実装内容】
// 1. 双方向リストで使用順序を管理（最近使用=先頭、古い=末尾）
// 2. ハッシュマップで高速検索
// 3. get時にリストの先頭に移動（最近使用マーク）
// 4. put時に容量超過なら末尾を削除
// 5. テンプレートで任意の型に対応

#ifndef LRU_CACHE_HPP
#define LRU_CACHE_HPP

#include <unordered_map>
#include <list>
#include <optional>
#include <iostream>

/// LRUキャッシュ
/// Key: キーの型
/// Value: 値の型
template<typename Key, typename Value>
class LRUCache {
private:
    // キーと値のペアを格納するリスト（最近使用=先頭）
    using CacheItem = std::pair<Key, Value>;
    using CacheList = std::list<CacheItem>;
    using ListIterator = typename CacheList::iterator;
    
    size_t capacity_;                              // 最大容量
    CacheList cache_list_;                         // データリスト
    std::unordered_map<Key, ListIterator> cache_map_;  // キー→イテレータのマップ
    
    // 統計情報
    size_t hits_;                                  // キャッシュヒット数
    size_t misses_;                                // キャッシュミス数
    size_t evictions_;                             // 追い出し回数

public:
    /// コンストラクタ
    /// capacity: キャッシュの最大容量
    explicit LRUCache(size_t capacity)
        : capacity_(capacity)
        , hits_(0)
        , misses_(0)
        , evictions_(0)
    {
        if (capacity_ == 0) {
            throw std::invalid_argument("Cache capacity must be > 0");
        }
    }
    
    /// 値を取得
    /// key: 取得するキー
    /// 戻り値: 値が存在すればstd::optional<Value>、なければstd::nullopt
    std::optional<Value> get(const Key& key) {
        auto it = cache_map_.find(key);
        
        if (it == cache_map_.end()) {
            // キャッシュミス
            ++misses_;
            return std::nullopt;
        }
        
        // キャッシュヒット
        ++hits_;
        
        // リストの先頭に移動（最近使用マーク）
        cache_list_.splice(cache_list_.begin(), cache_list_, it->second);
        
        return it->second->second;
    }
    
    /// 値を追加/更新
    /// key: キー
    /// value: 値
    void put(const Key& key, const Value& value) {
        auto it = cache_map_.find(key);
        
        if (it != cache_map_.end()) {
            // 既存キーの更新
            it->second->second = value;
            // リストの先頭に移動
            cache_list_.splice(cache_list_.begin(), cache_list_, it->second);
            return;
        }
        
        // 新規追加
        if (cache_list_.size() >= capacity_) {
            // 容量オーバー：末尾（最も古いもの）を削除
            evict_lru();
        }
        
        // 先頭に追加
        cache_list_.emplace_front(key, value);
        cache_map_[key] = cache_list_.begin();
    }
    
    /// キャッシュをクリア
    void clear() {
        cache_list_.clear();
        cache_map_.clear();
    }
    
    /// 現在のサイズを取得
    size_t size() const {
        return cache_list_.size();
    }
    
    /// 容量を取得
    size_t capacity() const {
        return capacity_;
    }
    
    /// 統計情報を表示
    void print_stats() const {
        std::cout << "📊 Cache Stats:\n";
        std::cout << "   Capacity: " << capacity_ << "\n";
        std::cout << "   Current size: " << cache_list_.size() << "\n";
        std::cout << "   Hits: " << hits_ << "\n";
        std::cout << "   Misses: " << misses_ << "\n";
        std::cout << "   Evictions: " << evictions_ << "\n";
        
        if (hits_ + misses_ > 0) {
            double hit_rate = hits_ * 100.0 / (hits_ + misses_);
            std::cout << "   Hit rate: " << hit_rate << "%\n";
        }
        
        // キャッシュ内容を表示（最近使用順）
        std::cout << "   Contents (recent → old):\n";
        for (const auto& item : cache_list_) {
            std::cout << "      " << item.first << " => " << item.second << "\n";
        }
    }
    
    /// キャッシュヒット率を取得
    double hit_rate() const {
        if (hits_ + misses_ == 0) {
            return 0.0;
        }
        return hits_ * 100.0 / (hits_ + misses_);
    }
    
    /// 統計をリセット
    void reset_stats() {
        hits_ = 0;
        misses_ = 0;
        evictions_ = 0;
    }

private:
    /// LRU（最も古い）要素を追い出す
    void evict_lru() {
        if (cache_list_.empty()) {
            return;
        }
        
        // リストの末尾（最も古い）を削除
        const Key& key = cache_list_.back().first;
        cache_map_.erase(key);
        cache_list_.pop_back();
        
        ++evictions_;
    }
};

/// ポインタを値として持つ特殊化版（メモリ管理用）
template<typename Key, typename Value>
class LRUCachePtr {
private:
    using CacheItem = std::pair<Key, Value*>;
    using CacheList = std::list<CacheItem>;
    using ListIterator = typename CacheList::iterator;
    
    size_t capacity_;
    CacheList cache_list_;
    std::unordered_map<Key, ListIterator> cache_map_;
    
    // 削除時のコールバック（メモリ解放用）
    std::function<void(Value*)> deleter_;

public:
    explicit LRUCachePtr(size_t capacity, std::function<void(Value*)> deleter = nullptr)
        : capacity_(capacity)
        , deleter_(deleter)
    {
        if (capacity_ == 0) {
            throw std::invalid_argument("Cache capacity must be > 0");
        }
    }
    
    ~LRUCachePtr() {
        // 全要素を削除
        if (deleter_) {
            for (auto& item : cache_list_) {
                deleter_(item.second);
            }
        }
    }
    
    Value* get(const Key& key) {
        auto it = cache_map_.find(key);
        
        if (it == cache_map_.end()) {
            return nullptr;
        }
        
        // リストの先頭に移動
        cache_list_.splice(cache_list_.begin(), cache_list_, it->second);
        
        return it->second->second;
    }
    
    void put(const Key& key, Value* value) {
        auto it = cache_map_.find(key);
        
        if (it != cache_map_.end()) {
            // 既存値を削除
            if (deleter_) {
                deleter_(it->second->second);
            }
            it->second->second = value;
            cache_list_.splice(cache_list_.begin(), cache_list_, it->second);
            return;
        }
        
        if (cache_list_.size() >= capacity_) {
            // 末尾を追い出し
            if (deleter_) {
                deleter_(cache_list_.back().second);
            }
            const Key& old_key = cache_list_.back().first;
            cache_map_.erase(old_key);
            cache_list_.pop_back();
        }
        
        cache_list_.emplace_front(key, value);
        cache_map_[key] = cache_list_.begin();
    }
};

#endif // LRU_CACHE_HPP

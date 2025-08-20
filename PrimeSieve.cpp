// PrimeSieve.cpp
//
// 概要:
//   エラトステネスの篩を使って指定範囲内の素数を列挙するサンプル
//
// 実装機能:
//   - 引数 n に対して、2〜n の素数を出力
//   - 計算量 O(n log log n)

#include <iostream>
#include <vector>
using namespace std;

vector<int> sieve(int n) {
    vector<bool> isPrime(n + 1, true);
    isPrime[0] = isPrime[1] = false;

    for (int i = 2; i * i <= n; i++) {
        if (isPrime[i]) {
            for (int j = i * i; j <= n; j += i) {
                isPrime[j] = false;
            }
        }
    }

    vector<int> primes;
    for (int i = 2; i <= n; i++) {
        if (isPrime[i]) primes.push_back(i);
    }
    return primes;
}

int main() {
    int n = 50;
    auto primes = sieve(n);
    cout << "Primes up to " << n << ": ";
    for (int p : primes) cout << p << " ";
    cout << endl;
}

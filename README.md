# 🔥 Elite Chat Server v2.0

![Production Ready](https://img.shields.io/badge/Production-Ready-brightgreen.svg)
![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![Boost.Asio](https://img.shields.io/badge/Boost.Asio-latest-orange.svg)
![Threads](https://img.shields.io/badge/Threads-Scalable-yellow.svg)
![Performance](https://img.shields.io/badge/Performance-10k%2B%20clients-red.svg)

Многопользовательский консольный чат-сервер на **Boost.Asio** с поддержкой **C++20 coroutines**. Senior-level production код.

## ✨ Особенности

| Функция | Статус |
|---------|--------|
| ✅ C++20 Coroutines (`co_await`, `co_return`) | **Production** |
| ✅ Multi-threaded IO (все ядра CPU) | **Scalable** |
| ✅ Thread-safe (shared_mutex) | **100% safe** |
| ✅ Zero-copy broadcast | **High perf** |
| ✅ Graceful shutdown (SIGINT) | **Production** |
| ✅ UX Prompts (`type your name:`, `[username]>`) | **User friendly** |
| ✅ Команды `/quit` | **Pro** |

## 🚀 Быстрый старт

```bash
# Клонировать и собрать
git clone <repo>
cd EliteChat
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .

# Запуск сервера
./chat_server 5050
# stone-chat

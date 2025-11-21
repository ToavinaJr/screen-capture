#include <iostream>
#include "../src/threading/ThreadPool.h"
#include <chrono>
#include <atomic>

int main() {
    std::cout << "=== Test ThreadPool ===\n\n";
    
    try {
        ThreadPool pool(4);
        std::cout << "✓ ThreadPool créé avec 4 threads\n";
        
        std::atomic<int> counter{0};
        
        // Enqueuer 10 tâches
        for (int i = 0; i < 10; ++i) {
            pool.enqueue([&counter, i]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                counter++;
                std::cout << "  Tâche " << i << " terminée\n";
            });
        }
        
        std::cout << "✓ 10 tâches enfilées\n";
        
        // Attendre que toutes les tâches soient terminées
        std::this_thread::sleep_for(std::chrono::seconds(3));
        
        std::cout << "\n✓ Compteur final: " << counter << "/10\n";
        
        if (counter == 10) {
            std::cout << "✓ Tous les tests réussis!\n";
            return 0;
        } else {
            std::cout << "✗ Erreur: toutes les tâches n'ont pas été exécutées\n";
            return 1;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "✗ Erreur: " << e.what() << std::endl;
        return 1;
    }
}
